/*

    Let's talk about system design again, but this time in a more structured manner.
    Before we talk about the how, we will talk about the what - what does this system
    need to support? Then we can talk about how it can be architected to support such
    requirements.

    Requirements:

        - Must provide instant access to top 100 levels of the book for downstream
        components.

        - Must be able to deal with levels in the book getting filled. It must maintain
        the deeper levels and maintain the top 100 levels as needed.
        
        - When recording results of trades, it must take into account potential latency
        and when the order will actually get filled. 
        
        - Orders our system enter into the book must also be simulated, i.e., they will
        not actually be executed against.

    For the first requirement, I've deceided to go with a double buffering system via
    L3 cache, where downstream components exist on a seperate core. The buffer will have
    a versioning system, where after apply an update from the market, the MDA core will
    increment a write_start value in buffer 1. Then, it will perform the write, and increment
    end write_end, which will be another value in the buffer, by 2. Lastly, if after 
    writing there is no new market data, it will increment write_start by 1, making
    write_start and write_end even and equal. Otherwise, it will increment it by 2
    and begin a new write after processing the new market data update.

    On the other core, we will query the buffer, and only if write_start are equal and
    even will we make a trade decision on this data by copying it into our own buffer. 
    We will then tell core A to write to the buffer we just read from.

    The actual orderbook system will be maintained by two ring buffers (L1): one for the 
    top 100 levels, and one for the next 100 levels. Any deeper orders will be placed in
    a std::vector in L2 cache. We will also store an optimized hashmap mapping order id
    to 

*/

// Each level should have price, volume, a marker indicating whether or not our quote
// lives in this book, and how much volume exists in front of it. Also, you should have a
// seperate hashmap which maps our order's ids to its address in the book, so that
// we dont conflict with the real orders.

// When we get an update, if its marked as our order, we check to see if it can be
// executed immediately, if not, slap it in the book.



#include <vector>
#include <array>
#include <limits>
#include "absl/container/flat_hash_map.h"
#include <boost/container/flat_map.hpp>

struct Level {
    int price;
    int volume;

    // If possible, try to store these somewhere else
    int volFront;
    int volBehind;
};

struct Order {
    int id;
    int price;
    int volume;
    Level* location;
};

struct LevelLocation {
    int expectedPrice;
    int volume;
    Level* location;
};

struct AsksHeader {
    int bestAsk = 0, worstAsk = std::numeric_limits<int>::max();
    int asksHead {}, asksTail {}, asksSize {};
};

struct BidsHeader {
    int bestBid = std::numeric_limits<int>::max(), worstBid = 0;
    int bidsHead {}, bidsTail {}, bidsSize {};
};

struct Update {
    int type;
    int id;
    int price;
    int volume;
};

struct ImportExportHeader {

    std::array<Update, 8>& exportBufferWrite;
    std::array<Update, 8>& exportBufferRead;

    std::array<Level, 8>& importBuffer;

    // Convert to uint8_t later
    int requestExport, exportCompleted;
    int requestImport, importCompleted;
};

class EquitiesOrderbook {
private:
    static constexpr int numTopLevels = 100;
    static constexpr bool ask = 0, bid = 1;

    // Store in L1 cache of core 1
    alignas(64) AsksHeader asksHeader;
    alignas(64) std::array<Level, numTopLevels> asks;
    // Second 100 levels, so that we always have access to top 100. If this dips below 25 in size,
    // signal another core to write levels to another cache line in L1 which we can read to refill.
    alignas(64) AsksHeader asksHeaderMid;
    alignas(64) std::array<Level, numTopLevels> asksMid;

    // Store in L1 cache
    alignas(64) BidsHeader bidsHeader;
    alignas(64) std::array<Level, numTopLevels> bids;
    // Second 100 levels, so that we always have access to top 100. If this dips below 25 in size,
    // signal another core to write levels to another cache line in L1 which we can read to refill.
    alignas(64) BidsHeader bidsHeaderMid;
    alignas(64) std::array<Level, numTopLevels> bidsMid;
    
    alignas(64) ImportExportHeader importExportHeader;
    alignas(64) std::array<Update, 8> exportBuffer1;
    alignas(64) std::array<Update, 8> exportBuffer2;
    alignas(64) std::array<Level, 8> importBuffer;

    // Store in L2 cache
    absl::flat_hash_map<int, LevelLocation> asksIdMap;
    absl::flat_hash_map<int, LevelLocation> bidsIdMap;

    // Store in L3 cache
    boost::container::flat_map<int, Level> deepAsks;
    boost::container::flat_map<int, Level> deepBids;

public:
    explicit EquitiesOrderbook();

    void AddOrder(Order order, bool side);

    void AddAskOrder(Order order);

    void AddBidOrder(Order order);

    void AddDeepOrder(Order order, bool side);

    void AddDeepAskOrder(Order order);

    void AddDeepBidOrder(Order order);

    void FillOrder(Order order, bool side);

    void FillAskOrder(int id);

    void FillBidOrder(Order order);

    void CancelOrder(Order order, bool side);

    void ModifyOrder(Order order, bool side);

    void EvictLevel(bool side);

    void EvictAskLevel();

    void EvictBidLevel();

    int GetApproxLevel();

    Order GetAskOrderById(int id);

    Order GetBidOrderById(int id);
};