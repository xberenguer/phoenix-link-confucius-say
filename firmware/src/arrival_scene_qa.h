#pragma once
// Included once after the production UI definitions. Diagnostic audio only;
// no fabricated game state, engine action, credit change or NVS write.
UiArrivalSceneTest uiTestArrivalScene(){
 UiArrivalSceneTest result;
#if !defined(UI_HOST_TEST) || defined(UI_NATIVE_ARRIVAL_TEST)
 if(runtimeFinancialBlocked()||!initialized||view!=View::Game||phase!=Phase::Ready||autoPlay||
    engine.state.mode!=Mode::Base||engine.state.roundOpen||fountainActive||
    winCuePending||!landingSounds||!audioReady()||audioMuted()||!audioVolume()||
    audioFeatureBusy()||audioLandingsBusy()||audioDiagnostics().pending||
    M5.Speaker.isPlaying())return result;
 const auto music=audioMusicDiagnostics();if(music.wanted||music.playing)return result;
 const AudioCue cues[]={AudioCue::PhoenixLand,AudioCue::PhoenixFollow,AudioCue::PhoenixThird};
 const uint32_t at[]={0,864,1218};
 for(unsigned i=0;i<3;++i){const auto clip=audioCueInfo(cues[i]);if(!clip.registered)return result;
  result.frames[i]=clip.frames;result.rates[i]=clip.rate;result.pcmFNV[i]=clip.fnv;}
 auto hash=[](){const auto* bytes=reinterpret_cast<const uint8_t*>(&engine);uint32_t h=2166136261u;for(size_t i=0;i<sizeof(engine);++i)h=(h^bytes[i])*16777619u;return h;};
 result.beforeHash=hash();const auto before=audioDiagnostics();const uint32_t began=millis();
 for(unsigned i=0;i<3;++i){
  while(uint32_t(millis()-began)<at[i]){audioUpdate(millis());delay(1);}
  const uint32_t now=millis();result.accepted+=audioQueueLanding(cues[i],now,now);
 }
 const uint32_t finishAt=at[2]+audioCueDurationMs(cues[2]);
 while(uint32_t(millis()-began)<finishAt||
       (uint32_t(millis()-began)<finishAt+1000&&audioLandingsBusy())){audioUpdate(millis());delay(1);}
 const auto after=audioDiagnostics();result.hardwareQueued=after.hardwareQueued-before.hardwareQueued;
 result.failures=after.failed-before.failed;result.pending=after.pending;
 result.durationMs=millis()-began;result.afterHash=hash();result.unchanged=result.beforeHash==result.afterHash;result.ran=true;
#endif
 return result;
}
