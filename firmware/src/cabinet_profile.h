#pragma once
#include <cstddef>
#include <cstdint>

// One recorded Confucius Say cabinet at Palms: mBcjPK63KW4.
// These are observed controls, not a recovered payset or probability table.
namespace cabinet_profile {
struct BetRow { const uint8_t* multipliers; unsigned count; };
struct Coin { int64_t minor; const uint8_t* lines; const BetRow* rows; unsigned count; };
constexpr int64_t denominations[]={1,2,5,10,100,200};
constexpr uint8_t standardBets[]={1,2,3,5,10},oneThroughFiveBets[]={1,2,3,4,5};
constexpr uint8_t pennyLines[]={50,100},nickelLines[]={25,50},dollarLines[]={5,10},twoDollarLines[]={5};
constexpr BetRow standardRows[]={{standardBets,5},{standardBets,5}};
// The Palms Confucius chooser shows PLAY5/10 at$1. For PLAY10,
// use the directly filmed PhoenixLink SantaFe GeneralTso1..5 row (G4b3tO8M6Ns).
// Shared-family UI adaptation; target-theme selected-row confirmation is pending.
constexpr BetRow dollarRows[]={{standardBets,5},{oneThroughFiveBets,5}};
constexpr BetRow twoDollarRows[]={{oneThroughFiveBets,5}};
constexpr Coin coins[]={
 {1,pennyLines,standardRows,2},{2,pennyLines,standardRows,2},
 {5,nickelLines,standardRows,2},{10,nickelLines,standardRows,2},
 {100,dollarLines,dollarRows,2},{200,twoDollarLines,twoDollarRows,1}
};
constexpr unsigned denominationCount=sizeof(coins)/sizeof(coins[0]);
inline const Coin* find(int64_t minor){for(const auto& coin:coins)if(coin.minor==minor)return &coin;return nullptr;}
inline unsigned lineIndex(const Coin& coin,uint8_t lines){for(unsigned i=0;i<coin.count;++i)if(coin.lines[i]==lines)return i;return coin.count;}
inline const BetRow* row(int64_t minor,uint8_t lines){const auto* coin=find(minor);if(!coin)return nullptr;const auto index=lineIndex(*coin,lines);return index<coin->count?&coin->rows[index]:nullptr;}
inline bool offered(int64_t minor,uint8_t lines,uint8_t multiplier){const auto* offer=row(minor,lines);if(!offer)return false;for(unsigned i=0;i<offer->count;++i)if(offer->multipliers[i]==multiplier)return true;return false;}
// FbBUqjjzN08 directly shows all seven dollar denominations and25-line play.
// Multipliers1/2/3 are witnessed at$100/$100/$50 respectively. Their shared
// row below is a local practice UI adaptation, not a recovered factory deck.
// Other high-limit line choices, multipliers and machine caps remain unrecovered.
// 5BUJOWxIBN4 directly shows Confucius Say $5 / 50 lines / multiplier3
// ($750). Expose that observed selection only; do not extend50 lines to
// other denominations or assume an unfilmed50-line multiplier deck.
constexpr int64_t highLimitDenominations[]={100,200,500,1000,2500,5000,10000};
constexpr unsigned highLimitDenominationCount=7;
constexpr uint8_t highLimitLines[]={25},highLimitBets[]={1,2,3};
constexpr BetRow highLimitRows[]={{highLimitBets,3}};
constexpr uint8_t fiveDollarHighLimitLines[]={25,50},observedFiveDollarFiftyBets[]={3};
constexpr BetRow fiveDollarHighLimitRows[]={{highLimitBets,3},{observedFiveDollarFiftyBets,1}};
constexpr Coin highLimitCoins[]={
 {100,highLimitLines,highLimitRows,1},{200,highLimitLines,highLimitRows,1},
 {500,fiveDollarHighLimitLines,fiveDollarHighLimitRows,2},{1000,highLimitLines,highLimitRows,1},
 {2500,highLimitLines,highLimitRows,1},{5000,highLimitLines,highLimitRows,1},
 {10000,highLimitLines,highLimitRows,1}
};
} // namespace cabinet_profile
