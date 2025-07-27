#include "../include/parser/ItchMessages.hpp"
#include "../include/parser/ItchParser.hpp"
#include <iostream>
#include <thread>

SPMC_Queue* ITCH::MmapReader::buffer_ = nullptr;

inline std::string toTicker(const char (&sym)[8])
{
    const char* end = std::find(sym, sym + 8, ' ');   // trim right spaces
    return std::string(sym, end);
}

int main() {
    SPMC_Queue spmcQ(4096);
    const char* filename = "08302019.NASDAQ_ITCH50";

    try {
        ITCH::MmapReader reader(filename);
        reader.setBuffer(&spmcQ);
        std::thread parserThread([&]() {
            reader.parse();  // this will emit messages to the queue
        });

        // Reader logic
        uint64_t readIndex = 0;
        PayloadSize size;
        std::array<uint8_t, 64> scratch;
        while (true) {
            if (spmcQ.Read(readIndex % spmcQ.size(), scratch.data(), size)) {
                ++readIndex;            // only after a successful read

                const char type = static_cast<char>(scratch[0]);

                switch (type) {
                    case 'A': {
                        const ITCH::AddOrderMsg& m =
                            *reinterpret_cast<const ITCH::AddOrderMsg*>(scratch.data());

                        std::cout << "[AddOrder] Ticker: " << toTicker(m.ticker)
                                << "  Loc:" << m.securityNameIdx
                                << "  Px:$"  << m.price / 10000.0
                                << '\n';
                        break;
                    }
                    case 'P': {
                        const ITCH::TradeMsg& m =
                            *reinterpret_cast<const ITCH::TradeMsg*>(scratch.data());

                        std::cout << "[Trade]   Ticker: " << toTicker(m.ticker)
                                << "  Qty:"   << m.quantity
                                << "  Px:$"   << m.price / 10000.0
                                << '\n';
                        break;
                    }
                    default:
                        // std::cout << "[Unhandled] MsgType: " << type << '\n';
                        break;
                    }
                }

            else {
                std::this_thread::sleep_for(std::chrono::microseconds{50});
            }
        }

        parserThread.join();
    } catch (const std::exception& e) {
        std::cerr << "Parser failed: " << e.what() << '\n';
    }

    return 0;
}