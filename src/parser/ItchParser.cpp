#include "../../include/parser/ItchParser.hpp"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>

namespace ITCH {

    inline constexpr uint8_t readU8(const char* data, size_t offset) {
        return *reinterpret_cast<const uint8_t*>(data + offset);
    }

    inline constexpr char readChar(const char* data, size_t offset) {
        return static_cast<char>(readU8(data, offset));
    }

    inline constexpr uint16_t readU16(const char* data, size_t offset) {
        return be16toh(*reinterpret_cast<const uint16_t*>(data + offset));
    }

    inline constexpr uint32_t readU32(const char* data, size_t offset) {
        return be32toh(*reinterpret_cast<const uint32_t*>(data + offset));
    }

    inline uint64_t readU48(const char* d, size_t off) {
        return (uint64_t(readU16(d, off)) << 32) | readU32(d, off + 2);
    }

    inline constexpr uint64_t readU64(const char* data, size_t offset) {
        return be64toh(*reinterpret_cast<const uint64_t*>(data + offset));
    }

    inline void copyBytes(void* dst, const char* data, size_t offset, size_t len) {
        std::memcpy(dst, data + offset, len);
    }

    MmapReader::MmapReader(const char* filename) 
        : fd(open(filename, O_RDONLY)), start(nullptr), cursor(nullptr), end(nullptr) {
        
        if (fd == -1) {
            throw std::runtime_error("Failed to open file: " + std::string(filename));
        }

        struct stat sb;
        if (fstat(fd, &sb) == -1) {
            close(fd);
            throw std::runtime_error("Failed to get file stats");
        }

        start = static_cast<char*>(mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
        if (start == MAP_FAILED) {
            close(fd);
            throw std::runtime_error("Failed to mmap file");
        }

        cursor = start;
        end = start + sb.st_size;

        initDispatchTable();
    }

    MmapReader::~MmapReader() {
        if (start != MAP_FAILED && start != nullptr) {
            munmap(start, end - start);
        }
        if (fd != -1) {
            close(fd);
        }
    }

    void MmapReader::parse() {
        while (const char* raw = nextMsg()) {
            msg_type type = getDataMessageType(raw);
            auto& handler = dispatchTable[static_cast<uint8_t>(type)];

            [[likely]] if (handler) {
                handler(raw);
            } 

            else {
                std::cerr << "Unknown message type: " << static_cast<int>(type) << "\n";
            }
        }
    }

    void MmapReader::initDispatchTable() {
        dispatchTable[SystemEventMsgType] = [](const char* data) {
            emitToBuffer(constructSystemEventMsg(data), SystemEventMsgType);
        };
        dispatchTable[StockDirectoryMsgType] = [](const char* data) {
            emitToBuffer(constructStockDirectoryMsg(data), StockDirectoryMsgType);
        };
        dispatchTable[StockTradingActionMsgType] = [](const char* data) {
            emitToBuffer(constructStockTradingActionMsg(data), StockTradingActionMsgType);
        };
        dispatchTable[RegSHORestrictionMsgType] = [](const char* data) {
            emitToBuffer(constructRegSHORestrictionMsg(data), RegSHORestrictionMsgType);
        };
        dispatchTable[MarketParticipantPositionMsgType] = [](const char* data) {
            emitToBuffer(constructMarketParticipantPositionMsg(data), MarketParticipantPositionMsgType);
        };
        dispatchTable[MWCBDeclineLevelMsgType] = [](const char* data) {
            emitToBuffer(constructMWCBDeclineLevelMsg(data), MWCBDeclineLevelMsgType);
        };
        dispatchTable[MWCBStatusMsgType] = [](const char* data) {
            emitToBuffer(constructMWCBStatusMsg(data), MWCBStatusMsgType);
        };
        dispatchTable[IPOQuotingPeriodUpdateMsgType] = [](const char* data) {
            emitToBuffer(constructIPOQuotingPeriodUpdateMsg(data), IPOQuotingPeriodUpdateMsgType);
        };
        dispatchTable[LULDAuctionCollarMsgType] = [](const char* data) {
            emitToBuffer(constructLULDAuctionCollarMsg(data), LULDAuctionCollarMsgType);
        };
        dispatchTable[OperationalHaltMsgType] = [](const char* data) {
            emitToBuffer(constructOperationalHaltMsg(data), OperationalHaltMsgType);
        };
        dispatchTable[AddOrderMsgType] = [](const char* data) {
            emitToBuffer(constructAddOrderMsg(data), AddOrderMsgType);
        };
        dispatchTable[AddOrderMPIDAttributionMsgType] = [](const char* data) {
            emitToBuffer(constructAddOrderMPIDAttributionMsg(data), AddOrderMPIDAttributionMsgType);
        };
        dispatchTable[OrderExecutedMsgType] = [](const char* data) {
            emitToBuffer(constructOrderExecutedMsg(data), OrderExecutedMsgType);
        };
        dispatchTable[OrderExecutedWithPriceMsgType] = [](const char* data) {
            emitToBuffer(constructOrderExecutedWithPriceMsg(data), OrderExecutedWithPriceMsgType);
        };
        dispatchTable[OrderCancelMsgType] = [](const char* data) {
            emitToBuffer(constructOrderCancelMsg(data), OrderCancelMsgType);
        };
        dispatchTable[OrderDeleteMsgType] = [](const char* data) {
            emitToBuffer(constructOrderDeleteMsg(data), OrderDeleteMsgType);
        };
        dispatchTable[OrderReplaceMsgType] = [](const char* data) {
            emitToBuffer(constructOrderReplaceMsg(data), OrderReplaceMsgType);
        };
        dispatchTable[TradeMsgType] = [](const char* data) {
            emitToBuffer(constructTradeMsg(data), TradeMsgType);
        };
        dispatchTable[CrossTradeMsgType] = [](const char* data) {
            emitToBuffer(constructCrossTradeMsg(data), CrossTradeMsgType);
        };
        dispatchTable[BrokenTradeMsgType] = [](const char* data) {
            emitToBuffer(constructBrokenTradeMsg(data), BrokenTradeMsgType);
        };
        dispatchTable[NOIIMessageMsgType] = [](const char* data) {
            emitToBuffer(constructNOIIMsg(data), NOIIMessageMsgType);
        };
        dispatchTable[RetailInterestMsgType] = [](const char* data) {
            emitToBuffer(constructRetailInterestMsg(data), RetailInterestMsgType);
        };
        dispatchTable[DirectListingWithCRPDMsgType] = [](const char* data) {
            emitToBuffer(constructDirectListingWithCRPDMsg(data), DirectListingWithCRPDMsgType);
        };   
    }

    template <typename MsgT>
    void MmapReader::emitToBuffer(const MsgT& msg, msg_type type) {
        buffer_->Write(sizeof(MsgT), [&](uint8_t* data) {
            static_assert(sizeof(MsgT) <= 64, "Message too large for Block buffer");
            std::memcpy(data, &msg, sizeof(MsgT));
        });
    }

    const char* MmapReader::nextMsg() {
        
        if (cursor >= end) return nullptr;

        if (cursor + 2 > end) return nullptr;

        uint16_t msgLength = ntohs(*reinterpret_cast<uint16_t*>(cursor));
        
        if (cursor + 2 + msgLength > end) {
            return nullptr;
        }

        const char* msgStart = cursor + 2;
        cursor += 2 + msgLength;
        
        return msgStart;
    }

    msg_type MmapReader::getDataMessageType (const char* msg) {
        return msg[0];
    }

    SystemEventMsg MmapReader::constructSystemEventMsg(const char* data) {
        return {
            readChar(data, 0), readU16(data, 1), readU16(data, 3), readU64(data, 5), readU8(data, 13)
        };
    }

    StockDirectoryMsg MmapReader::constructStockDirectoryMsg(const char* data) {
        StockDirectoryMsg m;
        m.msgType = readChar(data, 0);
        m.securityNameIdx = readU16(data, 1);
        m.seqNumber = readU16(data, 3);
        m.timestamp = readU64(data, 5);
        copyBytes(m.ticker, data, 13, 8);
        m.marketCategory = readU8(data, 21);
        m.financialStatusIndicator = readU8(data, 22);
        m.roundLotSize = readU32(data, 23);
        m.roundLotsOnly = readU8(data, 27);
        m.securityClass = readU8(data, 28);
        copyBytes(m.issueSubType, data, 29, 2);
        m.authenticity = readU8(data, 31);
        m.shortSaleThresholdIndicator = readU8(data, 32);
        m.IPOFlag = readU8(data, 33);
        m.LULDReferencePriceTier = readU8(data, 34);
        m.ETPFlag = readU8(data, 35);
        m.ETPLeverageFactor = readU32(data, 36);
        m.inverseIndicator = readU8(data, 40);
        return m;
    }

    StockTradingActionMsg MmapReader::constructStockTradingActionMsg(const char* data) {
        StockTradingActionMsg m;
        m.msgType = readChar(data, 0);
        m.securityNameIdx = readU16(data, 1);
        m.seqNumber = readU16(data, 3);
        m.timestamp = readU64(data, 5);
        copyBytes(m.ticker, data, 13, 8);
        m.tradingState = readU8(data, 21);
        m.reserved = readU8(data, 22);
        copyBytes(m.reason, data, 23, 4);
        return m;
    }

    RegSHORestrictionMsg MmapReader::constructRegSHORestrictionMsg(const char* data) {
        RegSHORestrictionMsg m;
        m.msgType = readChar(data, 0);
        m.securityNameIdx = readU16(data, 1);
        m.seqNumber = readU16(data, 3);
        m.timestamp = readU64(data, 5);
        copyBytes(m.ticker, data, 13, 8);
        m.RegSHOAction = readU8(data, 21);
        return m;
    }

    MarketParticipantPositionMsg MmapReader::constructMarketParticipantPositionMsg(const char* data) {
        MarketParticipantPositionMsg m;
        m.msgType = readChar(data, 0);
        m.securityNameIdx = readU16(data, 1);
        m.seqNumber = readU16(data, 3);
        m.timestamp = readU64(data, 5);
        copyBytes(m.MPID, data, 13, 4);
        copyBytes(m.ticker, data, 17, 8);
        m.primaryMarketMaker = readU8(data, 25);
        m.marketMakerMode = readU8(data, 26);
        m.marketParticipantState = readU8(data, 27);
        return m;
    }

    MWCBDeclineLevelMsg MmapReader::constructMWCBDeclineLevelMsg(const char* data) {
        return {
            readChar(data, 0), readU16(data, 1), readU16(data, 3), readU64(data, 5), readU64(data, 13), readU64(data, 21), readU64(data, 29)
        };
    }

    MWCBStatusMsg MmapReader::constructMWCBStatusMsg(const char* data) {
        return {
            readChar(data, 0), readU16(data, 1), readU16(data, 3), readU64(data, 5), readU8(data, 13)
        };
    }

    IPOQuotingPeriodUpdateMsg MmapReader::constructIPOQuotingPeriodUpdateMsg(const char* data) {
        IPOQuotingPeriodUpdateMsg m;
        m.msgType = readChar(data, 0);
        m.securityNameIdx = readU16(data, 1);
        m.seqNumber = readU16(data, 3);
        m.timestamp = readU64(data, 5);
        copyBytes(m.ticker, data, 13, 8);
        m.ipoQuotationReleaseTime = readU32(data, 21);
        m.ipoQuotationReleaseQualifier = readU8(data, 25);
        m.ipoPrice = readU32(data, 26);
        return m;
    }

    LULDAuctionCollarMsg MmapReader::constructLULDAuctionCollarMsg(const char* data) {
        LULDAuctionCollarMsg m;
        m.msgType = readChar(data, 0);
        m.securityNameIdx = readU16(data, 1);
        m.seqNumber = readU16(data, 3);
        m.timestamp = readU64(data, 5);
        copyBytes(m.ticker, data, 13, 8);
        m.auctionCollarRefPrice = readU32(data, 21);
        m.upperAuctionCollarPrice = readU32(data, 25);
        m.lowerAuctionCollarPrice = readU32(data, 29);
        m.auctionCollarExtension = readU32(data, 33);
        return m;
    }

    OperationalHaltMsg MmapReader::constructOperationalHaltMsg(const char* data) {
        OperationalHaltMsg m;
        m.msgType = readChar(data, 0);
        m.securityNameIdx = readU16(data, 1);
        m.seqNumber = readU16(data, 3);
        m.timestamp = readU64(data, 5);
        copyBytes(m.ticker, data, 13, 8);
        m.marketCode = readU8(data, 21);
        m.operationalHaltAction = readU8(data, 22);
        return m;
    }

    AddOrderMsg MmapReader::constructAddOrderMsg(const char* d) {
        AddOrderMsg m{};
        m.msgType        = readChar(d, 0);
        m.securityNameIdx= readU16  (d, 1);
        m.seqNumber      = readU16  (d, 3);
        m.timestamp      = readU48  (d, 5);
        m.orderId        = readU64  (d,11);
        m.side           = readChar (d,19);
        m.quantity       = readU32  (d,20);
        copyBytes(m.ticker, d,24,8);
        m.price          = readU32  (d,32);
        return m;
    }

    
    AddOrderMPIDAttributionMsg MmapReader::constructAddOrderMPIDAttributionMsg(const char* d)
    {
        AddOrderMPIDAttributionMsg m{};
        m.msgType        = readChar(d, 0);
        m.securityNameIdx= readU16 (d, 1);
        m.seqNumber      = readU16 (d, 3);
        m.timestamp      = readU48 (d, 5);
        m.orderId        = readU64 (d,11);
        m.side           = readChar(d,19);
        m.quantity       = readU32 (d,20);
        copyBytes(m.ticker, d, 24, 8);
        m.price          = readU32 (d,32);
        copyBytes(m.MPID,  d, 36, 4);
        return m;
    }

    OrderExecutedMsg MmapReader::constructOrderExecutedMsg(const char* d)
    {
        OrderExecutedMsg m{};
        m.msgType        = readChar(d,0);
        m.securityNameIdx= readU16 (d,1);
        m.seqNumber      = readU16 (d,3);
        m.timestamp      = readU48 (d,5);
        m.orderId        = readU64 (d,11);
        m.executedQuantity = readU32(d,19);
        return m;
    }

    OrderExecutedWithPriceMsg
    MmapReader::constructOrderExecutedWithPriceMsg(const char* d)
    {
        OrderExecutedWithPriceMsg m{};
        m.msgType        = readChar(d,0);
        m.securityNameIdx= readU16 (d,1);
        m.seqNumber      = readU16 (d,3);
        m.timestamp      = readU48 (d,5);
        m.orderId        = readU64 (d,11);
        m.executedQuantity = readU32(d,19);
        m.executedPrice    = readU32(d,23);
        return m;
    }

    OrderCancelMsg MmapReader::constructOrderCancelMsg(const char* d)
    {
        OrderCancelMsg m{};
        m.msgType          = readChar(d,0);
        m.securityNameIdx  = readU16 (d,1);
        m.seqNumber        = readU16 (d,3);
        m.timestamp        = readU48 (d,5);
        m.orderId          = readU64 (d,11);
        m.cancelledQuantity= readU32 (d,19);
        return m;
    }


    OrderDeleteMsg MmapReader::constructOrderDeleteMsg(const char* d)
    {
        OrderDeleteMsg m{};
        m.msgType        = readChar(d,0);
        m.securityNameIdx= readU16 (d,1);
        m.seqNumber      = readU16 (d,3);
        m.timestamp      = readU48 (d,5);
        m.orderId        = readU64 (d,11);
        return m;
    }

    OrderReplaceMsg MmapReader::constructOrderReplaceMsg(const char* d)
    {
        OrderReplaceMsg m{};
        m.msgType        = readChar(d,0);
        m.securityNameIdx= readU16 (d,1);
        m.seqNumber      = readU16 (d,3);
        m.timestamp      = readU48 (d,5);
        m.ogOrderId      = readU64 (d,11);
        m.newOrderId     = readU64 (d,19);
        m.quantity       = readU32 (d,27);
        m.price          = readU32 (d,31);
        return m;
    }

    TradeMsg MmapReader::constructTradeMsg(const char* d)
    {
        TradeMsg m{};
        m.msgType        = readChar(d,0);
        m.securityNameIdx= readU16 (d,1);
        m.seqNumber      = readU16 (d,3);
        m.timestamp      = readU48 (d,5);
        m.orderId        = readU64 (d,11);
        m.side           = readChar(d,19);
        m.quantity       = readU32 (d,20);
        copyBytes(m.ticker, d,24,8);
        m.price          = readU32 (d,32);
        m.matchId        = readU64 (d,36);
        return m;
    }


    CrossTradeMsg MmapReader::constructCrossTradeMsg(const char* d){
        CrossTradeMsg m{};
        m.msgType        = readChar(d,0);
        m.securityNameIdx= readU16 (d,1);
        m.seqNumber      = readU16 (d,3);
        m.timestamp      = readU48 (d,5);
        m.quantity       = readU64 (d,11);
        copyBytes(m.ticker,d,19,8);
        m.crossPrice     = readU32 (d,27);
        m.matchId        = readU64 (d,31);
        return m;
    }

    BrokenTradeMsg MmapReader::constructBrokenTradeMsg(const char* d){
        BrokenTradeMsg m{};
        m.msgType        = readChar(d,0);
        m.securityNameIdx= readU16 (d,1);
        m.seqNumber      = readU16 (d,3);
        m.timestamp      = readU48 (d,5);
        m.matchId        = readU64 (d,11);
        return m;
    }

    NOIIMsg MmapReader::constructNOIIMsg(const char* data) {
        NOIIMsg m;
        m.msgType = readChar(data, 0);
        m.securityNameIdx = readU16(data, 1);
        m.seqNumber = readU16(data, 3);
        m.timestamp = readU64(data, 5);
        m.pairedShares = readU64(data, 13);
        m.imbalanceShares = readU64(data, 21);
        m.imbalanceDirection = readU8(data, 29);
        copyBytes(m.ticker, data, 30, 8);
        m.farPrice = readU32(data, 38);
        m.nearPrice = readU32(data, 42);
        m.currentRefPrice = readU32(data, 46);
        m.crossType = readU8(data, 50);
        m.priceVariationIndicator = readU8(data, 51);
        return m;
    }

    RetailInterestMsg MmapReader::constructRetailInterestMsg(const char* data) {
        RetailInterestMsg m;
        m.msgType = readChar(data, 0);
        m.securityNameIdx = readU16(data, 1);
        m.seqNumber = readU16(data, 3);
        m.timestamp = readU64(data, 5);
        copyBytes(m.ticker, data, 13, 8);
        m.interestFlag = readU8(data, 21);
        return m;
    }

    DirectListingWithCRPDMsg MmapReader::constructDirectListingWithCRPDMsg(const char* data) {
        DirectListingWithCRPDMsg m;
        m.msgType = readChar(data, 0);
        m.securityNameIdx = readU16(data, 1);
        m.seqNumber = readU16(data, 3);
        m.timestamp = readU64(data, 5);
        copyBytes(m.ticker, data, 13, 8);
        m.openEligibilityStatus = readU8(data, 21);
        m.minAllowablePrice = readU32(data, 22);
        m.maxAllowablePrice = readU32(data, 26);
        m.nearExecutionPrice = readU32(data, 30);
        m.nearExecutionTime = readU64(data, 34);
        m.lowerPriceRangeCollar = readU32(data, 42);
        m.upperPriceRangeCollar = readU32(data, 46);
        return m;
    }

    ts MmapReader::getDataTimestamp(const char* d){
        return readU48(d,5);
    }

    ts MmapReader::strToTimestamp(const char* timestampStr) {
        return std::strtoull(timestampStr, nullptr, 10);
    }

    
}