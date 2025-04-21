#include <vector>
#include <array>
#include <limits>
#include "absl/container/flat_hash_map.h"
#include <boost/container/flat_map.hpp>

/*
    Hybrid Orderbook implementation, sorted fixed size ring buffer with contigous memory access and constant time 
    shifting for the first 100 levels, and sorted std::vector for outer levels. We can use linear search for 
    inserting into the ring buffer, as 100 levels is constant time and will outweigh the "better" theoretical 
    time complexities of node based data structures such as std::map. For the outer levels, we can pass them to
    another thread to maintain, and we can potentially use binary insertion.
*/

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



/*

    Ok,think I may have finally figured it out. The full system design

    Two cores. One for hot path, one for deep level maintenance. Hold on, let me explain.
    Basically, one core can do it all. Parsing, orderbook mutation, quoting. It's because L3
    access is just so slow.

    lets begin by talking about caches.

        The hotpath core's L1 cache will contain for both bids and asks (all cache line aligned):

            Metadata header + std::array ring buffer for levels 1 - 100.
            Metadata header + std::array ring buffer for levels 101-200.
            Import/Export header (small, fits on a single cache line)
            2 fixed size buffer which stores deltas for export, could be a std::array of updates. 
            2 fixed size buffer which stores the next best price levels that were retrieved from deep levels.
            We want these buffers to not be too big - maybe like 2 or 3 cache licnes.

        The hotpath core's L2 cache will contain (cache line aligned):

            Open adressing, optimized hashmap that maps order id to price, volume, and address in
            ring buffer in L1 cache for asks
            Open adressing, optimized hashmap that maps order id to price, volume, and address in
            ring buffer in L1 cache for bids

        The hotpath core's L3 cache will contain only one thing:

            A flat redblack tree, std::vector, or some other limitless data structure storing deep asks.
            A flat redblack tree, std::vector, or some other limitless data structure storing deep bids.

    Now how will we use these data structures?

        When we get an update, we will first parse it. Then, we check if it fits in the first 100 levels
        by checking against the header. If it does, we just insert it. If not, We check if it fits in the
        next 100. If it fits, we insert. If not, we send it to the export buffer.

        If levels from the top get filled, we retrieve levels from the next 100 ring buffer (101 - 200). if we
        get new optimal order which replace the front, we move our worst levels to the next 100. If the next 100 is
        full, we evict levels to the export buffer.

        When the first export buffer reaches capacity, we flag for export. The other core will read this, and
        try to access the  first export buffer, which will cause sharing of the cache lines in this first buffer.
        The main core can then write to the other buffer without interruption.

        Then, the alternate core will write the levels to the infinite depth book in L3. In the other case,
        which is that the second 100 levels is missing say like 10 levels, we fill flag an import request. The 
        other core will then write the next best 10 levels to the first import buffer. The main core will then
        get these cache lines via cache coherency, and the use the levels to refill the seccond 101-200
        ring buffer. Any excess updates can be written back to the export buffer, and note we need to clear export
        buffer before we import.

        This will allow our pricing algo to always have access to info about the top 100 levels hot in
        L1 cache. Now the hard part is choosing the right size and time to import/export, and if neccessary,
        explicitly slow down faster operations to lower jitter.

        Now another point would be to prefetch cache lines. If an order doesnt fit in the first 100 levels,
        prefectch the header for the next 200, as we need to access that next.
*/

// Buffer updates - 2 bit for type of update

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
    

    // Ask for export - flip flag as soon as we have an export delta - this wont be work done by main core anyway
    // however, we only signal core to import Levels into 100-200 range when we are at 10 percent capacity, we
    // want to rarely do this work as it uses the main core
    // Also note that if we need to import levels, we need to export all the export deltas first to be 
    // applied first in a batch before we fill the import buffer - thus we flip export buffer needsExport
    // as soon as we can so all the updates are applied as fast as possible


    // if the export buffer has been processed  by other core, maybe only flip a flag in the header, and main
    // core can just completely clear out the buffer which reduces the number of cache lines it will need to fetch
    // from L3 (we know it has been cleared)
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