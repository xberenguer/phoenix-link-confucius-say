// Export the implemented simulator tables. This does not recover casino PAR data.
// Build from the repository root: c++ -std=c++17 -O2 -Ifirmware/src tools/export_wiki_tables.cpp -o /tmp/export-wiki
// Run: /tmp/export-wiki docs/data
#include "phoenix_engine.h"
#include "cabinet_profile.h"
#include "static_grand_table.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <tuple>
struct ObservedRoute {unsigned count,id;std::array<uint8_t,5> path;};
constexpr ObservedRoute observedRoutes[]={
 {5,4,{{0,1,2,1,0}}},
 {15,8,{{1,2,2,2,1}}},
 {15,6,{{0,0,1,0,0}}},
 {25,1,{{1,1,1,1,1}}},
 {25,2,{{0,0,0,0,0}}},
 {25,3,{{2,2,2,2,2}}},
 {25,4,{{0,1,2,1,0}}},
 {25,5,{{2,1,0,1,2}}},
 {25,6,{{0,0,1,0,0}}},
 {25,7,{{2,2,1,2,2}}},
 {25,8,{{1,2,2,2,1}}},
 {25,9,{{1,0,0,0,1}}},
 {25,10,{{0,1,1,1,0}}},
 {25,11,{{2,1,1,1,2}}},
 {25,12,{{0,1,0,1,0}}},
 {25,13,{{2,1,2,1,2}}},
 {25,14,{{1,0,1,0,1}}},
 {25,15,{{1,2,1,2,1}}},
 {25,16,{{1,1,0,1,1}}},
 {25,17,{{1,1,2,1,1}}},
 {25,18,{{0,2,0,2,0}}},
 {25,19,{{2,0,2,0,2}}},
 {25,20,{{1,0,2,0,1}}},
 {25,21,{{1,2,0,2,1}}},
 {25,22,{{0,0,2,0,0}}},
 {25,23,{{2,2,0,2,2}}},
 {25,24,{{0,2,2,2,0}}},
 {25,25,{{2,0,0,0,2}}},
 {50,1,{{1,1,1,1,1}}},
 {50,2,{{0,0,0,0,0}}},
 {50,3,{{2,2,2,2,2}}},
 {50,4,{{0,1,2,1,0}}},
 {50,5,{{2,1,0,1,2}}},
 {50,6,{{0,0,1,0,0}}},
 {50,7,{{2,2,1,2,2}}},
 {50,8,{{1,2,2,2,1}}},
 {50,9,{{1,0,0,0,1}}},
 {50,10,{{0,1,1,1,0}}},
 {50,11,{{2,1,1,1,2}}},
 {50,12,{{0,1,0,1,0}}},
 {50,13,{{2,1,2,1,2}}},
 {50,14,{{1,0,1,0,1}}},
 {50,15,{{1,2,1,2,1}}},
 {50,16,{{1,1,0,1,1}}},
 {50,17,{{1,1,2,1,1}}},
 {50,18,{{0,2,0,2,0}}},
 {50,19,{{2,0,2,0,2}}},
 {50,20,{{1,0,2,0,1}}},
 {50,21,{{1,2,0,2,1}}},
 {50,22,{{0,0,2,0,0}}},
 {50,23,{{2,2,0,2,2}}},
 {50,24,{{0,2,2,2,0}}},
 {50,25,{{2,0,0,0,2}}},
 {50,26,{{0,2,1,2,0}}},
 {50,27,{{2,0,1,0,2}}},
 {50,28,{{1,1,1,1,2}}},
 {50,29,{{0,0,1,2,2}}},
 {50,30,{{2,2,1,0,0}}},
 {50,32,{{2,1,1,1,0}}},
 {50,34,{{2,1,0,1,0}}},
 {50,35,{{0,0,0,0,1}}},
 {50,36,{{2,2,2,2,1}}},
 {50,37,{{0,1,0,1,2}}},
 {50,38,{{2,1,2,1,0}}},
 {50,40,{{1,2,1,0,1}}},
 {50,41,{{1,1,0,0,0}}},
 {50,42,{{1,1,2,2,2}}},
 {50,43,{{1,0,0,1,2}}},
 {50,44,{{1,2,2,1,0}}},
 {50,46,{{1,2,1,0,0}}},
 {50,47,{{2,1,0,0,1}}},
 {50,49,{{0,0,1,2,1}}},
 {50,50,{{2,2,1,0,1}}},
 {100,1,{{1,1,1,1,1}}},
 {100,2,{{0,0,0,0,0}}},
 {100,3,{{2,2,2,2,2}}},
 {100,6,{{0,0,1,0,0}}},
 {100,8,{{1,2,2,2,1}}},
 {100,9,{{1,0,0,0,1}}},
 {100,12,{{0,1,0,1,0}}},
 {100,15,{{1,2,1,2,1}}},
 {100,16,{{1,1,0,1,1}}},
 {100,17,{{1,1,2,1,1}}},
 {100,18,{{0,2,0,2,0}}},
 {100,23,{{2,2,0,2,2}}},
 {100,25,{{2,0,0,0,2}}},
 {100,36,{{2,2,2,2,1}}},
 {100,37,{{0,1,0,1,2}}},
 {100,43,{{1,0,0,1,2}}},
 {100,90,{{2,0,0,0,1}}},
};
const char* geometryEvidence(unsigned count,unsigned id,const std::array<uint8_t,5>& path){for(const auto& x:observedRoutes)if(x.count==count&&x.id==id&&x.path==path)return count==25?"source_reconstructed_US25_chart":"source_observed_geometry_matches";return "local_fallback";}
bool currentMenuOffered(int64_t coin,unsigned lines,unsigned multiplier){
 if(cabinet_profile::offered(coin,lines,multiplier))return true;
 for(const auto& c:cabinet_profile::highLimitCoins)if(c.minor==coin)
  for(unsigned r=0;r<c.count;++r)if(c.lines[r]==lines)
   for(unsigned m=0;m<c.rows[r].count;++m)if(c.rows[r].multipliers[m]==multiplier)return true;
 return false;
}
int main(int argc,char**argv){
 if(argc!=2){std::cerr<<"Usage: export-wiki OUTPUT_DIRECTORY\n";return 2;}
 const std::filesystem::path output=argv[1];std::filesystem::create_directories(output);
 struct SymbolInfo{phoenix::Symbol symbol;const char* name;};
 const std::array<SymbolInfo,10> symbols={{{phoenix::Symbol::Nine,"9"},{phoenix::Symbol::Ten,"10"},
  {phoenix::Symbol::Jack,"J"},{phoenix::Symbol::Queen,"Q"},{phoenix::Symbol::King,"K"},
  {phoenix::Symbol::Bell,"Bell"},{phoenix::Symbol::Abacus,"Abacus"},{phoenix::Symbol::Book,"Book"},
  {phoenix::Symbol::Candle,"Candle"},{phoenix::Symbol::Wild,"Confucius Wild"}}};
 using Wager=std::tuple<int64_t,unsigned,unsigned>;
 std::set<Wager> lookupOffered,menuOffered;
 std::ofstream pays(output/"implemented-line-pays.csv");
 pays<<"denomination_minor,selected_lines,bet_multiplier,mode,symbol,match_count,unit_credit_pay,line_credit_pay,line_money_minor,offered_by_current_menu\n";
 unsigned payRows=0;
 for(auto coin:phoenix::local::denominations)for(auto lines:phoenix::local::lineOptions)for(auto mult:phoenix::local::betMultipliers)for(bool free:{false,true})for(const auto& info:symbols)for(unsigned count=2;count<=5;++count){
  const auto unit=phoenix::local::linePay(unsigned(info.symbol),count,mult,free,lines,coin);
  const int64_t credits=int64_t(unit)*mult;
  const bool offered=currentMenuOffered(coin,lines,mult);
  if(offered)lookupOffered.emplace(coin,lines,mult);
  pays<<coin<<','<<unsigned(lines)<<','<<unsigned(mult)<<','<<(free?"free":"base")<<','<<info.name<<','<<count<<','<<unit<<','<<credits<<','<<credits*coin<<','<<(offered?"true":"false")<<'\n';++payRows;
 }
 std::ofstream paths(output/"implemented-paylines.csv");paths<<"line_selection,line_number,reel_1_row,reel_2_row,reel_3_row,reel_4_row,reel_5_row,evidence_scope\n";
 unsigned pathRows=0;
 for(auto lines:phoenix::local::lineOptions)for(unsigned line=0;line<lines;++line){
  const auto& p=phoenix::local::linePath(line,lines);paths<<unsigned(lines)<<','<<line+1;
  for(unsigned c=0;c<5;++c)paths<<','<<unsigned(p[c]);
  paths<<','<<geometryEvidence(lines,line+1,p)<<'\n';++pathRows;
 }
 std::ofstream deck(output/"offered-wagers.csv");deck<<"denomination_minor,selected_lines,bet_multiplier,stake_credits,stake_money_minor,menu_mode\n";
 unsigned offered=0;
 auto writeDeck=[&](const auto& coins,const char* mode){
  for(const auto&coin:coins)for(unsigned row=0;row<coin.count;++row)for(unsigned b=0;b<coin.rows[row].count;++b){
   const unsigned mult=coin.rows[row].multipliers[b],lines=coin.lines[row];const int64_t stake=int64_t(lines)*mult;
   if(!menuOffered.emplace(coin.minor,lines,mult).second){std::cerr<<"Duplicate menu wager\n";return false;}
   deck<<coin.minor<<','<<lines<<','<<mult<<','<<stake<<','<<stake*coin.minor<<','<<mode<<'\n';++offered;
  }
  return true;
 };
 if(!writeDeck(cabinet_profile::coins,"standard")||!writeDeck(cabinet_profile::highLimitCoins,"high_limit"))return 4;
 // These are selectable prices only. No old numeric return/edge enclosure is
 // exported as current after the1887 counter-law correction.
 std::ofstream grand(output/"retained-grand-prices.csv");
 grand<<"denomination_minor,selected_lines,bet_multiplier,prize_cap,internal_slot,grand_credits,current_return_qualified\n";
 unsigned grandRows=0;
 for(const auto& row:static_grand_table::entries){
  if(!static_grand::qualified(row)||static_grand::hasQualifiedReturn(row))return 5;
  grand<<row.profile.coinMinor<<','<<unsigned(row.profile.lines)<<','<<unsigned(row.profile.multiplier)<<','
   <<(row.profile.cap==phoenix::PrizeCapProfile::HighCashOnly?"cash":"named")<<','<<unsigned(row.slot)<<','<<row.grandCredits<<",false\n";++grandRows;
 }
 pays.close();paths.close();deck.close();grand.close();if(!pays||!paths||!deck||!grand||grandRows!=std::size(static_grand_table::entries))return 3;
 std::cout<<"{\"engine_model\":"<<phoenix::local::modelId<<",\"line_pay_rows\":"<<payRows<<",\"active_line_paths\":"<<pathRows<<",\"offered_wagers\":"<<offered<<",\"retained_grand_price_rows\":"<<grandRows<<",\"current_return_qualified\":false}\n";
 unsigned expectedPaths=0;for(auto lines:phoenix::local::lineOptions)expectedPaths+=lines;
 const size_t expectedPays=std::size(phoenix::local::denominations)*std::size(phoenix::local::lineOptions)*std::size(phoenix::local::betMultipliers)*2*symbols.size()*4;
 return payRows==expectedPays&&pathRows==expectedPaths&&menuOffered==lookupOffered&&offered==menuOffered.size()?0:4;
}
