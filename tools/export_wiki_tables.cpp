// Export the implemented simulator tables. This does not recover casino PAR data.
// Build from the repository root: c++ -std=c++17 -O2 -Ifirmware/src tools/export_wiki_tables.cpp -o /tmp/export-wiki
// Run: /tmp/export-wiki docs/data
#include "phoenix_engine.h"
#include "cabinet_profile.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <tuple>
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
  const bool offered=cabinet_profile::offered(coin,lines,mult);
  if(offered)lookupOffered.emplace(coin,lines,mult);
  pays<<coin<<','<<unsigned(lines)<<','<<unsigned(mult)<<','<<(free?"free":"base")<<','<<info.name<<','<<count<<','<<unit<<','<<credits<<','<<credits*coin<<','<<(offered?"true":"false")<<'\n';++payRows;
 }
 std::ofstream paths(output/"implemented-paylines.csv");paths<<"line_selection,line_number,reel_1_row,reel_2_row,reel_3_row,reel_4_row,reel_5_row,evidence_scope\n";
 unsigned pathRows=0;
 for(auto lines:phoenix::local::lineOptions)for(unsigned line=0;line<lines;++line){
  const auto& p=phoenix::local::linePath(line,lines);paths<<unsigned(lines)<<','<<line+1;
  for(unsigned c=0;c<5;++c)paths<<','<<unsigned(p[c]);
  paths<<','<<(lines==25?"source_reconstructed_US25_chart":lines==15&&line==5?"source_observed_US15_line6":lines==50&&line==21?"source_observed_US50_line22":"local_fallback")<<'\n';++pathRows;
 }
 std::ofstream deck(output/"offered-wagers.csv");deck<<"denomination_minor,selected_lines,bet_multiplier,stake_credits,stake_money_minor\n";
 unsigned offered=0;
 for(const auto&coin:cabinet_profile::coins)for(unsigned row=0;row<coin.count;++row)for(unsigned b=0;b<coin.rows[row].count;++b){
  const unsigned mult=coin.rows[row].multipliers[b],lines=coin.lines[row];const int64_t stake=int64_t(lines)*mult;
  if(!menuOffered.emplace(coin.minor,lines,mult).second){std::cerr<<"Duplicate menu wager\n";return 4;}
  deck<<coin.minor<<','<<lines<<','<<mult<<','<<stake<<','<<stake*coin.minor<<'\n';++offered;
 }
 pays.close();paths.close();deck.close();if(!pays||!paths||!deck)return 3;
 std::cout<<"{\"engine_model\":"<<phoenix::local::modelId<<",\"line_pay_rows\":"<<payRows<<",\"active_line_paths\":"<<pathRows<<",\"offered_wagers\":"<<offered<<"}\n";
 unsigned expectedPaths=0;for(auto lines:phoenix::local::lineOptions)expectedPaths+=lines;
 const size_t expectedPays=std::size(phoenix::local::denominations)*std::size(phoenix::local::lineOptions)*std::size(phoenix::local::betMultipliers)*2*symbols.size()*4;
 return payRows==expectedPays&&pathRows==expectedPaths&&menuOffered==lookupOffered&&offered==menuOffered.size()?0:4;
}
