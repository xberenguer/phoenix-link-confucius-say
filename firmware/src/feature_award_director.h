#pragma once
#include <stdint.h>
// Camera-fitted presentation landmarks, not original manufacturer timers.
// l874jFsUyV8:354.94s completion flourish;360s award title;365s rising
// bird;368.6s amount;372.5s coins;378.5s cash collection. See review.json.
namespace feature_award {
// The upper-display precollection effect must remain visible on the Stick.
// Retarget its old3800ms header burn across the full playfield; keep the
// first ordinary pickup clock unchanged. Large/named preludes stay separate.
constexpr uint32_t ordinarySweepAt=3800;
constexpr uint32_t introMs=5100;
constexpr uint32_t birdAt=5100,pickupAt=7900,amountAt=8600;
constexpr uint32_t revealBurnAt=10000,revealEnd=11400;
// Relative to the354.945s clean completion-cue origin: the clean award
// phrase begins372.696s, first coins372.733s, first cash flare378.633s.
constexpr uint32_t musicAt=12650,fountainAt=12700;
constexpr uint32_t exitBurnAt=16900,durationMs=18400;
constexpr uint32_t cashLeadAfterAward=200;
// The eleven ordinary cash cells after the filmed MAJOR finish with a
// header burn at394.4..395.6s. Preserve the separate normal/large endings.
constexpr uint32_t namedCashTailMs=1100,namedHeaderFireMs=1200;
// Independent large-cash ending, si4wPYu_5co: completion cue103.565s in
//1400..1510clip; matching ending clip has exact106s overlap. Bird starts
//~9.4s after cue, first cash flare~12.2s. The later fast pickups are not
// used to infer the ordinary cadence or an original auto-skip rule.
constexpr uint32_t largeBirdAt=9400,largeCollectLead=12200;
constexpr uint32_t normalHoldMs=6000,bigHoldMs=8000;
// The two native source intervals overlap at534..567ms from complete
// lower WIN to visible returning crest. A520ms lead plus one40ms native
// render frame targets560ms. Keep the old200ms display-roll approximation;
// only move it before that settled interval. The source prefix law is unknown.
constexpr uint32_t normalWinCountMs=200,normalWinSettleMs=520;
constexpr uint32_t normalWinTransferMs=normalHoldMs-normalWinSettleMs;
inline int64_t normalWinCount(int64_t total,uint32_t elapsed){
 if(total<=0)return 0;
 const uint32_t start=normalWinTransferMs-normalWinCountMs;
 if(elapsed<=start)return 0;
 const uint32_t age=elapsed-start;
 if(age>=normalWinCountMs)return total;
 // Preserve exact cents without multiplying a large committed award by age.
 return (total/normalWinCountMs)*age+(total%normalWinCountMs)*age/normalWinCountMs;
}
constexpr uint32_t bigMusicAt=1800,bigFountainAt=2200;
// Provisional presentation policy, not a recovered game rule. Wave22 found
// coins and the full phrase at50x (including a Mini) and259x (with a Minor).
// The filmed22x Hold is intermediate Free; the old35x read is39x combined.
// Cropped views/mixed audio cannot establish a universal lower cutoff.
inline bool cashFountain(int64_t cash,int64_t bet){return bet>0&&cash/bet>=50;}
}
