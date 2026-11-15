#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <array>
#include <limits>

// Independent, offline recreation of the documented Phoenix Link presentation.
// ALL probabilities, reel weights, line paths and payouts below are LOCAL model
// assumptions unless explicitly marked observed. They are not Aristocrat PAR
// data, certified RTP, or a claim of identical casino odds. No network is used.
namespace phoenix {

enum class Symbol : uint8_t {
  Nine, Ten, Jack, Queen, King, Bell, Abacus, Book, Candle,
  Wild, Scatter, Phoenix, Count
};
enum class Mode : uint8_t { Base, Free, Hold };
enum class PrizeKind : uint8_t { Credits, Mini, Minor, Major };
enum class PrizeCapProfile : uint8_t { HighCashOnly, NamedOrHighCash };
// Capacity results do not alter the financial snapshot or its schema/model.
// A rejected action consumes no random word and changes no persisted field.
enum CapacityFaultBit : uint32_t {
  CapacityBalance=1u<<0, CapacityRoundPayout=1u<<1, CapacityFeatureWin=1u<<2,
  CapacityLifetimePaid=1u<<3, CapacityActions=1u<<4, CapacityHoldAwards=1u<<5,
  CapacityGrandAwards=1u<<6, CapacityPaidHits=1u<<7, CapacityMeterAwards=1u<<8,
  CapacityLifetimeStaked=1u<<9, CapacityPaidRounds=1u<<10, CapacityRounds=1u<<11,
  CapacityFreeSpins=1u<<12, CapacityFreeAwards=1u<<13,
  CapacityLuckyAttempts=1u<<14, CapacityLuckyWins=1u<<15
};
constexpr uint32_t kCapacityKnownFaults=0xffffu;

constexpr size_t kColumns = 5, kRows = 3, kCells = 15, kMaxLines = 100;
constexpr size_t cellIndex(size_t column, size_t row) { return column * kRows + row; }

struct Cell {
  Symbol symbol = Symbol::Nine;
  PrizeKind kind = PrizeKind::Credits;
  uint8_t multiplier = 1;
  bool highlight = false;
  // A Phoenix's unmultiplied credit value. For named prizes this is the locked
  // jackpot value at landing. Ordinary symbols always carry zero prize.
  int64_t prize = 0;
};

namespace local {
// Version identifies this local, deliberately approximate math configuration.
constexpr uint32_t modelId = 0x00020011u;
// Scoped H revision: exact WO10c/25/M1 future Base prices only.
constexpr uint32_t previousGPriceModelId = 0x00020010u;
// Scoped G revision: source68 exact-profile future Base prices only.
constexpr uint32_t previousFPriceModelId = 0x0002000fu;
// Private F candidate: five future Base cells; E/D owed bodies stay exact.
constexpr uint32_t previousEPriceModelId = 0x0002000eu;
// Future source-caption Jack4/King3 prices; imported D bodies stay exact.
constexpr uint32_t previousPlazaPriceModelId = 0x0002000du;
// Future scoped Jack3/Abacus5 price revision; C/B bodies stay exact.
constexpr uint32_t previousBellPriceModelId = 0x0002000cu;
// Future scoped2c/100/M3 Bell2 price; importedB bodies and prizes stay exact.
constexpr uint32_t previousVegasPriceModelId = 0x0002000bu;
// Future source-scoped Vegas Base prices; importedA bodies stay exact.
constexpr uint32_t previousSevenPriceModelId = 0x0002000au;
// Future observed seven-price batch; imported09 bodies and awards stay exact.
constexpr uint32_t previousContextualPayModelId = 0x00020009u;
// Future observed Base4Jack price changes; imported schema4 bodies stay exact.
constexpr uint32_t previousJackPayModelId = 0x00020008u;
// Future observed25-line paths/three scoped pays change; saved bodies stay exact.
constexpr uint32_t previousPaylineModelId = 0x00020007u;
// Future observed named values change; imported schema4 bodies remain exact.
constexpr uint32_t previousNamedProfileModelId = 0x00020006u;
// Future Free3 Wisdom/Abacus pays changed; committed schema4 bodies stay exact.
constexpr uint32_t previousFreePayModelId = 0x00020005u;
constexpr uint32_t precedingObservedPayModelId = 0x00020004u;
constexpr uint32_t olderObservedPayModelId = 0x00020003u;
constexpr uint32_t previousSchema4ModelId = 0x00020002u;
constexpr uint32_t olderSchema4ModelId = 0x00020001u;
constexpr uint32_t firstSchema4ModelId = 0x00020000u;
constexpr uint32_t legacyModelId = 0x00010004u;
constexpr size_t legacySaveBytes = 1874;
constexpr int64_t maximumBankMinor = 1000000000000000LL;  // external play-money funding limit.
// Earned/committed awards need not fit the external funding limit. Wire money
// fields have always been signed64; accepting their full nonnegative range is
// lossless for every old bank and also recovers old over-funding-limit awards.
constexpr int64_t maximumStoredMinor = std::numeric_limits<int64_t>::max();
constexpr int64_t maximumCreditPrize = 3000000000LL;  // permits lossless old3x-credit prize migration.
constexpr int64_t newBankMinor = 100000;  // $1,000.00 play bank.
constexpr int64_t denominations[] = {1, 2, 5, 10, 25, 50, 100, 200, 500, 1000};
constexpr size_t denominationCount = sizeof(denominations) / sizeof(denominations[0]);
constexpr uint8_t lineOptions[] = {5,10,15,25,50,100};
constexpr uint16_t meterBetCredits[] = {5,10,15,20,25,30,35,40,45,50,60,70,75,80,90,100,105,120,125,135,150,175,200,225,250,300,350,400,450,500,600,700,800,900,1000};
constexpr uint16_t legacyMeterBetCredits[] = {5,10,15,20,25,30,35,40,45,50,60,70,75,80,90,100,125,150,175,200,225,250,300,350,400,450,500,600,700,800,900,1000};
constexpr size_t legacyMeterBetCount = sizeof(legacyMeterBetCredits)/sizeof(legacyMeterBetCredits[0]);
constexpr size_t meterBetCount = sizeof(meterBetCredits) / sizeof(meterBetCredits[0]);
constexpr size_t meterBucketCount = denominationCount * meterBetCount;
constexpr uint8_t betMultipliers[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// OBSERVED configurations: supplied cabinet view is 25 lines at 10 cents;
// Confucius video 4:22 is 5 lines at $1, multiplier 2 ($10 total). The exact
// denomination/line-selection mapping is unknown. Additional observed family
// configurations are 50/100 lines at1c (hmy/BJ4), with corresponding integer
// wagers. Menu presets do not imply every casino offers all configurations.
struct Wager { int64_t denomination; uint8_t lines, multiplier; };
constexpr Wager wagers[] = {
  {100,5,2}, {100,5,1}, {100,5,3}, {100,5,5}, {100,5,10},
  {10,25,1}, {10,25,2}, {10,25,3}, {10,25,5}, {10,25,7}, {10,25,10},
  {1,50,1}, {1,50,2}, {1,50,3}, {1,50,5}, {1,50,10},
  {1,100,1}, {1,100,2}, {1,100,3}, {1,100,5}, {1,100,10},
  {2,50,1}, {2,50,2}, {2,50,3}, {2,50,5}, {2,50,10},
  {5,50,1}, {5,50,2}, {5,50,3}, {5,50,5}, {5,50,10},
  {10,50,1}, {10,50,2}, {10,50,5}, {100,10,1}, {100,10,2},
  {200,5,1}, {200,5,2}, {500,5,1}, {1000,5,1},
  // Observed BigPayback25c/15line deck; append to retain old preset indices.
  {25,15,1}, {25,15,2}, {25,15,3}, {25,15,5}, {25,15,10},
  {50,25,1}, {50,25,2}, {50,25,3}, {50,25,5}, {50,25,10}
};

// LOCAL fallback paths, retained exactly for5/10/15/50/100-line selections.
// The separately observed US25-line chart below overrides only count25. Rows are
// top=0, middle=1, bottom=2; wins pay consecutively from the leftmost reel.
constexpr uint8_t firstLines[25][kColumns] = {
  {1,1,1,1,1}, {0,0,0,0,0}, {2,2,2,2,2}, {0,1,2,1,0}, {2,1,0,1,2},
  {0,0,1,2,2}, {2,2,1,0,0}, {1,0,0,0,1}, {1,2,2,2,1}, {0,1,1,1,0},
  {2,1,1,1,2}, {1,0,1,2,1}, {1,2,1,0,1}, {0,1,0,1,0}, {2,1,2,1,2},
  {1,1,0,1,1}, {1,1,2,1,1}, {0,0,2,0,0}, {2,2,0,2,2}, {0,2,2,2,0},
  {2,0,0,0,2}, {1,0,2,0,1}, {1,2,0,2,1}, {0,2,0,2,0}, {2,0,2,0,2}
};
// Supplemental LOCAL paths are unique base-three row patterns. No50/100-line
// diagrams have been recovered; their existence supports observed50/100-line
// menus without falsely paying 243 ways or duplicating the first25 lines.
constexpr std::array<std::array<uint8_t,kColumns>,kMaxLines> makeLines() {
  std::array<std::array<uint8_t,kColumns>,kMaxLines> result{};
  size_t used = 25;
  for (size_t i = 0; i < 25; ++i) for (size_t c = 0; c < kColumns; ++c) result[i][c] = firstLines[i][c];
  for (unsigned code = 0; code < 243 && used < kMaxLines; ++code) {
    std::array<uint8_t,kColumns> candidate{};
    unsigned value = code;
    for (size_t c = 0; c < kColumns; ++c) { candidate[c] = uint8_t(value % 3); value /= 3; }
    bool duplicate = false;
    for (size_t i = 0; i < used; ++i) {
      bool same = true;
      for (size_t c = 0; c < kColumns; ++c) if (result[i][c] != candidate[c]) same = false;
      duplicate |= same;
    }
    if (!duplicate) result[used++] = candidate;
  }
  return result;
}
inline constexpr auto lines = makeLines();
inline constexpr std::array<uint8_t,kColumns> observedFifteenLine6 = {0,0,1,0,0};
// OBSERVED original US Confucius Say25-line numbered divider/end markers,
// YfbaaFXhln0 (50c/25lines/mult1),57.5..1901.8s; all25 numbered paths.
// Only25-line selection changes; other local line profiles remain untouched.
// This is not a recovered50/100 chart or a universal regional paytable.
inline constexpr std::array<std::array<uint8_t,kColumns>,25> observedTwentyFiveLines = {{
  {{1,1,1,1,1}}, {{0,0,0,0,0}}, {{2,2,2,2,2}}, {{0,1,2,1,0}}, {{2,1,0,1,2}},
  {{0,0,1,0,0}}, {{2,2,1,2,2}}, {{1,2,2,2,1}}, {{1,0,0,0,1}}, {{0,1,1,1,0}},
  {{2,1,1,1,2}}, {{0,1,0,1,0}}, {{2,1,2,1,2}}, {{1,0,1,0,1}}, {{1,2,1,2,1}},
  {{1,1,0,1,1}}, {{1,1,2,1,1}}, {{0,2,0,2,0}}, {{2,0,2,0,2}}, {{1,0,2,0,1}},
  {{1,2,0,2,1}}, {{0,0,2,0,0}}, {{2,2,0,2,2}}, {{0,2,2,2,0}}, {{2,0,0,0,2}}
}};
// Direct numbered markers in BigPayback Confucius25c/15line:
// Line6=[top,top,middle,top,top]; Line8=[middle,bottom,bottom,bottom,middle].
// Keep other configurations untouched. Line9 retains the prior LOCAL upper
// mirror while8uses its observed lower path;9is not a source-confirmed path.
inline const std::array<uint8_t,kColumns>& linePath(uint8_t line,uint8_t count=kMaxLines) {
  if(line>=kMaxLines)return lines[0];
  if(count==15){
    if(line==5)return observedFifteenLine6;
    if(line==7)return lines[8];
    if(line==8)return lines[7];
  }
  if(count==25&&line<observedTwentyFiveLines.size())return observedTwentyFiveLines[line];
  return lines[line];
}

// LOCAL credit pays per one credit bet per line. Exact target paytable remains
// unknown. Order follows the visually confirmed symbol inventory.
constexpr uint16_t basePays[10][6] = {
  {0,0,0,5,15,40}, {0,0,0,5,10,40}, {0,0,0,8,20,50},
  {0,0,0,5,20,50}, {0,0,0,10,25,60},
  {0,0,0,10,50,100}, {0,0,0,5,30,150},
  {0,0,0,5,25,250}, {0,0,0,5,25,400}, {0,0,0,50,150,400}
};
// Observed US base constraints, not recovered whole tables or probabilities:
// HzpqxpE-Ht8 source308s:$2/5lines/mult1,2Candle=2 (one line, final$4).
// source412s:10c/25lines/mult3,2Bell=2 (Line2wins6; total$1.80).
// Same cabinet5c/25lines/mult5:3Bell10,4Ten10,4Abacus30,3Candle5,
// 4Candle25,4Book25. Remaining cells and extrapolation to unfilmed presets
// stay local. Two-symbol pays are configuration-scoped below: source119/121
// explicitly leaves the25-line Candle/Sage pair unpaid. Free2-symbol awards
// have not been recovered and are unchanged.
inline constexpr uint16_t basePay(unsigned symbol,unsigned count,unsigned lines){
  if(symbol>=10||count>5)return 0;
  if(count==2){
    if(lines==5&&symbol==unsigned(Symbol::Candle))return 2;
    if(lines==25&&symbol==unsigned(Symbol::Bell))return 2;
  }
  return basePays[symbol][count];
}
// Observed base constraints:3Book=5,4Bell=50,3Ten=5,3Abacus=5,3Queen=5.
// MQt local151.5 Free Line2 CandleCandleCandleKingKing pays5; local145.5 Free Line1 KingKingKingBellBell pays5.
// Abacus: BigPayback Confucius25c/15line, Line6wins25 at multiplier5,
// rules/observed-math-frames/bell-cycle/002.jpg. Applying this best observed
// Base value to the local presets does not prove it for every casino variant.
// Other entries
// above remain LOCAL. The operator specifies a SEPARATE Free paytable; its
// numerical values have not been recovered, so this distinct table is LOCAL.
// Observed mQZbc-_wjO4 cut130s Free2of6: Line4 Abacus/Abacus/Abacus/Q/Q
// wins5 at multiplier1. Only Free3Abacus20->5 is revised below.
constexpr uint16_t freePays[10][6] = {
  {0,0,0,5,15,40}, {0,0,0,5,15,40}, {0,0,0,8,20,50},
  {0,0,0,8,20,50}, {0,0,0,5,25,60},
  {0,0,0,15,40,100}, {0,0,0,5,60,150},
  {0,0,0,30,100,250}, {0,0,0,5,150,400}, {0,0,0,50,150,400}
};
// Future evaluation only, matching observed US50c/25lines/mult1 profile.
// Yfbaa649..650s: literal Base3King wins5 ($2.50);20.5..23s:
// Base5Candle wins100, plus3Candle5 =>105credits ($52.50). Free6of6
// 91..97s: three5King lines*50 +twelve3Abacus*5 =210credits ($105).
// Other counts/ranks/profiles retain the prior local values. No saved award
// or locked Cell is repriced. denominationMinor=0 keeps unscoped inspection
// calls on the unchanged fallback pays; live settleSpin passes the real coin.
inline constexpr uint16_t linePay(unsigned symbol,unsigned count,unsigned multiplier,bool free,unsigned lineCount,int64_t denominationMinor){
  // Source82/83 wO4wC7v3Ouw: Confucius Say paid Base10c/25/M1.
  // Native8724 Line11 King3=5; native9090/9150 Line22 Book5=100.
  // Exact filmed profile, future evaluations only. Free/paths/saved awards unchanged.
  if(!free&&denominationMinor==10&&lineCount==25&&multiplier==1){
    if(symbol==unsigned(Symbol::King)&&count==3)return 5;
    if(symbol==unsigned(Symbol::Book)&&count==5)return 100;
  }

  // Source68 3pfZnZUWGhI: Confucius Say paid Base2c/50/M3.
  // Line3/7/1 Bell-prefix2 wins6 credits; Line3/36 King3 wins15.
  // Future exact-profile prices only; no Free, route or saved-award change.
  if(!free&&denominationMinor==2&&lineCount==50&&multiplier==3){
    if(symbol==unsigned(Symbol::Bell)&&count==2)return 2;
    if(symbol==unsigned(Symbol::King)&&count==3)return 5;
  }

  if(symbol>=10||count>5)return 0;
  // RESEARCH CANDIDATE ONLY: wave31 directly filmed Base prices.
  // Rio-declared GclIM2tutCg 1c/50/M10: RIO13 Line3 Bell2=20;
  // RIO20 Lines13/38 Jack3=50 credits. Exact filmed profile only.
  if(!free&&denominationMinor==1&&lineCount==50&&multiplier==10){
    if(symbol==unsigned(Symbol::Bell)&&count==2)return 2;
    if(symbol==unsigned(Symbol::Jack)&&count==3)return 5;
  }
  // SlotBears Zfc-LBjwXQA 10c/100/M1: Bell2=2, Jack3=5,
  // Queen4=10 credits. No route/probability/Grand or saved-award edits.
  if(!free&&denominationMinor==10&&lineCount==100&&multiplier==1){
    if(symbol==unsigned(Symbol::Bell)&&count==2)return 2;
    if(symbol==unsigned(Symbol::Jack)&&count==3)return 5;
    if(symbol==unsigned(Symbol::Queen)&&count==4)return 10;
  }
  // Original Confucius, Plaza HzpqxpE-Ht8 f4128 (172s): Base Line3
  // J/J/Wild then Phoenix pays25credits at5c/25lines/M5 ($1.25).
  // Scope only this observed profile; no Free/off-profile extrapolation.
  if(!free&&denominationMinor==5&&lineCount==25&&multiplier==5&&
     symbol==unsigned(Symbol::Jack)&&count==3)return 5;
  // Original Confucius, Cannery YoyplGW4DPM, 1c/100lines/M5:
  // f25490 Line6 King3=25; f25730 Line1 Jack3=25;
  // f26915 Line2 Bell2=10 credits. Per-line prices are directly visible;
  // LOCAL100-line paths remain unchanged and are not source-validated.
  if(!free&&denominationMinor==1&&lineCount==100&&multiplier==5){
    if(count==3&&(symbol==unsigned(Symbol::King)||symbol==unsigned(Symbol::Jack)))return 5;
    if(count==2&&symbol==unsigned(Symbol::Bell))return 2;
  }
  // Original Confucius, Cannery YoyplGW4DPM f29765: Base Line8
  // Bell-prefix2 wins6credits at2c/100lines/M3. f29870 Line15 also
  // says6, but its original route is unresolved; no LOCAL path is changed.
  // Future Base evaluations only; do not reprice a saved/committed award.
  if(!free&&denominationMinor==2&&lineCount==100&&multiplier==3&&
     symbol==unsigned(Symbol::Bell)&&count==2)return 2;
  // Original Confucius, Cannery YoyplGW4DPM: f30525/30180/30575/30215
  // Base Jack3 wins15credits, and f29360 Abacus5 wins300 at2c/100/M3.
  // These are future prices only. Keep every LOCAL100-line path/ID and
  // every imported committed award, cached prize, meter and PCG byte exact.
  if(!free&&denominationMinor==2&&lineCount==100&&multiplier==3){
    if(symbol==unsigned(Symbol::Jack)&&count==3)return 5;
    if(symbol==unsigned(Symbol::Abacus)&&count==5)return 100;
  }
  // Source-caption correction: scoped to this observed Base configuration.
  // Original Confucius, Plaza HzpqxpE-Ht8: f5112 Line7 wins30 on four
  // Jacks at10c/25/M3; f5280 Line1 wins15 on King/Wild/King. Unit10/5.
  // Future Base evaluation only; no Free/off-profile or committed-price changes.
  if(!free&&denominationMinor==10&&lineCount==25&&multiplier==3){
    if(symbol==unsigned(Symbol::Jack)&&count==4)return 10;
    if(symbol==unsigned(Symbol::King)&&count==3)return 5;
  }
  if(denominationMinor==50&&lineCount==25&&multiplier==1){
    // Yfbaa US50c/25line/1x: BaseQ4=10 (822.5s); FreeBell3=10
    // (719.5s), Book3=5 (530s), Jack3=5 (203.5s), Ten5=35
    // (704.5s), Candle5=70 (713.5s). FreeJack5=35 is independently
    // forced by both530s and713.5s full-board/cash equations. Future
    // line evaluation only; no imported/committed award is repriced.
    if(!free&&symbol==unsigned(Symbol::Queen)&&count==4)return 10;
    if(free){
      if(symbol==unsigned(Symbol::Bell)&&count==3)return 10;
      if(symbol==unsigned(Symbol::Book)&&count==3)return 5;
      if(symbol==unsigned(Symbol::Jack)&&count==3)return 5;
      if(symbol==unsigned(Symbol::Jack)&&count==5)return 35;
      if(symbol==unsigned(Symbol::Ten)&&count==5)return 35;
      if(symbol==unsigned(Symbol::Candle)&&count==5)return 70;
    }
    // Yfbaa474.8/982.5s: Base4Jack wins10credits; each closed cashwin$5.
    if(!free&&symbol==unsigned(Symbol::Jack)&&count==4)return 10;
    if(!free&&symbol==unsigned(Symbol::King)&&count==3)return 5;
    if(!free&&symbol==unsigned(Symbol::Candle)&&count==5)return 100;
    if(free&&symbol==unsigned(Symbol::King)&&count==5)return 50;
  }
  return free?freePays[symbol][count]:basePay(symbol,count,lineCount);
}
static constexpr auto& pays = basePays;  // compatibility alias, never used for Free evaluation.
// LOCAL marginal symbol weights; these are not original virtual reel strips. The
// differing feature weights below model documented increased feature chances.
// Phase1 PLAYABLE PACING only: base Phoenix150/Scatter30 gives LOCAL IID
// natural6+Phoenix~1:149.63 and3+Scatter~1:180.35. These were deliberately
// chosen to make all features accessible, NOT estimated from casino outcomes.
constexpr uint16_t baseWeights[12] = {170,170,155,145,135,80,65,50,35,25,30,150};
constexpr uint16_t freeWeights[12] = {170,170,155,145,135,80,65,50,35,25,30,180};
constexpr uint16_t holdLandingChance = 950;  // per empty cell / 10,000.
// Operator1.01-69279 pp1-2: these are TOTAL-BET multiples. Weights remain
// explicitly LOCAL prototype priors, not estimated/recovered casino odds.
constexpr uint16_t prizeValues[] = {1,2,3,4,5,10,15,20,50,100,200,250};
constexpr uint16_t prizeWeights[] = {7000,1800,600,250,150,90,40,25,15,10,4,2};
constexpr uint16_t miniChance = 45, minorChance = 12, majorChance = 1;
constexpr uint16_t doubleChance = 650, tripleChance = 150;  // /10,000.
constexpr int64_t jackpotBaseCoins[4] = {40,100,1000,10000};
// BigPayback cabinetG2301T0065: fixed Mini$175/Minor$875 at25c/15lines,
// including the recorded multiplier1Hold and subsequent multiplier5play.
// This property-specific reference does not establish every casino's awards.
constexpr int64_t observed25c15MiniCredits=700,observed25c15MinorCredits=3500;
// YfbaaFXhln0 1665/1680.6s: 50c,25lines,multiplier1 ($12.50 bet).
// Mini$500/Minor$2500; one recorded cabinet, not a universal wager ladder.
constexpr int64_t observed50c25x1MiniCredits=1000,observed50c25x1MinorCredits=5000;

// Documented feature rules, plus explicitly named LOCAL fallbacks where no
// target help-screen confirmation has yet been recovered.
constexpr uint8_t scatterTrigger = 3, freeGamesAward = 6;
// Confucius base3Scatter2x and4Scatter10x total bet are directly observed.
// Four: mQZbc-_wjO4 local section20s, $5 stake, Scatter wins50 ($1 credits).
// Basefive and Freefour remain LOCAL. Freefive is observed in originalUS
// Confucius5BUJOWxIBN4~2046..2064s:7500credits*$5=$37500 on$750bet,
// exactly50xtotalbet. FiveWisdom atFree6of6alsoadds3games(next7of9).
constexpr uint8_t baseScatterBetMultipliers[6] = {0,0,0,2,10,75};
// US Free3 Wisdom pays1x total bet: Yfbaa269s25credits*50c=$12.50
// on$12.50bet; mQZbc-_wjO4 cut132s5credits*$1=$5 on$5bet. Base3
// remains2x. Freefive50x and+3retrigger are unchanged.
constexpr uint8_t freeScatterBetMultipliers[6] = {0,0,0,1,15,50};
static constexpr auto& scatterBetMultipliers = baseScatterBetMultipliers;
constexpr bool twinOuterPairs = true;  // manufacturer: reels 1/2 and 4/5.
constexpr bool allowFreeRetrigger = true;
// Observed target US machine: mQZbc-_wjO4 Free2of6 with3Wisdom -> Free3of9.
// Quebec operator1.01-69279 p3 instead specifies+6: regional versions differ.
constexpr uint8_t retriggerGames = 3;
// Adjacent Sage Wild cells visually join into a taller figure (two-cell Sage
// with an unchanged Nine above is verified). This is visual joining, without
// changing the board. An unverified mathematical full-reel replacement is
// disabled; its optional conditional probability below remains explicitly LOCAL.
constexpr bool expandingSageEnabled = false;
constexpr uint16_t sageExpansionChance = 3500;  // /10,000 per Wild-containing reel.
constexpr bool sageExpansionReplacesSpecialSymbols = false;
constexpr uint8_t phoenixHoldTrigger = 6;  // observed lower-screen instruction.
constexpr uint8_t holdRespins = 3;  // operator1.01-69279 p2.
constexpr bool resetRespinsOnLanding = true;
constexpr bool fullBoardAwardsGrand = true;  // operator1.01-69279 pp1-2: Grand PLUS credits.
constexpr bool jackpotMultiplierAppliesToCredits = false;  //2x/3x apply to named prizes only.
constexpr bool jackpotScaleWithBetMultiplier = true;  // LOCAL scale table.
// Operator1.01-69279 p2: an initial Hold with zero NEW Phoenixes revives3.
// The text states no finite retry limit; no invented finite cutoff.
constexpr bool revivalEnabled = true;
// OBSERVED mQZbc-_wjO4 150–195s: meter831 ->834 ->835 ->836 while Hold
// lands new Phoenixes. 315s $1/5line/$10bet meter496, 330s $1/5line/$5bet
// meter851: wager changes select different counters. The exact internal key is
// unknown from these frames alone. BJ4HAXsPE1k11:03 explains persistence by
// denomination and TOTAL bet, even when a line-count flip reaches that bet.
constexpr bool meterCountsHoldLandings = true;
constexpr bool meterCountsPhoenixMultiplier = false;  // LOCAL unknown scope.
constexpr bool meterSharedAcrossWagers = false;
constexpr uint16_t meterMustHitBefore = 1888;  // supplied screen observation.
constexpr uint16_t meterThresholdMinimum = 1;  // LOCAL uniform prior; no claimed lower bound.
constexpr uint16_t meterThresholdMaximum = meterMustHitBefore - 1;
// Operator1.01-69279 p2: reset support100..1500. No weights are supplied.
constexpr uint16_t meterResetMinimum = 100, meterResetMaximum = 1500;
// The support above is documented. Uniform reset/threshold weights are STILL
// LOCAL assumptions; no actual distribution/hazard has been recovered.
constexpr PrizeCapProfile defaultPrizeCapProfile = PrizeCapProfile::HighCashOnly;
constexpr bool enforceSingleMajor = true;
constexpr uint16_t maximumPendingMeterHolds = 65535;  // defensive storage bound, not vendor limit.
constexpr uint8_t meterHoldSeedCount = 0;  // retain current only; no invented fill-to-six.
constexpr uint16_t maximumFreeGames = 1000;  // defensive persistence bound.
}  // namespace local

class Pcg32 {
 public:
  uint64_t state = 0, increment = 1;
  explicit Pcg32(uint64_t seed = 1, uint64_t stream = 54) { reseed(seed, stream); }
  void reseed(uint64_t seed, uint64_t stream = 54) {
    state = 0; increment = (stream << 1u) | 1u;
    next(); state += seed; next();
  }
  uint32_t next() {
    const uint64_t old = state;
    state = old * 6364136223846793005ULL + increment;
    const uint32_t shifted = uint32_t(((old >> 18u) ^ old) >> 27u);
    const uint32_t rotation = uint32_t(old >> 59u);
    return (shifted >> rotation) | (shifted << ((-rotation) & 31));
  }
  uint32_t bounded(uint32_t bound) {
    if (!bound) return 0;
    const uint32_t threshold = uint32_t(-bound) % bound;
    for (;;) { const uint32_t value = next(); if (value >= threshold) return value % bound; }
  }
};

struct LineWin {
  uint8_t line = 0, count = 0;
  Symbol symbol = Symbol::Nine;
  uint16_t mask = 0;
  int64_t credits = 0;
};
struct Statistics {
  uint64_t paidRounds = 0, paidHits = 0, freeAwards = 0, freeSpins = 0;
  uint64_t holdAwards = 0, meterAwards = 0, grandAwards = 0;
  uint64_t stakedMinor = 0, paidMinor = 0;
  uint64_t luckyAttempts = 0, luckyWins = 0;
};
struct MeterBank { uint16_t counter = 0, threshold = 0; };
struct State {
  Cell cells[kCells], holdCells[kCells];
  // All amounts named balance/win/*Minor are integer US cents. Cell.prize,
  // jackpotCoins and betCoins are integer credits (one credit = coinValue cents).
  int64_t balance = local::newBankMinor, win = 0, featureWin = 0, roundPayout = 0;
  int64_t coinValue = 100, betCoins = 10;
  int64_t jackpotCoins[4] = {80,200,2000,20000};  // Mini, Minor, Major, Grand.
  uint64_t seed = 1, rounds = 0, actions = 0;
  uint16_t freeLeft = 0, freePlayed = 0, meter = 0, meterThreshold = 300;
  MeterBank meterBanks[local::meterBucketCount];
  uint8_t respinsLeft = 0, lineCount = 5, betMultiplier = 2, wagerIndex = 0;
  uint8_t holdStartCount = 0, holdNewCount = 0;
  uint16_t holdRevivals = 0;
  Mode mode = Mode::Base;
  bool freeSession = false, roundOpen = false, grandAwarded = false, holdFromMeter = false;
  uint16_t pendingMeterHolds = 0;
  PrizeCapProfile prizeCapProfile = local::defaultPrizeCapProfile;
  Statistics stats;
};

// An action computes its final board ONCE. The renderer animates these copies
// and lands exactly spinResult/heldResult; animation never calls the RNG.
struct Presentation {
  Cell before[kCells], beforeExpansion[kCells], spinResult[kCells], heldBefore[kCells], heldResult[kCells], nextHeldResult[kCells];
  Mode beforeMode = Mode::Base, afterMode = Mode::Base;
  bool valid = false, paidSpin = false, freeSpin = false, holdRespin = false;
  bool freeAwarded = false, freeEnded = false, holdEntered = false, holdSettled = false;
  bool meterTriggered = false, grandAwarded = false, roundCompleted = false;
  bool chainedHoldEntered = false;  // collect old Hold, then intro nextHeldResult.
  bool holdFromMeter = false, holdRevived = false, revived = false;
  bool luckyAttempt = false, luckyWon = false;
  bool twinSpin = false, hasSpinResult = false, hasHeldResult = false;
  bool expandedWild = false;
  uint8_t expandedReelsMask = 0;  // bits 0..4: visual Sage joins/replacements.
  uint8_t freeAwardedCount = 0, scatterCount = 0, phoenixCount = 0;
  uint8_t respinsBefore = 0, respinsAfter = 0, lineWinCount = 0;
  uint16_t freeBefore = 0, freeAfter = 0, meterBefore = 0, meterAfter = 0;
  uint16_t meterAwardsCount = 0;
  // Committed presentation trace: arrivals replay resets without drawing RNG.
  uint16_t meterArrivalMask = 0, meterAfterCell[kCells] = {}, queuedAfterCell[kCells] = {};
  uint16_t pendingHoldsBefore = 0, pendingHoldsAfter = 0;
  uint16_t landedMask = 0, heldMask = 0, winMask = 0, scatterMask = 0;
  uint16_t expandedMask = 0;  // exact Wild cells that join, two or three per reel.
  int64_t balanceBefore = 0, balanceAfter = 0, stakeMinor = 0;
  int64_t lineWinMinor = 0, scatterWinMinor = 0, holdWinMinor = 0, phoenixWinMinor = 0, actionWinMinor = 0;
  uint64_t actionNumber = 0;
  LineWin lineWins[kMaxLines];
};

// Stable, explicitly little-endian bank encoding; no native-struct padding is
// persisted. The transient Presentation is reconstructed afresh on next action.
constexpr size_t kSaveCapacity = 2560;
struct Snapshot { uint8_t data[kSaveCapacity] = {}; size_t length = 0; };

class Engine {
 public:
  using Snapshot = phoenix::Snapshot;
  State state;
  Presentation presentation;

  explicit Engine(uint64_t seed = 1, int64_t bankMinor = local::newBankMinor) { reset(seed, bankMinor); }

  void reset(uint64_t seed, int64_t bankMinor = local::newBankMinor) {
    state = State{}; presentation = Presentation{};
    capacityFaults_ = 0;
    state.seed = seed;
    state.balance = bankMinor >= 0 && bankMinor <= local::maximumBankMinor ? bankMinor : local::newBankMinor;
    rng_.reseed(seed);
    resetActiveMeter();
    const Symbol initial[kCells] = {
      Symbol::Bell,Symbol::King,Symbol::Queen, Symbol::Ten,Symbol::Book,Symbol::Jack,
      Symbol::Candle,Symbol::Nine,Symbol::Abacus, Symbol::Queen,Symbol::Bell,Symbol::Ten,
      Symbol::King,Symbol::Abacus,Symbol::Book
    };
    for (size_t i = 0; i < kCells; ++i) state.cells[i].symbol = initial[i];
    setJackpots();
  }
  void newBank() {
    // Explicit fresh-session helper for simulation/test callers. UI credit
    // controls must use setCredits/addCredits to preserve progressive state.
    const uint64_t actions = state.actions + 1;
    const uint64_t high = rng_.next(), low = rng_.next();
    const uint64_t seed = (high << 32) | low;
    reset(seed); state.actions = actions;
  }
  int64_t betMinor() const { return state.betCoins * state.coinValue; }
  bool canChangeBet() const { return state.mode == Mode::Base && !state.roundOpen; }
  bool ready() const { return state.mode != Mode::Base || state.balance >= betMinor(); }
  // Transient result of the latest action attempt; not a saved math setting.
  // At finite storage exhaustion a feature remains owed, byte-for-byte. The
  // caller must stop retries and surface/export that liability, never reset it.
  bool capacityBlocked() const { return capacityFaults_ != 0; }
  uint32_t capacityFaultMask() const { return capacityFaults_; }
  int64_t capacityReserveMinor() const { return maximumActionAwardMinor(state.mode); }
  // Read-only diagnostics used to bind a persistent service fault after load.
  uint32_t capacityPreflightMask(bool lucky=false) const {
    return actionCapacityFaults(state.mode,lucky ? state.balance : betMinor(),lucky);
  }
  bool setCredits(int64_t amountMinor) {
    if (!canChangeBet() || amountMinor < 0 || amountMinor > local::maximumBankMinor) return false;
    state.balance=amountMinor;
    return true;
  }
  // External play-money funding is separate from game outcomes. Preserve
  // RNG, meter buckets, wagers, wins and statistics byte-for-byte.
  bool addCredits(int64_t amountMinor) {
    if (!canChangeBet() || amountMinor <= 0 || amountMinor > local::maximumBankMinor-state.balance) return false;
    state.balance += amountMinor;
    return true;
  }

  bool setWager(int64_t denominationMinor, uint8_t lineCount, uint8_t betMultiplier) {
    if (!canChangeBet() || !validWager(denominationMinor, lineCount, betMultiplier)) return false;
    if (state.coinValue == denominationMinor && state.lineCount == lineCount && state.betMultiplier == betMultiplier) return true;
    syncMeterBucket();
    state.coinValue = denominationMinor; state.lineCount = lineCount;
    state.betMultiplier = betMultiplier; state.betCoins = int64_t(lineCount) * betMultiplier;
    state.wagerIndex = 255;
    for (size_t i = 0; i < wagerCount(); ++i) {
      const auto& w = local::wagers[i];
      if (w.denomination == denominationMinor && w.lines == lineCount && w.multiplier == betMultiplier) state.wagerIndex = uint8_t(i);
    }
    activateMeterBucket(); setJackpots(); ++state.actions;
    return true;
  }
  void nextBet() {
    if (!canChangeBet()) return;
    const size_t index = state.wagerIndex < wagerCount() ? (state.wagerIndex + 1) % wagerCount() : 0;
    const auto& wager = local::wagers[index];
    setWager(wager.denomination, wager.lines, wager.multiplier);
  }
  bool action() { return actionImpl(nullptr); }
  bool step() { return action(); }
  bool canLuckyChance() const {
    return canChangeBet() && state.balance >= 1 && state.balance < betMinor();
  }
  // True means an eligible wager was processed, whether it won or lost.
  // The published probability is remaining cents/full configured wager cents.
  bool tryLuckyChance() { return luckyChanceImpl(nullptr); }

  // Only adjacent awarded faces join. Independent base-game frames at110s
  // and176s show neighbouring unawarded faces remaining separate and blue.
  // This is presentation geometry; it never adds Wilds or changes a payout.
  static uint16_t joinedSageMask(const Cell* cells, uint16_t awarded) {
    if (!cells) return 0;
    uint16_t joined = 0;
    for (size_t column = 0; column < kColumns; ++column) for (size_t row = 0; row + 1 < kRows; ++row) {
      const size_t first = cellIndex(column,row);
      const uint16_t pair = uint16_t(3u << first);
      if ((awarded & pair) == pair && cells[first].symbol == Symbol::Wild && cells[first+1].symbol == Symbol::Wild) joined |= pair;
    }
    return joined;
  }

  // A pure evaluator is useful both to the renderer and independent host tests.
  // Only the highest compatible combination pays on a line; Wild substitutes
  // regular symbols and never Scatter/Phoenix. Payouts are one line's credits.
  static LineWin evaluateLine(const Cell* cells, uint8_t line, uint8_t multiplier, bool free = false, uint8_t lineCount=kMaxLines, int64_t denominationMinor=0) {
    LineWin best; best.line = line;
    if (!cells || line >= kMaxLines || !multiplier) return best;
    for (uint8_t candidate = 0; candidate <= uint8_t(Symbol::Wild); ++candidate) {
      uint8_t count = 0; uint16_t mask = 0;
      for (size_t column = 0; column < kColumns; ++column) {
        const size_t i = cellIndex(column, local::linePath(line,lineCount)[column]);
        if (cells[i].symbol != Symbol(candidate) && cells[i].symbol != Symbol::Wild) break;
        ++count; mask |= uint16_t(1u << i);
      }
      const int64_t credits = int64_t(local::linePay(candidate,count,multiplier,free,lineCount,denominationMinor)) * multiplier;
      if (credits > best.credits) {
        best.symbol = Symbol(candidate); best.count = count; best.mask = mask; best.credits = credits;
      }
    }
    return best;
  }
  static int64_t cellCredits(const Cell& cell) {
    if (cell.symbol != Symbol::Phoenix) return 0;
    const uint8_t multiplier = cell.kind != PrizeKind::Credits || local::jackpotMultiplierAppliesToCredits ? cell.multiplier : 1;
    return cell.prize * multiplier;
  }
  int64_t jackpotValueCoins(unsigned tier) const {
    if(tier>=4)return 0;
    if(state.coinValue==25&&state.lineCount==15){
      if(tier==0)return local::observed25c15MiniCredits;
      if(tier==1)return local::observed25c15MinorCredits;
    }
    // An imported active cache may still hold the old local40/100 credits.
    // Resolve future landings without rewriting any committed cache or Cell.
    if(state.coinValue==50&&state.lineCount==25&&state.betMultiplier==1){
      if(tier==0)return local::observed50c25x1MiniCredits;
      if(tier==1)return local::observed50c25x1MinorCredits;
    }
    return state.jackpotCoins[tier];
  }
  static uint16_t phoenixMask(const Cell* cells) {
    uint16_t mask = 0;
    for (size_t i = 0; i < kCells; ++i) if (cells[i].symbol == Symbol::Phoenix) mask |= uint16_t(1u << i);
    return mask;
  }
  static uint8_t countMask(uint16_t mask) {
    uint8_t count = 0;
    while (mask) { count += uint8_t(mask & 1u); mask >>= 1; }
    return count;
  }
  static size_t wagerCount() { return sizeof(local::wagers) / sizeof(local::wagers[0]); }

  // Returned length is the required bank size even for a null/short buffer.
  size_t save(void* destination, size_t capacity) const {
    uint8_t buffer[kSaveCapacity] = {};
    Writer out(buffer, sizeof(buffer));
    out.u32(0x46434c50u);  // "PLCF", independent Phoenix Confucius bank.
    out.u16(4); out.u16(16); out.u32(0); out.u32(local::modelId);
    writeState(out, state); out.u64(rng_.state); out.u64(rng_.increment);
    const size_t length = out.position + 4;
    if (!out.good || length > sizeof(buffer)) return 0;
    writeU32(buffer + 8, uint32_t(length));
    out.u32(checksum(buffer, length - 4));
    if (destination && capacity >= length) std::memcpy(destination, buffer, length);
    return length;
  }
  Snapshot snapshot() const {
    Snapshot result; result.length = save(result.data, sizeof(result.data)); return result;
  }
  bool restore(const Snapshot& snapshot) { return restore(snapshot.data, snapshot.length); }
  bool load(const void* source, size_t length) { return restore(source, length); }
  bool restore(const void* source, size_t length) {
    if (!source || length < 20 || length > kSaveCapacity) return false;
    const auto* bytes = static_cast<const uint8_t*>(source);
    Reader in(bytes, length);
    const uint32_t magic = in.u32(); const uint16_t version = in.u16(), header = in.u16();
    const uint32_t size = in.u32(), model = in.u32();
    const bool legacy = version == 3 && model == local::legacyModelId && length == local::legacySaveBytes;
    // Previous schema4 models have the same wire layout. Already committed
    // bank/feature/prize/RNG bytes survive unchanged. A new pay revision only
    // changes future awards; stored prizes, wins and all meter buckets retain
    // their exact committed values, without resampling or retrospective pays.
    const bool current = version == 4 && (model == local::modelId || model == local::previousGPriceModelId || model == local::previousFPriceModelId || model == local::previousEPriceModelId || model == local::previousPlazaPriceModelId || model == local::previousBellPriceModelId || model == local::previousVegasPriceModelId || model == local::previousSevenPriceModelId || model == local::previousContextualPayModelId || model == local::previousJackPayModelId || model == local::previousPaylineModelId || model == local::previousNamedProfileModelId || model == local::previousFreePayModelId || model == local::precedingObservedPayModelId || model == local::olderObservedPayModelId || model == local::previousSchema4ModelId || model == local::olderSchema4ModelId || model == local::firstSchema4ModelId);
    if (magic != 0x46434c50u || (!legacy && !current) || header != 16 || size != length) return false;
    if (readU32(bytes + length - 4) != checksum(bytes, length - 4)) return false;
    State candidate; readState(in, candidate, legacy);
    const uint64_t rngState = in.u64(), rngIncrement = in.u64();
    in.u32();
    if (!in.good || in.position != length || !(rngIncrement & 1u) || !validState(candidate)) return false;
    state = candidate; rng_.state = rngState; rng_.increment = rngIncrement;
    presentation = Presentation{};  // checkpoint is already a fully settled action.
    capacityFaults_ = 0;
    return true;
  }

#ifdef PHOENIX_ENGINE_TESTING
  // Tests inject complete visible outcomes through the same financial/feature
  // transition path, without depending on rare random events. Not in firmware.
  bool testActionWithResult(const Cell cells[kCells]) { return actionImpl(cells); }
  bool testLuckyChanceWithResult(const Cell cells[kCells]) { return luckyChanceImpl(cells); }
  void testSetRng(uint64_t seed) { rng_.reseed(seed); }
  void testGenerateSpin(bool free) { generateSpin(free); }
  Cell testPhoenixCell(const Cell existing[kCells]) { return phoenixCell(existing); }
  int64_t testMaximumActionAwardMinor(Mode before) const { return maximumActionAwardMinor(before); }
#endif

 private:
  Pcg32 rng_;
  uint32_t capacityFaults_ = 0;

  struct Writer {
    uint8_t* bytes; size_t capacity, position = 0; bool good = true;
    Writer(uint8_t* b, size_t c) : bytes(b), capacity(c) {}
    void u8(uint8_t value) { if (position < capacity) bytes[position] = value; else good = false; ++position; }
    void u16(uint16_t value) { u8(uint8_t(value)); u8(uint8_t(value >> 8)); }
    void u32(uint32_t value) { for (unsigned i = 0; i < 4; ++i) u8(uint8_t(value >> (i * 8))); }
    void u64(uint64_t value) { for (unsigned i = 0; i < 8; ++i) u8(uint8_t(value >> (i * 8))); }
    void i64(int64_t value) { u64(uint64_t(value)); }
  };
  struct Reader {
    const uint8_t* bytes; size_t capacity, position = 0; bool good = true;
    Reader(const uint8_t* b, size_t c) : bytes(b), capacity(c) {}
    uint8_t u8() { if (position >= capacity) { good = false; ++position; return 0; } return bytes[position++]; }
    uint16_t u16() { uint16_t result = 0; for (unsigned i = 0; i < 2; ++i) result |= uint16_t(u8()) << (i * 8); return result; }
    uint32_t u32() { uint32_t result = 0; for (unsigned i = 0; i < 4; ++i) result |= uint32_t(u8()) << (i * 8); return result; }
    uint64_t u64() { uint64_t result = 0; for (unsigned i = 0; i < 8; ++i) result |= uint64_t(u8()) << (i * 8); return result; }
    int64_t i64() { return int64_t(u64()); }
  };

  static void writeU32(uint8_t* bytes, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) bytes[i] = uint8_t(value >> (8 * i));
  }
  static uint32_t readU32(const uint8_t* bytes) {
    uint32_t value = 0; for (unsigned i = 0; i < 4; ++i) value |= uint32_t(bytes[i]) << (8 * i); return value;
  }
  static uint32_t checksum(const uint8_t* bytes, size_t length) {
    uint32_t result = 2166136261u;
    for (size_t i = 0; i < length; ++i) { result ^= bytes[i]; result *= 16777619u; }
    return result;
  }
  static void writeCell(Writer& out, const Cell& cell) {
    out.u8(uint8_t(cell.symbol)); out.u8(uint8_t(cell.kind)); out.u8(cell.multiplier);
    out.u8(cell.highlight ? 1 : 0); out.i64(cell.prize);
  }
  static void readCell(Reader& in, Cell& cell) {
    cell.symbol = Symbol(in.u8()); cell.kind = PrizeKind(in.u8()); cell.multiplier = in.u8();
    const uint8_t highlight = in.u8(); if (highlight > 1) in.good = false;
    cell.highlight = highlight != 0; cell.prize = in.i64();
  }
  static void writeState(Writer& out, const State& s) {
    for (const Cell& c : s.cells) writeCell(out, c);
    for (const Cell& c : s.holdCells) writeCell(out, c);
    out.i64(s.balance); out.i64(s.win); out.i64(s.featureWin); out.i64(s.roundPayout);
    out.i64(s.coinValue); out.i64(s.betCoins);
    for (int64_t jackpot : s.jackpotCoins) out.i64(jackpot);
    out.u64(s.seed); out.u64(s.rounds); out.u64(s.actions);
    out.u16(s.freeLeft); out.u16(s.freePlayed); out.u16(s.meter); out.u16(s.meterThreshold);
    for (const MeterBank& bank : s.meterBanks) { out.u16(bank.counter); out.u16(bank.threshold); }
    out.u8(s.respinsLeft); out.u8(s.lineCount); out.u8(s.betMultiplier); out.u8(s.wagerIndex);
    out.u8(s.holdStartCount); out.u8(s.holdNewCount); out.u16(s.holdRevivals);
    out.u8(uint8_t(s.mode)); out.u8(s.freeSession ? 1 : 0); out.u8(s.roundOpen ? 1 : 0); out.u8(s.grandAwarded ? 1 : 0); out.u8(s.holdFromMeter ? 1 : 0);
    out.u16(s.pendingMeterHolds); out.u8(uint8_t(s.prizeCapProfile));
    const Statistics& t = s.stats;
    out.u64(t.paidRounds); out.u64(t.paidHits); out.u64(t.freeAwards); out.u64(t.freeSpins);
    out.u64(t.holdAwards); out.u64(t.meterAwards); out.u64(t.grandAwards); out.u64(t.stakedMinor); out.u64(t.paidMinor);
    out.u64(t.luckyAttempts); out.u64(t.luckyWins);
  }
  static void readState(Reader& in, State& s, bool legacy) {
    for (Cell& c : s.cells) readCell(in, c);
    for (Cell& c : s.holdCells) readCell(in, c);
    s.balance = in.i64(); s.win = in.i64(); s.featureWin = in.i64(); s.roundPayout = in.i64();
    s.coinValue = in.i64(); s.betCoins = in.i64();
    for (int64_t& jackpot : s.jackpotCoins) jackpot = in.i64();
    s.seed = in.u64(); s.rounds = in.u64(); s.actions = in.u64();
    s.freeLeft = in.u16(); s.freePlayed = in.u16(); s.meter = in.u16(); s.meterThreshold = in.u16();
    if (legacy) {
      // Remap by actual credit bet, never array offset:15line options added
      //105/120/135-credit buckets. No random draw occurs during migration.
      for (size_t denomination = 0; denomination < local::denominationCount; ++denomination)
        for (size_t oldBet = 0; oldBet < local::legacyMeterBetCount; ++oldBet) {
          const uint16_t counter = in.u16(), threshold = in.u16();
          for (size_t newBet = 0; newBet < local::meterBetCount; ++newBet)
            if (local::meterBetCredits[newBet] == local::legacyMeterBetCredits[oldBet])
              s.meterBanks[denomination * local::meterBetCount + newBet] = MeterBank{counter,threshold};
        }
    } else for (MeterBank& bank : s.meterBanks) { bank.counter = in.u16(); bank.threshold = in.u16(); }
    s.respinsLeft = in.u8(); s.lineCount = in.u8(); s.betMultiplier = in.u8(); s.wagerIndex = in.u8();
    s.holdStartCount = in.u8(); s.holdNewCount = in.u8(); s.holdRevivals = in.u16();
    s.mode = Mode(in.u8());
    const uint8_t free = in.u8(), round = in.u8(), grand = in.u8();
    uint8_t meterHold = 0;
    if (legacy) {
      const uint8_t pending = in.u8(); meterHold = in.u8();
      if (pending > 1 || s.lineCount == 15 || (s.wagerIndex != 255 && s.wagerIndex >= 40)) in.good = false;
      s.pendingMeterHolds = pending;
    } else {
      meterHold = in.u8(); s.pendingMeterHolds = in.u16(); s.prizeCapProfile = PrizeCapProfile(in.u8());
    }
    if (free > 1 || round > 1 || grand > 1 || meterHold > 1) in.good = false;
    s.freeSession = free != 0; s.roundOpen = round != 0; s.grandAwarded = grand != 0;
    s.holdFromMeter = meterHold != 0;
    Statistics& t = s.stats;
    t.paidRounds = in.u64(); t.paidHits = in.u64(); t.freeAwards = in.u64(); t.freeSpins = in.u64();
    t.holdAwards = in.u64(); t.meterAwards = in.u64(); t.grandAwards = in.u64(); t.stakedMinor = in.u64(); t.paidMinor = in.u64();
    if (!legacy) { t.luckyAttempts = in.u64(); t.luckyWins = in.u64(); }
    else {
      // Locked old ordinary-credit multipliers must retain their financial
      // value after named-only multipliers are enforced in the new model.
      for (Cell* board : {s.cells,s.holdCells}) for (size_t i = 0; i < kCells; ++i) {
        Cell& c = board[i];
        const bool validLegacyPrize = c.prize >= 0 && c.prize <= 1000000000LL && c.multiplier >= 1 && c.multiplier <= 3;
        if (!validLegacyPrize) in.good = false;
        if (validLegacyPrize && c.symbol == Symbol::Phoenix && c.kind == PrizeKind::Credits) {
          c.prize *= c.multiplier; c.multiplier = 1;
        }
      }
      for (int64_t jackpot : s.jackpotCoins) if (jackpot < 1 || jackpot > 1000000000LL) in.good = false;
    }
  }
  static bool validWager(int64_t denomination, uint8_t lines, uint8_t multiplier) {
    bool validDenomination = false;
    for (int64_t value : local::denominations) if (value == denomination) validDenomination = true;
    bool validLines = false;
    for (uint8_t value : local::lineOptions) if (value == lines) validLines = true;
    return validDenomination && validLines && multiplier >= 1 && multiplier <= 10;
  }
  static size_t meterBucketIndex(const State& s) {
    if (local::meterSharedAcrossWagers) return 0;
    size_t denomination = 0;
    for (size_t i = 0; i < local::denominationCount; ++i) if (local::denominations[i] == s.coinValue) denomination = i;
    size_t bet = 0;
    for (size_t i = 0; i < local::meterBetCount; ++i) if (local::meterBetCredits[i] == s.betCoins) bet = i;
    return denomination * local::meterBetCount + bet;
  }
  static bool validCell(const Cell& c) {
    if (uint8_t(c.symbol) >= uint8_t(Symbol::Count) || uint8_t(c.kind) > uint8_t(PrizeKind::Major) || c.multiplier < 1 || c.multiplier > 3) return false;
    if (c.prize < 0 || c.prize > local::maximumCreditPrize) return false;
    return c.symbol == Symbol::Phoenix ? c.prize > 0 && (c.kind != PrizeKind::Credits || c.multiplier == 1) : c.prize == 0 && c.kind == PrizeKind::Credits && c.multiplier == 1;
  }
  static bool validState(const State& s) {
    if (!validWager(s.coinValue, s.lineCount, s.betMultiplier) || s.betCoins != int64_t(s.lineCount) * s.betMultiplier) return false;
    if (s.wagerIndex != 255 && s.wagerIndex >= wagerCount()) return false;
    if (s.wagerIndex != 255) {
      const auto& w = local::wagers[s.wagerIndex];
      if (w.denomination != s.coinValue || w.lines != s.lineCount || w.multiplier != s.betMultiplier) return false;
    }
    const int64_t amounts[] = {s.balance,s.win,s.featureWin,s.roundPayout};
    for (int64_t value : amounts) if (value < 0 || value > local::maximumStoredMinor) return false;
    for (int64_t value : s.jackpotCoins) if (value <= 0 || value > local::maximumCreditPrize) return false;
    if (s.meterThreshold < local::meterThresholdMinimum || s.meterThreshold > local::meterThresholdMaximum || s.meter >= s.meterThreshold) return false;
    for (const MeterBank& bank : s.meterBanks) {
      if (!bank.threshold) { if (bank.counter) return false; }
      else if (bank.threshold < local::meterThresholdMinimum || bank.threshold > local::meterThresholdMaximum || bank.counter >= bank.threshold) return false;
    }
    const MeterBank& active = s.meterBanks[meterBucketIndex(s)];
    if (active.counter != s.meter || active.threshold != s.meterThreshold) return false;
    if (uint8_t(s.mode) > uint8_t(Mode::Hold) || s.respinsLeft > local::holdRespins || s.freeLeft > local::maximumFreeGames) return false;
    if (s.mode == Mode::Base && (s.freeLeft || s.freeSession || s.roundOpen || s.respinsLeft)) return false;
    if (s.mode == Mode::Free && (!s.freeLeft || !s.freeSession || !s.roundOpen || s.respinsLeft)) return false;
    if (s.mode == Mode::Hold) {
      const uint8_t count = countMask(phoenixMask(s.holdCells));
      if (!s.roundOpen || !s.respinsLeft || (!s.holdFromMeter && s.holdStartCount < local::phoenixHoldTrigger) || s.holdStartCount > 15 || s.holdNewCount > 15 || s.holdStartCount + s.holdNewCount != count) return false;
    }
    if (s.freeLeft && !s.freeSession) return false;
    if (s.pendingMeterHolds && s.mode != Mode::Hold) return false;
    if (uint8_t(s.prizeCapProfile) > uint8_t(PrizeCapProfile::NamedOrHighCash) || s.stats.luckyWins > s.stats.luckyAttempts) return false;
    for (const Cell& c : s.cells) if (!validCell(c)) return false;
    for (const Cell& c : s.holdCells) if (!validCell(c)) return false;
    return true;
  }
  static void copyCells(Cell* destination, const Cell* source) { std::memcpy(destination, source, sizeof(Cell) * kCells); }

  void setJackpots() {
    for (size_t i = 0; i < 4; ++i) state.jackpotCoins[i] = local::jackpotBaseCoins[i] * (local::jackpotScaleWithBetMultiplier ? state.betMultiplier : 1);
    if(state.coinValue==25&&state.lineCount==15){state.jackpotCoins[0]=local::observed25c15MiniCredits;state.jackpotCoins[1]=local::observed25c15MinorCredits;}
    if(state.coinValue==50&&state.lineCount==25&&state.betMultiplier==1){state.jackpotCoins[0]=local::observed50c25x1MiniCredits;state.jackpotCoins[1]=local::observed50c25x1MinorCredits;}
  }
  uint16_t nextMeterThreshold(uint16_t after) {
    return uint16_t(after + 1 + rng_.bounded(local::meterThresholdMaximum - after));
  }
  void resetActiveMeter() {
    state.meter = uint16_t(local::meterResetMinimum + rng_.bounded(local::meterResetMaximum - local::meterResetMinimum + 1));
    state.meterThreshold = nextMeterThreshold(state.meter);
    syncMeterBucket();
  }
  void syncMeterBucket() {
    MeterBank& bank = state.meterBanks[meterBucketIndex(state)];
    bank.counter = state.meter; bank.threshold = state.meterThreshold;
  }
  void activateMeterBucket() {
    MeterBank& bank = state.meterBanks[meterBucketIndex(state)];
    if (!bank.threshold) {
      resetActiveMeter();
      return;
    }
    state.meter = bank.counter; state.meterThreshold = bank.threshold;
  }
  template <typename T, size_t N> uint32_t weighted(const T (&weights)[N]) {
    uint32_t total = 0; for (auto weight : weights) total += weight;
    uint32_t draw = rng_.bounded(total);
    for (size_t i = 0; i < N; ++i) { if (draw < weights[i]) return uint32_t(i); draw -= weights[i]; }
    return uint32_t(N - 1);
  }
  bool highCash(const Cell& cell) const {
    return cell.symbol == Symbol::Phoenix && cell.kind == PrizeKind::Credits && cell.prize >= 200 * state.betCoins;
  }
  Cell phoenixCell(const Cell* existing) {
    Cell result; result.symbol = Symbol::Phoenix;
    bool hasHigh = false, hasNamed = false, hasMajor = false;
    if (existing) for (size_t i = 0; i < kCells; ++i) if (existing[i].symbol == Symbol::Phoenix) {
      hasHigh |= highCash(existing[i]); hasNamed |= existing[i].kind != PrizeKind::Credits;
      hasMajor |= existing[i].kind == PrizeKind::Major;
    }
    const bool anyExclusive = hasHigh || hasNamed;
    const bool allowNamed = state.prizeCapProfile != PrizeCapProfile::NamedOrHighCash || !anyExclusive;
    const bool allowHigh = state.prizeCapProfile == PrizeCapProfile::NamedOrHighCash ? !anyExclusive : !hasHigh;
    const uint32_t prizeRoll = rng_.bounded(10000);
    if (prizeRoll < local::majorChance) result.kind = PrizeKind::Major;
    else if (prizeRoll < local::majorChance + local::minorChance) result.kind = PrizeKind::Minor;
    else if (prizeRoll < local::majorChance + local::minorChance + local::miniChance) result.kind = PrizeKind::Mini;
    if (!allowNamed || (result.kind == PrizeKind::Major && local::enforceSingleMajor && hasMajor)) result.kind = PrizeKind::Credits;
    if (result.kind == PrizeKind::Credits) {
      uint32_t total = 0;
      for (size_t i = 0; i < sizeof(local::prizeValues)/sizeof(local::prizeValues[0]); ++i)
        if (allowHigh || local::prizeValues[i] < 200) total += local::prizeWeights[i];
      uint32_t draw = rng_.bounded(total);
      for (size_t i = 0; i < sizeof(local::prizeValues)/sizeof(local::prizeValues[0]); ++i) {
        const uint32_t weight = allowHigh || local::prizeValues[i] < 200 ? local::prizeWeights[i] : 0;
        if (draw < weight) { result.prize = int64_t(local::prizeValues[i]) * state.betCoins; break; }
        draw -= weight;
      }
    } else {
      // Old saves retain locked cell values. This getter also supplies the
      // corrected future Mini/Minor values when an old cache is restored.
      result.prize = jackpotValueCoins(uint8_t(result.kind) - 1);
      const uint32_t multiplierRoll = rng_.bounded(10000);
      result.multiplier = multiplierRoll < local::tripleChance ? 3 : multiplierRoll < local::tripleChance + local::doubleChance ? 2 : 1;
    }
    return result;
  }
  Cell randomCell(bool free, size_t column, bool excludeWild=false) {
    const bool allowWild = !excludeWild && (free ? column >= 2 : column >= 1);  // operatorpp3-4.
    const auto& weights = free ? local::freeWeights : local::baseWeights;
    uint32_t total = 0;
    for (size_t i = 0; i < uint8_t(Symbol::Count); ++i) if (allowWild || i != uint8_t(Symbol::Wild)) total += weights[i];
    uint32_t draw = rng_.bounded(total); Cell result;
    for (size_t i = 0; i < uint8_t(Symbol::Count); ++i) {
      const uint32_t weight = allowWild || i != uint8_t(Symbol::Wild) ? weights[i] : 0;
      if (draw < weight) { result.symbol = Symbol(i); break; } draw -= weight;
    }
    return result.symbol == Symbol::Phoenix ? phoenixCell(state.cells) : result;
  }
  void generateSpin(bool free) {
    for (Cell& cell : state.cells) cell = Cell{};  // avoid counting a stale prior board's prizes.
    for (size_t column = 0; column < kColumns; ++column) {
      if (free && local::twinOuterPairs && (column == 1 || column == 4)) {
        for (size_t row = 0; row < kRows; ++row) {
          const Cell& twin = state.cells[cellIndex(column - 1,row)];
          // Observed hmy1540s: paired reels share symbol stops, while Phoenix
          // cash values and jackpot labels differ. Do not copy a prize value.
          state.cells[cellIndex(column,row)] = twin.symbol == Symbol::Phoenix ? phoenixCell(state.cells) : twin;
        }
      } else {
        // Footage shows contiguous three-Sage strip runs, including partial
        // runs at a window edge. IID rows made the observed tall figure almost
        // absent. This LOCAL correlated model preserves the existing per-row
        // Wild marginal exactly; it does not recover real reel stops or odds.
        const bool eligible = free ? column >= 2 : column >= 1;
        const auto& weights = free ? local::freeWeights : local::baseWeights;
        uint32_t total=0;for(auto weight:weights)total+=weight;
        // A uniformly offset length3 run intersects this length3 window in
        //1,2,3,2,1 cells. Each row is covered for3of5 offsets.
        const bool run=eligible && rng_.bounded(3*total)<5*weights[unsigned(Symbol::Wild)];
        const int start=run?int(rng_.bounded(5))-2:4;
        for(size_t row=0;row<kRows;++row) {
          Cell cell;
          if(run&&int(row)>=start&&int(row)<start+3)cell.symbol=Symbol::Wild;
          else cell=randomCell(free,column,true);
          state.cells[cellIndex(column,row)]=cell;
        }
      }
    }
  }
  void expandSage(bool free) {
    copyCells(presentation.beforeExpansion, state.cells);
    for (size_t column = 0; local::expandingSageEnabled && column < kColumns; ++column) {
      if (free && local::twinOuterPairs && (column == 1 || column == 4)) continue;
      bool wild = false, special = false;
      for (size_t row = 0; row < kRows; ++row) {
        const Symbol symbol = state.cells[cellIndex(column,row)].symbol;
        wild |= symbol == Symbol::Wild;
        special |= symbol == Symbol::Phoenix || symbol == Symbol::Scatter;
      }
      if (!wild || (special && !local::sageExpansionReplacesSpecialSymbols) || rng_.bounded(10000) >= local::sageExpansionChance) continue;
      const size_t last = free && local::twinOuterPairs && (column == 0 || column == 3) ? column + 1 : column;
      for (size_t expandedColumn = column; expandedColumn <= last; ++expandedColumn) {
        presentation.expandedReelsMask |= uint8_t(1u << expandedColumn);
        for (size_t row = 0; row < kRows; ++row) {
          const size_t index = cellIndex(expandedColumn,row);
          state.cells[index] = Cell{}; state.cells[index].symbol = Symbol::Wild;
          presentation.expandedMask |= uint16_t(1u << index);
        }
      }
      presentation.expandedWild = true;
    }
    // Actual observed Sage growth can join just two adjacent Wild cells. Tag
    // the existing contiguous cells for the renderer, without adding Wilds or
    // consuming any random numbers. Top/bottom Wilds separated by another
    // symbol do not join and must not cover that middle symbol.
    for (size_t column = 0; column < kColumns; ++column) for (size_t row = 0; row + 1 < kRows; ++row) {
      const size_t first = cellIndex(column,row), second = cellIndex(column,row+1);
      if (state.cells[first].symbol == Symbol::Wild && state.cells[second].symbol == Symbol::Wild) {
        presentation.expandedWild = true;
        presentation.expandedReelsMask |= uint8_t(1u << column);
        presentation.expandedMask |= uint16_t((1u << first) | (1u << second));
      }
    }
  }
  void pay(int64_t credits, bool feature) {
    const int64_t minor = credits * state.coinValue;
    state.balance += minor; state.win += minor; state.roundPayout += minor;
    if (feature || state.freeSession) state.featureWin += minor;
    state.stats.paidMinor += uint64_t(minor);
  }
  static bool storedAddFits(int64_t value, int64_t added) {
    return value >= 0 && added >= 0 && value <= local::maximumStoredMinor - added;
  }
  static bool counterAddFits(uint64_t value, uint64_t added) {
    return value <= std::numeric_limits<uint64_t>::max() - added;
  }
  int64_t maximumActionAwardMinor(Mode before) const {
    static_assert(local::meterHoldSeedCount < kCells, "Revisit capacity bound if queued Holds can enter full");
    // At most one15-cell Phoenix/Grand collection can settle in one action.
    // Queued Holds start empty; later actions collect them independently.
    // Three is the largest valid named multiplier, including imported prizes.
    int64_t credits = int64_t(kCells) * local::maximumCreditPrize * 3 + state.jackpotCoins[3];
    if (before != Mode::Hold) {
      const bool free = before == Mode::Free;
      uint16_t maxLine = 0;
      for (unsigned symbol = 0; symbol < 10; ++symbol) for (unsigned count = 0; count <= kColumns; ++count) {
        const uint16_t pay = local::linePay(symbol,count,state.betMultiplier,free,state.lineCount,state.coinValue);
        if (pay > maxLine) maxLine = pay;
      }
      credits += int64_t(state.lineCount) * state.betMultiplier * maxLine;
      const auto& scatters = free ? local::freeScatterBetMultipliers : local::baseScatterBetMultipliers;
      uint8_t maxScatter = 0; for (uint8_t value : scatters) if (value > maxScatter) maxScatter = value;
      credits += state.betCoins * maxScatter;
    }
    return credits * state.coinValue;
  }
  uint32_t actionCapacityFaults(Mode before, int64_t stake, bool lucky = false) const {
    // This outcome-independent preflight runs before RNG, stake or partial pay.
    // All bit meanings are stable caller/service diagnostics, not saved math.
    uint32_t faults=0;
    const int64_t award = maximumActionAwardMinor(before);
    const int64_t balanceAfterStake = state.balance - (before == Mode::Base ? stake : 0);
    if (!storedAddFits(balanceAfterStake,award)) faults|=CapacityBalance;
    if (before != Mode::Base && !storedAddFits(state.roundPayout,award)) faults|=CapacityRoundPayout;
    if (before != Mode::Base && !storedAddFits(state.featureWin,award)) faults|=CapacityFeatureWin;
    if (!counterAddFits(state.stats.paidMinor,uint64_t(award))) faults|=CapacityLifetimePaid;
    if (!counterAddFits(state.actions,1)) faults|=CapacityActions;
    if (!counterAddFits(state.stats.holdAwards,1)) faults|=CapacityHoldAwards;
    if (!counterAddFits(state.stats.grandAwards,1)) faults|=CapacityGrandAwards;
    if (!counterAddFits(state.stats.paidHits,1)) faults|=CapacityPaidHits;
    if (!counterAddFits(state.stats.meterAwards,uint64_t(kCells)*(local::meterCountsPhoenixMultiplier?3:1))) faults|=CapacityMeterAwards;
    if (before == Mode::Base) {
      if (!counterAddFits(state.stats.stakedMinor,uint64_t(stake))) faults|=CapacityLifetimeStaked;
      if (!counterAddFits(state.stats.paidRounds,1)) faults|=CapacityPaidRounds;
      if (!counterAddFits(state.rounds,1)) faults|=CapacityRounds;
    }
    if (before == Mode::Free && !counterAddFits(state.stats.freeSpins,1)) faults|=CapacityFreeSpins;
    if (before != Mode::Hold && !counterAddFits(state.stats.freeAwards,1)) faults|=CapacityFreeAwards;
    if (lucky) {
      if (!counterAddFits(state.stats.luckyAttempts,1)) faults|=CapacityLuckyAttempts;
      if (!counterAddFits(state.stats.luckyWins,1)) faults|=CapacityLuckyWins;
    }
    return faults;
  }
  void collectMeter(const Cell* cells, uint16_t mask) {
    // Every arrival is processed individually. More than one feature can be
    // queued if multiple random meter hits occur; no one-bit award is lost.
    for (size_t i = 0; i < kCells; ++i) if (mask & uint16_t(1u << i)) {
      unsigned count = local::meterCountsPhoenixMultiplier ? cells[i].multiplier : 1;
      while (count--) {
        ++state.meter;
        if (state.meter >= state.meterThreshold) {
          presentation.meterTriggered = true; ++presentation.meterAwardsCount; ++state.stats.meterAwards;
          resetActiveMeter();
          if (state.mode == Mode::Hold) queueMeterHolds(1);
        }
      }
      presentation.meterArrivalMask |= uint16_t(1u << i);
      presentation.meterAfterCell[i] = state.meter;
      presentation.queuedAfterCell[i] = state.pendingMeterHolds;
    }
    syncMeterBucket();
  }
  void queueMeterHolds(uint16_t count) {
    const uint32_t total = uint32_t(state.pendingMeterHolds) + count;
    state.pendingMeterHolds = uint16_t(total > local::maximumPendingMeterHolds ? local::maximumPendingMeterHolds : total);
  }
  void enterHold(bool meterTriggered, bool chained = false) {
    for (size_t i = 0; i < kCells; ++i) state.holdCells[i] = state.cells[i].symbol == Symbol::Phoenix ? state.cells[i] : Cell{};
    // Meter entry keeps only the Phoenixes already on the triggering board.
    // For a queued second Hold the old paid board is cleared first (actual
    // hmyKdYjMjAQ509.5s shows the queued meter feature beginning empty).
    if (meterTriggered) {
      uint8_t filled = countMask(phoenixMask(state.holdCells));
      while (filled < local::meterHoldSeedCount) {
        const size_t index = rng_.bounded(kCells);
        if (state.holdCells[index].symbol != Symbol::Phoenix) { state.holdCells[index] = phoenixCell(state.holdCells); ++filled; }
      }
    }
    state.mode = Mode::Hold; state.respinsLeft = local::holdRespins;
    state.holdFromMeter = meterTriggered; state.holdStartCount = countMask(phoenixMask(state.holdCells));
    state.holdNewCount = 0; state.holdRevivals = 0;
    ++state.stats.holdAwards;
    if (chained) {
      presentation.chainedHoldEntered = true; presentation.meterTriggered = true;
      copyCells(presentation.nextHeldResult, state.holdCells);
    } else {
      presentation.holdEntered = true; presentation.hasHeldResult = true;
      copyCells(presentation.heldResult, state.holdCells);
      presentation.heldMask = phoenixMask(state.holdCells);
    }
    presentation.holdFromMeter = meterTriggered;
    // A full board that triggers the feature settles immediately exactly once.
    if (phoenixMask(state.holdCells) == 0x7fff) finishHold();
  }
  void finishRoundIfComplete() {
    if (state.mode == Mode::Hold) return;
    if (state.freeLeft) { state.mode = Mode::Free; return; }
    if (state.freeSession) presentation.freeEnded = true;
    state.mode = Mode::Base; state.freeSession = false; state.roundOpen = false; state.respinsLeft = 0;
    presentation.roundCompleted = true;
    if (state.roundPayout > 0) ++state.stats.paidHits;
  }
  void settleSpin(bool free) {
    presentation.hasSpinResult = true; presentation.twinSpin = free && local::twinOuterPairs;
    expandSage(free);
    for (Cell& cell : state.cells) cell.highlight = false;
    for (uint8_t line = 0; line < state.lineCount; ++line) {
      const LineWin win = evaluateLine(state.cells, line, state.betMultiplier, free, state.lineCount, state.coinValue);
      if (win.credits) {
        presentation.lineWins[presentation.lineWinCount++] = win;
        presentation.winMask |= win.mask; presentation.lineWinMinor += win.credits * state.coinValue;
        pay(win.credits, free);
      }
    }
    // A win using one face must not turn its unawarded neighbours into a
    // tall gold figure. Preserve every symbol and all financial calculations.
    presentation.expandedMask = joinedSageMask(state.cells,presentation.winMask);
    presentation.expandedReelsMask = 0;
    for (size_t column = 0; column < kColumns; ++column) {
      const uint16_t reel = uint16_t(7u << (column * kRows));
      if (presentation.expandedMask & reel) presentation.expandedReelsMask |= uint8_t(1u << column);
    }
    presentation.expandedWild = presentation.expandedMask != 0;
    for (size_t i = 0; i < kCells; ++i) {
      if (presentation.winMask & uint16_t(1u << i)) state.cells[i].highlight = true;
      if (state.cells[i].symbol == Symbol::Scatter) {
        ++presentation.scatterCount; presentation.scatterMask |= uint16_t(1u << i);
      }
    }
    const uint8_t scatterIndex = presentation.scatterCount < 5 ? presentation.scatterCount : 5;
    const int64_t scatterCredits = state.betCoins * (free ? local::freeScatterBetMultipliers[scatterIndex] : local::baseScatterBetMultipliers[scatterIndex]);
    if (scatterCredits) {
      pay(scatterCredits, free);
      presentation.scatterWinMinor = scatterCredits * state.coinValue;
      presentation.winMask |= presentation.scatterMask;
      for (size_t i = 0; i < kCells; ++i) if (presentation.scatterMask & uint16_t(1u << i)) state.cells[i].highlight = true;
    }
    const uint16_t phoenixes = phoenixMask(state.cells);
    presentation.phoenixCount = countMask(phoenixes);
    collectMeter(state.cells, phoenixes);
    if (presentation.scatterCount >= local::scatterTrigger && (!free || local::allowFreeRetrigger)) {
      const uint16_t added = free ? local::retriggerGames : local::freeGamesAward;
      const uint16_t room = local::maximumFreeGames - state.freeLeft;
      const uint16_t granted = added < room ? added : room;
      state.freeLeft += granted; state.freeSession = true;
      if (!free) state.featureWin = 0;
      ++state.stats.freeAwards; presentation.freeAwarded = true; presentation.freeAwardedCount = uint8_t(granted);
    }
    copyCells(presentation.spinResult, state.cells);
    const bool full = presentation.phoenixCount == kCells;
    const bool naturalHold = presentation.phoenixCount >= local::phoenixHoldTrigger && !full;
    if (naturalHold || full) queueMeterHolds(presentation.meterAwardsCount);
    else if (presentation.meterAwardsCount > 1) queueMeterHolds(presentation.meterAwardsCount - 1);
    if (full) {
      // The operator's15-Phoenix rule pays Grand AND every Phoenix directly;
      // it does not enter an artificial Hold. The general15-symbol rule is
      // also used for a Free result; no original Free15 exception is published.
      int64_t credits = state.jackpotCoins[3];
      for (const Cell& cell : state.cells) credits += cellCredits(cell);
      pay(credits,true); presentation.phoenixWinMinor = credits * state.coinValue;
      presentation.grandAwarded = state.grandAwarded = true; ++state.stats.grandAwards;
      if (!startQueuedHold()) finishRoundIfComplete();
    } else if (naturalHold || presentation.meterTriggered) enterHold(!naturalHold);
    else finishRoundIfComplete();
  }
  bool startQueuedHold() {
    if (!state.pendingMeterHolds) return false;
    --state.pendingMeterHolds;
    for (Cell& cell : state.cells) cell = Cell{};
    enterHold(true,true);
    return true;
  }
  void finishHold() {
    int64_t credits = 0;
    for (const Cell& cell : state.holdCells) credits += cellCredits(cell);
    if (local::fullBoardAwardsGrand && phoenixMask(state.holdCells) == 0x7fff) {
      credits += state.jackpotCoins[3]; state.grandAwarded = true;
      presentation.grandAwarded = true; ++state.stats.grandAwards;
    }
    pay(credits, true); presentation.phoenixWinMinor = presentation.holdWinMinor = credits * state.coinValue;
    presentation.holdSettled = true; state.respinsLeft = 0;
    // Preserve the final held cells in state and the presentation for the count
    // up, while the next action is already the correct free/base phase.
    copyCells(state.cells, state.holdCells);
    if (startQueuedHold()) return;
    state.mode = state.freeLeft ? Mode::Free : Mode::Base;
    finishRoundIfComplete();
  }
  void holdStep(const Cell* scripted) {
    copyCells(presentation.heldBefore, state.holdCells);
    presentation.heldMask = phoenixMask(state.holdCells);
    --state.respinsLeft;
    for (size_t i = 0; i < kCells; ++i) {
      if (state.holdCells[i].symbol == Symbol::Phoenix) continue;
      const bool landed = scripted ? scripted[i].symbol == Symbol::Phoenix : rng_.bounded(10000) < local::holdLandingChance;
      if (landed) {
        state.holdCells[i] = scripted ? scripted[i] : phoenixCell(state.holdCells);
        presentation.landedMask |= uint16_t(1u << i);
      }
    }
    if (presentation.landedMask && local::resetRespinsOnLanding) state.respinsLeft = local::holdRespins;
    state.holdNewCount += countMask(presentation.landedMask);
    if (local::meterCountsHoldLandings) collectMeter(state.holdCells, presentation.landedMask);
    presentation.hasHeldResult = true; copyCells(presentation.heldResult, state.holdCells);
    copyCells(state.cells, state.holdCells);
    if (!state.respinsLeft && local::revivalEnabled && !state.holdNewCount) {
      state.respinsLeft = local::holdRespins; presentation.holdRevived = presentation.revived = true;
      if (state.holdRevivals != 65535) ++state.holdRevivals;
    } else if (!state.respinsLeft || phoenixMask(state.holdCells) == 0x7fff) finishHold();
  }
  bool actionImpl(const Cell* scripted, int64_t paidStakeOverride = -1) {
    presentation = Presentation{};
    capacityFaults_ = 0;
    const Mode before = state.mode;
    const int64_t stake = paidStakeOverride >= 0 ? paidStakeOverride : betMinor();
    if (before == Mode::Base && state.balance < stake) return false;
    if (before == Mode::Free && !state.freeLeft) return false;
    if (before == Mode::Hold && !state.respinsLeft) return false;
    if (scripted) for (size_t i = 0; i < kCells; ++i) if (!validCell(scripted[i])) return false;
    if ((capacityFaults_ = actionCapacityFaults(before,stake)) != 0) return false;
    presentation.beforeMode = before; presentation.balanceBefore = state.balance;
    presentation.freeBefore = state.freeLeft; presentation.respinsBefore = state.respinsLeft; presentation.meterBefore = state.meter;
    presentation.pendingHoldsBefore = state.pendingMeterHolds;
    copyCells(presentation.before, state.cells); state.win = 0;
    if (before == Mode::Base) {
      state.balance -= stake; state.stats.stakedMinor += uint64_t(stake);
      ++state.rounds; ++state.stats.paidRounds; state.roundOpen = true;
      state.roundPayout = 0; state.featureWin = 0; state.freePlayed = 0; state.grandAwarded = false;
      presentation.paidSpin = true; presentation.stakeMinor = stake;
      if (scripted) copyCells(state.cells, scripted); else generateSpin(false);
      settleSpin(false);
    } else if (before == Mode::Free) {
      presentation.freeSpin = true; --state.freeLeft; ++state.freePlayed; ++state.stats.freeSpins;
      if (scripted) copyCells(state.cells, scripted); else generateSpin(true);
      settleSpin(true);
    } else {
      presentation.holdRespin = true; holdStep(scripted);
    }
    ++state.actions; presentation.actionNumber = state.actions; presentation.valid = true;
    presentation.afterMode = state.mode; presentation.balanceAfter = state.balance;
    presentation.freeAfter = state.freeLeft; presentation.respinsAfter = state.respinsLeft; presentation.meterAfter = state.meter;
    presentation.pendingHoldsAfter = state.pendingMeterHolds;
    presentation.actionWinMinor = state.win;
    return true;
  }
  bool luckyChanceImpl(const Cell* scripted) {
    capacityFaults_ = 0;
    if (!canLuckyChance()) return false;
    if (scripted) for (size_t i = 0; i < kCells; ++i) if (!validCell(scripted[i])) return false;
    const int64_t remaining = state.balance;
    // Check the possible winning branch before consuming the Lucky gate draw.
    if ((capacityFaults_ = actionCapacityFaults(Mode::Base,remaining,true)) != 0) { presentation = Presentation{}; return false; }
    const bool won = rng_.bounded(uint32_t(betMinor())) < uint64_t(remaining);
    if (won) {
      // Full configured reel/payout math, but debit ONLY remaining cents once.
      if (!actionImpl(scripted,remaining)) return false;
      ++state.stats.luckyAttempts; ++state.stats.luckyWins;
      presentation.luckyAttempt = presentation.luckyWon = true;
      return true;
    }
    presentation = Presentation{}; presentation.valid = true; presentation.luckyAttempt = true;
    presentation.beforeMode = presentation.afterMode = Mode::Base;
    presentation.balanceBefore = remaining; presentation.stakeMinor = remaining;
    presentation.meterBefore = presentation.meterAfter = state.meter;
    copyCells(presentation.before,state.cells);
    state.balance = state.win = state.featureWin = state.roundPayout = 0;
    state.freePlayed = 0; state.grandAwarded = false;
    for (Cell& cell : state.cells) cell.highlight = false;
    state.stats.stakedMinor += uint64_t(remaining); ++state.stats.luckyAttempts; ++state.actions;
    presentation.actionNumber = state.actions; presentation.roundCompleted = true;
    return true;
  }
};

static_assert(std::is_trivially_copyable<State>::value, "Engine state must stay CPU-only");
static_assert(kSaveCapacity <= 3072, "StickS3 bank buffer budget");
}  // namespace phoenix
