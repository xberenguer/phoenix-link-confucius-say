// Source-based lifecycle proposal. Included only by the final reviewed UI.
// No mutable state or independent musical/financial epoch is introduced.
bool twinFreeReturnMusic89(uint32_t now){
 if(phase!=Phase::ReturnWipe||engine.state.mode!=Mode::Free||
    !engine.state.freeSession||pendingChained||(pendingFree&&!freeRetriggerIntro()))return false;
 // The independent3pf recording restarts the first Free phrase at the hot
 // full-coverage part of the return fire, before Free2 moves. This maps its
 // observed occlusion boundary onto the existing retarget, not a vendor clock.
 constexpr uint32_t opaqueAt=feature_entry::fireDelayMs+phoenix_fx::wipeDurationMs*42/100;
 return uint32_t(now-phaseAt)>=collectionReturnLead()+opaqueAt;
}
bool twinFreeMusicWanted89(uint32_t now){
 if(twinFreeReturnMusic89(now))return true;
 if(phase==Phase::FreeIntro)return freeRetriggerIntro();
 if(phase==Phase::Ready)return engine.state.mode==Mode::Free||wisdomEntry30.stage==wisdom_ctrl30::Stage::Accepted;
 const bool freeResult=phase==Phase::Spin||phase==Phase::Land||phase==Phase::Expand||
                       phase==Phase::FreeCounter||phase==Phase::SpinAward||phase==Phase::SpinAwardSettle;
 // A final Free action commits Mode::Base before its strips start. Keep
 // music through that still-Free presentation instead of cutting its last spin.
 return freeResult&&engine.presentation.valid&&engine.presentation.freeSpin;
}
