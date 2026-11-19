// Real public-menu navigation supplement. No fixture state or value editing.
#define main capture_gameplay_original_main
#include "capture_gameplay.cpp"
#undef main
#include "phoenix_engine.h"

int main(int argc,char**argv){
 if(argc<2)return 2;Recorder record(argv[1]);phoenix::random::Service random;
 if(!random.initialize(freshEntropy,nullptr)||!uiInitFresh(random))return 3;
 uiSetOutcomeCheckpointHandler(owner);audioInit(false);uiPrepareEffects();
 auto press=[&](char button,const char*label){if(button=='A')uiButtonA();else if(button=='B')uiButtonB();else if(button=='M')uiMenu();else if(button=='T')uiToggleAuto();else if(button=='C')uiCabinetMenu();record.tick();record.log(label);};
 record.tick();record.log("fresh-start");record.still("startup-standard");
 press('T',"hold-blue-switch-high-limit");record.still("startup-high-limit");press('T',"hold-blue-return-standard");
 press('A',"blue-select-denomination");record.still("startup-bet");
 for(unsigned i=0;i<30&&field(uiSnapshot(),"view")!="game";++i){press('A',"blue-select-option");if(field(uiSnapshot(),"view")=="game")break;press('B',"grey-next-option");}
 if(field(uiSnapshot(),"view")!="game")return 4;
 press('A',"blue-open-credit-voucher");record.still("credit-voucher");press('A',"blue-increase-voucher-digit-to-1000-dollars");for(unsigned i=0;i<6;++i)press('B',"grey-next-voucher-field");record.still("credit-voucher-filled");press('A',"blue-insert-play-money-voucher");
 press('M',"hold-grey-open-menu");record.still("menu");press('A',"blue-open-balance");record.still("balance");press('M',"hold-grey-return-menu");
 press('B',"grey-select-game-rules");press('A',"blue-open-game-rules");for(unsigned page=0;page<6;++page){record.still("rules-"+std::to_string(page+1));press('B',"grey-next-rules-page");}press('M',"hold-grey-return-menu");
 press('B',"grey-select-paylines");press('A',"blue-open-paylines");std::vector<unsigned> layouts{unsigned(number(uiSnapshot(),"lines"))};for(auto count:phoenix::local::lineOptions)if(count!=layouts[0])layouts.push_back(count);
 for(auto lines:layouts){for(unsigned page=0;page<(lines+5)/6;++page){record.still("paylines-"+std::to_string(lines)+"-"+std::to_string(page+1));press('B',"grey-next-payline-page");}press('A',"blue-next-payline-layout");}press('M',"hold-grey-return-menu");
 press('B',"grey-select-counter-jackpots");press('A',"blue-open-counter-jackpots");record.still("game-values");press('A',"blue-open-counter-editor");record.still("counter-editor-unmodified");press('M',"hold-grey-leave-editor-without-edit");press('B',"grey-select-jackpot-picker");press('A',"blue-open-jackpot-picker");record.still("jackpot-picker");
 for(const char*tier:{"mini","minor","major","grand"}){press('A',"blue-view-jackpot-editor");record.still(std::string("jackpot-editor-")+tier+"-unmodified");press('M',"hold-grey-leave-editor-without-edit");press('B',"grey-next-jackpot-tier");}
 press('M',"hold-grey-return-game-values");press('M',"hold-grey-return-menu");press('B',"grey-select-settings");press('A',"blue-open-settings");record.still("settings");press('A',"blue-open-sound");record.still("sound");press('B',"grey-next-sound-option");press('B',"grey-select-volume");press('A',"blue-open-volume");record.still("volume");press('M',"hold-grey-return-sound");press('M',"hold-grey-return-settings");press('B',"grey-select-display");press('A',"blue-open-display");record.still("display");press('M',"hold-grey-return-settings");press('M',"hold-grey-return-menu");
 press('B',"grey-select-simulator-return");press('A',"blue-open-return-list");record.still("simulator-return-list");
 // The fresh game's retained default is displayed as option 3 (cursor 2).
 // Cycle through the eight list choices to option 1 without applying any.
 for(unsigned i=0;i<6;++i)press('B',"grey-cycle-return-focus-to-first-option");
 for(unsigned option=0;option<6;++option){press('A',"blue-view-return-detail-without-applying");record.still("return-detail-"+std::to_string(option+1));press('B',"grey-return-without-applying");press('B',"grey-next-return-option");}press('A',"blue-open-return-help");record.still("return-help");press('M',"hold-grey-return-list");press('M',"hold-grey-return-menu");
 std::ofstream summary(record.out/"summary.json");summary<<"{\"kind\":\"actual-public-menu-navigation\",\"journal_entries\":"<<record.inputIndex<<",\"engine_actions\":"<<uiActionCount()<<",\"rng_healthy\":"<<(uiRandomHealthy()?"true":"false")<<",\"all_payline_layouts\":[5,10,15,25,50,100],\"counter_or_jackpot_values_edited\":false,\"return_option_applied\":false}\n";return uiRandomHealthy()?0:5;
}
