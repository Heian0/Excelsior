#include <cstddef>
#include <array>
#include <unordered_map>
#include <vector>

struct Level {
    size_t price;
    size_t volume;
    int volAhead = -1; // -1 if we dont have an order in this level
    int volBehind = -1;
};

struct Order {
    int id;
    int price;
    int volume;
    Level* location;
};


class EquitiesOrderbook {
private:
    static constexpr int numTopLevels = 50;
    static constexpr bool ask = 0, bid = 1;

    // Store in L1 cache
    alignas(64) std::array<Level, numTopLevels> asks {};
    alignas(64) std::array<Level, numTopLevels> asksMid {};
    alignas(64) std::array<Level, numTopLevels> bids {};
    alignas(64) std::array<Level, numTopLevels> bidsMid {};

    // Store in L2 cache
    alignas(64) std::unordered_map<int, Order> orderMap {};

    // Store in L3 cache
    std::vector<Level> deepAsks(); // Preallocate
    std::vector<Level> deepBids(); // Preallocate

public:
    EquitiesOrderbook();

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