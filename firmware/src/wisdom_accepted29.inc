// Requires the existing freeView() declaration and current UI phase state.
// Observed initial START has already removed the pair rails and announced
// BONUS REELS IN PLAY before any first-free spin pixels move. The paid
// trigger presentation has twinSpin=false, so it cannot gate this interval.
// No board, prize, wager, Engine state or input controller is changed here.
bool startedInitialFreeReady29(){
 return engine.presentation.valid&&freeView()&&!(phase==Phase::FreeIntro&&!freeRetriggerIntro());
}
bool pairedReelsAccepted29(){
 return !holdView&&((engine.presentation.valid&&engine.presentation.twinSpin)||startedInitialFreeReady29());
}
bool completedFreeBoard29(){
 return !holdView&&engine.presentation.valid&&engine.presentation.twinSpin&&engine.presentation.freeEnded&&
        (phase==Phase::Outro||phase==Phase::Ready);
}
bool bonusReelsActive29(){
 return !holdView&&startedInitialFreeReady29()&&!completedFreeBoard29();
}
