#pragma once

#include <cstddef>
#include <cstdint>

/*

    ITCH message identifier aliases and struct definitions.

*/

namespace ITCH {

    constexpr int MAX_ITCH_MSG_SIZE {64}; // For padding

    namespace Side {
        constexpr char BUY {'B'};
        constexpr char SELL {'S'};
    }

    // Wrapper for writing message of heterogenous size to a ring buffer

    struct AddOrderMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber; 
        uint64_t    timestamp;
        uint64_t    orderId;
        char        side;
        uint32_t    quantity;
        char        ticker[8]; 
        uint32_t    price;
    };

    struct AddOrderMPIDAttributionMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber; 
        uint64_t    timestamp;
        uint64_t    orderId;
        char        side;
        uint32_t    quantity;
        char        ticker[8]; 
        uint32_t    price;
        char MPID[4];
    };

    struct OrderExecutedMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber; 
        uint64_t    timestamp;
        uint64_t    orderId;
        uint32_t    executedQuantity;
    };

    struct OrderExecutedWithPriceMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber; 
        uint64_t    timestamp;
        uint64_t    orderId;
        uint32_t    executedQuantity;
        uint32_t    executedPrice;
    };

    struct OrderCancelMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber; 
        uint64_t    timestamp;
        uint64_t    orderId;
        uint32_t    cancelledQuantity;
    };

    struct OrderDeleteMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber; 
        uint64_t    timestamp;
        uint64_t    orderId;
    };

    struct OrderReplaceMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber;
        uint64_t    timestamp;
        uint64_t    ogOrderId;
        uint64_t    newOrderId;
        uint32_t    quantity;
        uint32_t    price;
    };

    struct TradeMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber; 
        uint64_t    timestamp;
        uint64_t    orderId;
        char        side;
        uint32_t    quantity;
        char        ticker[8];
        uint32_t    price;
        uint64_t    matchId; 
    };

    struct CrossTradeMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber;   
        uint64_t    timestamp;
        uint64_t    quantity;
        char        ticker[8];
        uint32_t    crossPrice;
        uint64_t    matchId;
    };

    struct BrokenTradeMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber;  
        uint64_t    timestamp;
        uint64_t    matchId;
    };

    struct SystemEventMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber;
        uint64_t    timestamp;
        uint8_t     eventCode;
    };

    struct StockDirectoryMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber;
        uint64_t    timestamp;
        char        ticker[8];
        uint8_t     marketCategory;
        uint8_t     financialStatusIndicator;
        uint32_t    roundLotSize;
        uint8_t     roundLotsOnly;
        uint8_t     securityClass; // 'A' = ETF, 'E' = Common Equity
        uint8_t     issueSubType[2]; // Further detail on security
        uint8_t     authenticity;
        uint8_t     shortSaleThresholdIndicator;
        uint8_t     IPOFlag; // Is this security an IPO
        uint8_t     LULDReferencePriceTier;
        uint8_t     ETPFlag; // Exchange Traded Product
        uint32_t    ETPLeverageFactor;
        uint8_t     inverseIndicator; // Inverse ETF indicator
    };

    struct StockTradingActionMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber;
        uint64_t    timestamp;
        char        ticker[8];
        uint8_t     tradingState; // Halted, Paused, etc
        uint8_t     reserved; // Unused
        uint8_t     reason[4];
    };

    struct RegSHORestrictionMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber;
        uint64_t    timestamp;
        char        ticker[8];
        uint8_t     RegSHOAction; // unrestricted, restricted, lifted
    };

    struct MarketParticipantPositionMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber;
        uint64_t    timestamp;
        uint8_t     MPID[4];
        char        ticker[8];
        uint8_t     primaryMarketMaker; // Y, N
        uint8_t     marketMakerMode; // Normal, Passive, Supplemental
        uint8_t     marketParticipantState; // Active, Excused, Withdrawn
    };

    struct MWCBDeclineLevelMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber;
        uint64_t    timestamp;
        uint64_t    level1; // 6%
        uint64_t    level2; // 13%
        uint64_t    level3; // 20%
    };

    struct MWCBStatusMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber;
        uint64_t    timestamp;
        uint8_t     breachedLevel;
    };

    struct IPOQuotingPeriodUpdateMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber;
        uint64_t    timestamp;
        char        ticker[8];
        uint32_t    ipoQuotationReleaseTime;
        uint8_t     ipoQuotationReleaseQualifier;
        uint32_t    ipoPrice;
    };

    struct LULDAuctionCollarMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber;
        uint64_t    timestamp;
        char        ticker[8];
        uint32_t    auctionCollarRefPrice;
        uint32_t    upperAuctionCollarPrice;
        uint32_t    lowerAuctionCollarPrice;
        uint32_t    auctionCollarExtension;
    };

    struct OperationalHaltMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber;
        uint64_t    timestamp;
        char        ticker[8];
        uint8_t     marketCode;
        uint8_t     operationalHaltAction;
    };

    struct NOIIMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber;
        uint64_t    timestamp;
        uint64_t    pairedShares;
        uint64_t    imbalanceShares;
        uint8_t     imbalanceDirection;
        char        ticker[8];
        uint32_t    farPrice;
        uint32_t    nearPrice;
        uint32_t    currentRefPrice;
        uint8_t     crossType;
        uint8_t     priceVariationIndicator;
    };

    struct RetailInterestMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber;
        uint64_t    timestamp;
        char        ticker[8];
        uint8_t     interestFlag;
    };

    struct DirectListingWithCRPDMsg {
        char        msgType;
        uint16_t    securityNameIdx;
        uint16_t    seqNumber;
        uint64_t    timestamp;
        char        ticker[8];
        uint8_t     openEligibilityStatus;
        uint32_t    minAllowablePrice;
        uint32_t    maxAllowablePrice;
        uint32_t    nearExecutionPrice;
        uint64_t    nearExecutionTime;
        uint32_t    lowerPriceRangeCollar;
        uint32_t    upperPriceRangeCollar;
    };

    using msg_type = char;
    using ts       = uint64_t;

    // Message type aliasing for clarity

    constexpr msg_type SystemEventMsgType                              {'S'};
    constexpr msg_type StockDirectoryMsgType                           {'R'};
    constexpr msg_type StockTradingActionMsgType                       {'H'};
    constexpr msg_type RegSHORestrictionMsgType                        {'Y'};
    constexpr msg_type MarketParticipantPositionMsgType                {'L'};
    constexpr msg_type MWCBDeclineLevelMsgType                         {'V'};
    constexpr msg_type MWCBStatusMsgType                               {'W'};
    constexpr msg_type IPOQuotingPeriodUpdateMsgType                   {'K'};
    constexpr msg_type LULDAuctionCollarMsgType                        {'J'};
    constexpr msg_type OperationalHaltMsgType                          {'h'};
    constexpr msg_type AddOrderMsgType                                 {'A'};
    constexpr msg_type AddOrderMPIDAttributionMsgType                  {'F'};
    constexpr msg_type OrderExecutedMsgType                            {'E'};
    constexpr msg_type OrderExecutedWithPriceMsgType                   {'C'};
    constexpr msg_type OrderCancelMsgType                              {'X'};
    constexpr msg_type OrderDeleteMsgType                              {'D'};
    constexpr msg_type OrderReplaceMsgType                             {'U'};
    constexpr msg_type TradeMsgType                                    {'P'};
    constexpr msg_type CrossTradeMsgType                               {'Q'};
    constexpr msg_type BrokenTradeMsgType                              {'B'};
    constexpr msg_type NOIIMessageMsgType                              {'I'};
    constexpr msg_type RetailInterestMsgType                           {'N'};
    constexpr msg_type DirectListingWithCRPDMsgType                    {'O'};

    // For clarity in logging/printing

    template<char msg_type> constexpr char* MsgName = msg_type + " - ";

    template <>
    constexpr char const *MsgName<SystemEventMsgType> = "SystemEvent";
    template <>
    constexpr char const *MsgName<StockDirectoryMsgType> = "StockDirectory";
    template <>
    constexpr char const *MsgName<StockTradingActionMsgType> = "StockTradingAction";
    template <>
    constexpr char const *MsgName<RegSHORestrictionMsgType> = "RegSHORestriction";
    template <>
    constexpr char const *MsgName<MarketParticipantPositionMsgType> = "MarketParticipantPosition";
    template <>
    constexpr char const *MsgName<MWCBDeclineLevelMsgType> = "MWCBDeclineLevel";
    template <>
    constexpr char const *MsgName<MWCBStatusMsgType> = "MWCBStatus";
    template <>
    constexpr char const *MsgName<IPOQuotingPeriodUpdateMsgType> = "IPOQuotingPeriodUpdate";
    template <>
    constexpr char const *MsgName<LULDAuctionCollarMsgType> = "LULDAuctionCollar";
    template <>
    constexpr char const *MsgName<OperationalHaltMsgType> = "OperationalHalt";
    template <>
    constexpr char const *MsgName<AddOrderMsgType> = "AddOrder";
    template <>
    constexpr char const *MsgName<AddOrderMPIDAttributionMsgType> = "AddOrderWithMPID";
    template <>
    constexpr char const *MsgName<OrderExecutedMsgType> = "OrderExecuted";
    template <>
    constexpr char const *MsgName<OrderExecutedWithPriceMsgType> = "OrderExecutedWithPrice";
    template <>
    constexpr char const *MsgName<OrderCancelMsgType> = "OrderCancel";
    template <>
    constexpr char const *MsgName<OrderDeleteMsgType> = "OrderDelete";
    template <>
    constexpr char const *MsgName<OrderReplaceMsgType> = "OrderReplace";
    template <>
    constexpr char const *MsgName<TradeMsgType> = "Trade";
    template <>
    constexpr char const *MsgName<CrossTradeMsgType> = "CrossTrade";
    template <>
    constexpr char const *MsgName<BrokenTradeMsgType> = "BrokenTrade";
    template <>
    constexpr char const *MsgName<NOIIMessageMsgType> = "NetOrderImbalanceIndicator";
    template <>
    constexpr char const *MsgName<RetailInterestMsgType> = "RetailInterest";
    template <>
    constexpr char const *MsgName<DirectListingWithCRPDMsgType> = "DirectListingPriceDiscovery";

    template<char msg_type> constexpr uint16_t MsgSize = -1;

    template <> constexpr uint16_t MsgSize<AddOrderMsgType>                = 36;
    template <> constexpr uint16_t MsgSize<AddOrderMPIDAttributionMsgType> = 40;

    template <> constexpr uint16_t MsgSize<OrderExecutedMsgType>           = 23;
    template <> constexpr uint16_t MsgSize<OrderExecutedWithPriceMsgType>  = 27;

    template <> constexpr uint16_t MsgSize<OrderCancelMsgType>             = 23;
    template <> constexpr uint16_t MsgSize<OrderDeleteMsgType>             = 19;
    template <> constexpr uint16_t MsgSize<OrderReplaceMsgType>            = 35;

    template <> constexpr uint16_t MsgSize<TradeMsgType>                   = 44;

    template <> constexpr uint16_t MsgSize<CrossTradeMsgType>              = 39;
    template <> constexpr uint16_t MsgSize<BrokenTradeMsgType>             = 19;

    template <> constexpr uint16_t MsgSize<SystemEventMsgType>             = 12;
    template <> constexpr uint16_t MsgSize<StockDirectoryMsgType>          = 39;
    template <> constexpr uint16_t MsgSize<StockTradingActionMsgType>      = 25;
    template <> constexpr uint16_t MsgSize<RegSHORestrictionMsgType>       = 20;
    template <> constexpr uint16_t MsgSize<MarketParticipantPositionMsgType> = 26;

    template <> constexpr uint16_t MsgSize<MWCBDeclineLevelMsgType>        = 35;
    template <> constexpr uint16_t MsgSize<MWCBStatusMsgType>              = 12;

    template <> constexpr uint16_t MsgSize<IPOQuotingPeriodUpdateMsgType>  = 28;
    template <> constexpr uint16_t MsgSize<LULDAuctionCollarMsgType>       = 35;
    template <> constexpr uint16_t MsgSize<OperationalHaltMsgType>         = 21;

    template <> constexpr uint16_t MsgSize<NOIIMessageMsgType>             = 50;
    template <> constexpr uint16_t MsgSize<RetailInterestMsgType>          = 20;
    template <> constexpr uint16_t MsgSize<DirectListingWithCRPDMsgType>   = 48;

}