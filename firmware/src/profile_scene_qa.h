#pragma once
UiMathProfileTest uiTestMathProfile(){
 UiMathProfileTest result;
 if(runtimeFinancialBlocked()||!initialized||view!=View::Game||phase!=Phase::Ready||autoPlay||
    engine.state.mode!=Mode::Base||!engine.canChangeBet())return result;
#ifdef UI_HOST_TEST
 void* memory=std::malloc(sizeof(phoenix::Engine));
#else
 void* memory=ps_malloc(sizeof(phoenix::Engine));
#endif
 if(!memory)return result;
 auto hash=[](){const auto* p=reinterpret_cast<const uint8_t*>(&engine);uint32_t h=2166136261u;for(size_t n=0;n<sizeof(engine);++n)h=(h^p[n])*16777619u;return h;};
 result.beforeHash=hash();auto* clone=new(memory)phoenix::Engine(engine);
 if(clone->setWager(50,25,1)){
  // Reproduce the active imported40/100-credit cache; never patch live saves.
  clone->state.jackpotCoins[0]=40;clone->state.jackpotCoins[1]=100;
  result.model=phoenix::local::modelId;result.miniCredits=clone->jackpotValueCoins(0);
  result.minorCredits=clone->jackpotValueCoins(1);result.betMinor=clone->betMinor();
  Cell mini,minor;mini.symbol=minor.symbol=Symbol::Phoenix;
  mini.kind=phoenix::PrizeKind::Mini;mini.prize=result.miniCredits;
  minor.kind=phoenix::PrizeKind::Minor;minor.prize=result.minorCredits;minor.multiplier=3;
  result.holdMinor=uint32_t((phoenix::Engine::cellCredits(mini)+phoenix::Engine::cellCredits(minor)+10*25)*clone->state.coinValue);
  result.roundMinor=result.holdMinor+2500;result.ran=true;
  // Source-confirmed line awards are evaluated with the clone's real wager
  // context. No spin, RNG draw, credit transfer or live state mutation.
  auto quote=[&](Symbol symbol,unsigned count,bool free){
   Cell board[15]{};for(auto& cell:board)cell.symbol=Symbol::Scatter;
   const auto& path=phoenix::local::linePath(0,clone->state.lineCount);
   for(unsigned column=0;column<count;++column)board[column*3+path[column]].symbol=symbol;
   return uint32_t(phoenix::Engine::evaluateLine(board,0,clone->state.betMultiplier,free,
                                               clone->state.lineCount,clone->state.coinValue).credits);
  };
  result.baseKing3=quote(Symbol::King,3,false);result.freeKing5=quote(Symbol::King,5,true);
  result.baseCandle5=quote(Symbol::Candle,5,false);result.baseJack4=quote(Symbol::Jack,4,false);
  result.baseQueen4=quote(Symbol::Queen,4,false);result.freeBell3=quote(Symbol::Bell,3,true);
  result.freeBook3=quote(Symbol::Book,3,true);result.freeJack3=quote(Symbol::Jack,3,true);
  result.freeJack5=quote(Symbol::Jack,5,true);result.freeTen5=quote(Symbol::Ten,5,true);
  result.freeCandle5=quote(Symbol::Candle,5,true);result.linePathFNV=2166136261u;
  for(unsigned line=0;line<25;++line)for(auto row:phoenix::local::linePath(line,25))
   result.linePathFNV=(result.linePathFNV^row)*16777619u;
 }
 clone->~Engine();std::free(memory);result.afterHash=hash();result.unchanged=result.beforeHash==result.afterHash;return result;
}
