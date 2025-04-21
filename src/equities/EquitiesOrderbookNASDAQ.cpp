#include "../../include/equities/EquitiesOrderbookNASDAQ.hpp"

/*
    Things to keep in mind/Todo

    Manage shadow liquidity

    Put market maker orders in correct queue location, wait for preexisiting liquidity to fill

    Configure MM for Nasdaq and NYSE

    Hashmap -  Order id maps to price level and a bool for whether it is in front of our order
    at this level. If it is, when it gets executed or cancelled, we can change the vol in front
    in our Level struct. 

    Ring buffer is almost always stable addressing. The only time it isnt stable is if we perform a shift,
    which should very rarely ever happen. Thus it would probably be good if we could invalidate addresses for
    level mapped by our flat hash when a level is shifted down - all we have to do is check if the price levels
    match.


    Ok, big changes

    We are going to use two threads. One will handle asks, and one will handle bids.

    Now, the special case is if our parse gets an execute message. In this case, the side
    being executed on will get a fill message, and the side doing execution will get a wait.
    When the side being filled completes, and we know how much volume to remove from the 
    executor's side, we will send that thread a message and let it know the volume. Then,
    the lock will be released. Actually, we can still allow the executing side to get updated,
    as long as that specific price level is not being changed or if we are not trying
    to read (this might actually be an uneccesary optimization).

    This leads us into the next part - for the actual market making agent, it will periodically read
    the top 100 levels from the both other threads.

    Thus, in total we will have 4 cores, 1 to read updates and parse them and send them to the correct queues,
    two threads to update both sides, and 1 core to make market making decisions.

    Furthermore, because we have access to the best and worst asks at any given time,
    which are essentially boundaries, we can estimate approximately which cache line
    our data would be stored on.

    Also, use Cache-aware interpolation to estimate which of the 25 cache lines the price level is stored on

    Wow this is actually getting pretty fast...i think

    And now a new way to keep order id map. Because order id's are never reused in a trading day,
    what we can do is delete order id's from our map. So the size of the map which we should allocate
    is what is the max number of active orders at any time in the day. Even for liquid stocks
    in periods of high volatility, our map should fit in L2 cache, and if not, definitely L3 cache.
    After all, the majority of messages should be add or cancel orders, with most orders being cancelled
    almost immediately.




*/

EquitiesOrderbook::EquitiesOrderbook() {
    // Fill bids and asks
    for (int i = 0; i < numTopLevels; i++) {
        asks[i] = {std::numeric_limits<int>::min(), 0, 0, 0};
        bids[i] = {std::numeric_limits<int>::max(), 0, 0, 0};
    }
}

void EquitiesOrderbook::AddOrder(Order order, bool side) {
    /*
        Need to implement prefetching

        A few things can happen here. First, note that we maintain the boundaries
        of our top 100 asks. If this order falls out of this range on the non-optimal
        side, i.e. we are maintaining asks from 100 - 200 and this ask is 201, we can
        simply call AddDeepOrder and return.

        Now, if the order falls on the optimal side, i.e., we maintain 100 - 200 and
        this ask is at price 99, then we do: If our array is full -> evict the least optimal 
        level, then insert by shifting the head. If our array is not full, just shift the head.

        Now there is one final case, which may occur when we first begin recieving data.
        However, this should rarely occur. If we have only a few levels, say 100, 104, 105,
        and we get an order at say 102, we need to insert by shifting. If the insertion goes
        out of range, evict the worst level and then insert, otherwise just insert. Note
        that the levels in the array should be chromatic most of the time, and thus this shift
        should rarely happen except for when we first start reading from the market data feed.
    */

    // Bids
    if (side == bid) {
        if (bidsHeader.bidsSize == numTopLevels && order.price < bidsHeader.worstBid) [[unlikely]] {
            AddDeepOrder(order, side);
            return;
        }
        AddBidOrder(order);
    }
     
    // Asks
    else {
        if (asksHeader.asksSize == numTopLevels && order.price > asksHeader.worstAsk) [[unlikely]] {
            AddDeepOrder(order, side);
            return;
        }
        AddAskOrder(order);
    }
}

void EquitiesOrderbook::AddAskOrder(Order order) {

    /*
        Search for insert index. If during search, we find a level with the same price, add order
        volume to that level and directly return. This should be the most common case.

        If this does not occur, find the index in which we need to insert. Note that we will
        shift our tail index over only if the array is not full.
        
        Now we have two cases, insert at front and insert in the middle. But first, if the array is full,
        evict the worst ask, which will shift tail back.

        If we are inserting at the front simply shift the head indexm and insert the new level.
        If it is not full, increase size by 1

        if we are not inserting at the front shift everything down until the tail (we already evicted) which
        will also move tail index down 1.
        Then insert, and if the array wasnt full increase size otherwise dont.
    */

    if (asksHeader.asksSize == numTopLevels && order.price > asksHeader.worstAsk) [[unlikely]] {
        AddDeepOrder(order, ask);
        return;
    }

    // Interpolate index - very good for cache if we can get a direct hit
    if (asksHeader.asksSize == numTopLevels) {
        int guess = asksHeader.asksHead + (order.price - asksHeader.bestAsk);
        if (asks[guess].price == order.price) {
            asks[guess].volume += order.volume;
            asksIdMap[order.id] = {order.price, order.volume, &asks[guess]};
            return;
        }
    }

    int leftPrice = -1, rightPrice, idx = -1;
    for (int i = asksHeader.asksHead, count = 0; count < asksHeader.asksSize + 1; i = (i + 1) % numTopLevels, count++) {
        rightPrice = asks[i].price;

        // Level Exists
        if (rightPrice == order.price) {
            asks[i].volume += order.volume;
            asksIdMap[order.id] = {order.price, order.volume, &asks[i]};
            return;
        }

        // Level Does not exist, thus we need to insert
        else if (order.price > leftPrice && order.price < rightPrice) {
            idx = i;
            break;
        }

        leftPrice = rightPrice;
    }

    // We did not find a price level, and did not find an insert, something is wrong
    if (idx == -1) exit(1);

    bool evicted = false;
    if (asksHeader.asksSize == numTopLevels) {
        EvictAskLevel();
        evicted = true;
    }

    // Front insert
    if (idx == asksHeader.asksHead) {
        asksHeader.asksHead == 0 ? asksHeader.asksHead = numTopLevels - 1 : asksHeader.asksHead--;
        asks[asksHeader.asksHead] = {order.price, order.volume, 0, 0};
        asksIdMap[order.id] = {order.price, order.volume, &asks[asksHeader.asksHead]};
        asksHeader.bestAsk = order.price;
        if (!evicted) asksHeader.asksSize++;
    }

    // Non front insert
    else {
        Level temp = asks[idx], next;
        for (int j = idx; j != asksHeader.asksTail; j = (j + 1) % numTopLevels) {
            next = asks[(j + 1) % numTopLevels];
            asks[(j + 1) % numTopLevels] = temp;
            temp = next;
        }
        asksHeader.asksTail++;
        asks[idx] = {order.price, order.volume, 0, 0};
        asksIdMap[order.id] = {order.price, order.volume, &asks[idx]};
        if (!evicted) asksHeader.asksSize++;
    }
}

void EquitiesOrderbook::AddBidOrder(Order order) {

}

// Need to manage huge orders sweeping 100 levels (in case, but this should never happen)
void EquitiesOrderbook::FillAskOrder(int id) {

    // need to prefetch top of vector

    Order order = GetAskOrderById(id);

    int remainingVol = order.volume;
    int cleared = 0;
    for (int i = bidsHeader.bidsHead, count = 0; count < bidsHeader.bidsSize + 1; i = (i + 1) % numTopLevels, count++) {
        Level& bidOrder = bids[i];
        if (bidOrder.price >= order.price) {
            int executedVol = std::min(remainingVol, bidOrder.volume);
            bidOrder.volume -= executedVol;
            remainingVol -= executedVol;
            if (bidOrder.volume == 0) {
                bidsHeader.bidsHead++;
                cleared++;
            }
            if (remainingVol == 0) break;
        }
        else break;
    }

    bidsHeader.bidsSize -= cleared;
    bidsHeader.bestBid =  bids[bidsHeader.bidsHead].price;

    // Handle order id removal - dont need to do anything for filled bids because we check expected price,
    // and if we ever get another order with the exact same id it will overwrite LevelLocation

    order.location->volume -= (order.volume - remainingVol);

    if (remainingVol != 0) {
        // Check if order cleared out the entire TopNLevels and still has liquidity
        // Should never happen, but if it did, send the remaining
        // volume as a fill order to the deeper levels, otherwise
        // just add remaining volatility directoly to ask side
    }
}

void EquitiesOrderbook::EvictLevel(bool side) {
    if (side == bid) EvictBidLevel();
    else EvictAskLevel();
}

void EquitiesOrderbook::EvictAskLevel() {
    int oldTail = asksHeader.asksTail;
    asksHeader.worstAsk = asks[asksHeader.asksTail - 1].price;
    asksHeader.asksSize--;
    asksHeader.asksTail = ((asksHeader.asksTail - 1) % numTopLevels + numTopLevels) % numTopLevels;
    Level toEvict = asks[oldTail];
    AddDeepAskOrder({-1, toEvict.price, toEvict.price, nullptr});
}

void EquitiesOrderbook::EvictBidLevel() {
    int oldTail = bidsHeader.bidsTail;
    bidsHeader.worstBid = bids[bidsHeader.bidsTail - 1].price;
    bidsHeader.bidsSize--;
    bidsHeader.bidsTail = ((bidsHeader.bidsTail - 1) % numTopLevels + numTopLevels) % numTopLevels;
    Level toEvict = bids[bidsHeader.bidsTail];
    AddDeepBidOrder({-1, toEvict.price, toEvict.price, nullptr});
}

Order EquitiesOrderbook::GetAskOrderById(int id) {
    LevelLocation cached = asksIdMap[id];
    Level* levelLocation = cached.location;
    if (levelLocation->price != cached.expectedPrice) {
        // find the correct location
        for (int i = asksHeader.asksHead; i != asksHeader.asksTail; i = (i + 1) % numTopLevels) {
            if (asks[i].price == cached.expectedPrice) {
                levelLocation = &asks[i];
                break;
            }
        }
    }
    return {id, cached.expectedPrice, cached.volume, levelLocation};
}

void EquitiesOrderbook::AddDeepOrder(Order order, bool side) {
    
}

void EquitiesOrderbook::AddDeepAskOrder(Order order) {
    
}


void EquitiesOrderbook::AddDeepBidOrder(Order order) {
    
}