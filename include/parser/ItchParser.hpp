#pragma once

#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <cstring> 
#include <memory>
#include <array>
#include "ItchMessages.hpp"
#include "../../src/utils/SpmcRingBuffer.cpp"

namespace ITCH {

    using DispatchTableEntry = void(*)(const char*);

    class MmapReader {
    public:
        MmapReader() = delete; // No default constructor

        MmapReader(const char* filename);

        // Prevent accidental copy construction or assignment
        MmapReader(const MmapReader& other) = delete;
        MmapReader& operator=(const MmapReader& other) = delete;

        ~MmapReader();

        const char* nextMsg();

        void parse();

        static void setBuffer(SPMC_Queue* buf) { buffer_ = buf; }

    private:
        int fd;
        char* start;
        char* cursor;
        char* end;  
        static SPMC_Queue* buffer_;

        // Avoid branchy code in parse
        std::array<DispatchTableEntry, 256> dispatchTable = {};

        void initDispatchTable();

        template <typename MsgEnvelope>
        static void emitToBuffer(const MsgEnvelope& msg, msg_type type);

        static SystemEventMsg                         constructSystemEventMsg(char const* data);
        static StockDirectoryMsg                      constructStockDirectoryMsg(char const* data);
        static StockTradingActionMsg                  constructStockTradingActionMsg(char const* data);
        static RegSHORestrictionMsg                   constructRegSHORestrictionMsg(char const* data);
        static MarketParticipantPositionMsg           constructMarketParticipantPositionMsg(char const* data);
        static MWCBDeclineLevelMsg                    constructMWCBDeclineLevelMsg(char const* data);
        static MWCBStatusMsg                          constructMWCBStatusMsg(char const* data);
        static IPOQuotingPeriodUpdateMsg              constructIPOQuotingPeriodUpdateMsg(char const* data);
        static LULDAuctionCollarMsg                   constructLULDAuctionCollarMsg(char const* data);
        static OperationalHaltMsg                     constructOperationalHaltMsg(char const* data);
        static AddOrderMsg                            constructAddOrderMsg(char const* data);
        static AddOrderMPIDAttributionMsg             constructAddOrderMPIDAttributionMsg(char const* data);
        static OrderExecutedMsg                       constructOrderExecutedMsg(char const* data);
        static OrderExecutedWithPriceMsg              constructOrderExecutedWithPriceMsg(char const* data);
        static OrderCancelMsg                         constructOrderCancelMsg(char const* data);
        static OrderDeleteMsg                         constructOrderDeleteMsg(char const* data);
        static OrderReplaceMsg                        constructOrderReplaceMsg(char const* data);
        static TradeMsg                               constructTradeMsg(char const* data);
        static CrossTradeMsg                          constructCrossTradeMsg(char const* data);
        static BrokenTradeMsg                         constructBrokenTradeMsg(char const* data);
        static NOIIMsg                                constructNOIIMsg(char const* data);
        static RetailInterestMsg                      constructRetailInterestMsg(char const* data);
        static DirectListingWithCRPDMsg               constructDirectListingWithCRPDMsg(char const* data);

        ts getDataTimestamp(char const* data);
        ts strToTimestamp(char const* timestampStr);
        msg_type getDataMessageType(char const* data);
    };

    struct MsgEnvelope {
        msg_type type;
        uint16_t length;
        alignas(8) char payload[MAX_ITCH_MSG_SIZE];  // explicitly aligned and padded

        template <typename MsgT>
        void setPayload(const MsgT& msg, msg_type t) {
            type = t;
            length = sizeof(MsgT);
            static_assert(sizeof(MsgT) <= MAX_ITCH_MSG_SIZE, "Message too large for payload");
            std::memset(payload, 0, sizeof(payload));  // clear unused bytes
            std::memcpy(payload, &msg, sizeof(MsgT));
        }

        template <typename MsgT>
        const MsgT& as() const {
            return *reinterpret_cast<const MsgT*>(payload);
        }
    };

}