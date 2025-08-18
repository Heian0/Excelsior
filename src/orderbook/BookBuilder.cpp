#include <thread>
#include <atomic>
#include <iostream>
#include "../utils/SpmcRingBuffer.cpp"
#include "../../include/parser/ItchParser.hpp"

class Orderbook;

class BookBuilder {
public:
    BookBuilder(SPMC_RingBuffer<ITCH::MsgEnvelope>& ring, uint16_t securityNameIdx, Orderbook& book)
        : ringBuffer_(ring), securityId_(securityNameIdx), running_(true), book_(book) {
        worker_ = std::thread(&BookBuilder::pollLoop, this);
    }

    ~BookBuilder() {
        running_ = false;
        if (worker_.joinable())
            worker_.join();
    }

private:
    void pollLoop() {
        ConsumerState state; // owns read index
        while (running_) {
            auto maybe = ringBuffer_.tryRead(state.readIdx);
            if (!maybe.has_value()) continue;

            const ITCH::MsgEnvelope& msg = *maybe;

            // Check type and filter by security ID
            if (msg.type == ITCH::AddOrderMsgType) {
                const ITCH::AddOrderMsg& order = msg.as<ITCH::AddOrderMsg>();
                if (order.securityNameIdx == securityId_) {
                    std::cout << "AddOrder for " << securityId_
                              << " at price " << order.price
                              << " qty " << order.quantity << '\n';
                }
            }

            ++state.readIdx;
        }
    }

    SPMC_RingBuffer<ITCH::MsgEnvelope>& ringBuffer_;
    uint16_t securityId_;
    std::atomic<bool> running_;
    std::thread worker_;
    Orderbook& book_;
};
