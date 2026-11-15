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
} // namespace cabinet_profile
