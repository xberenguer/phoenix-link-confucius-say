// Frozen0.2.58fire/composition oracle, for offscreen parity only.
void referenceFireWipe58(uint32_t elapsed,uint32_t duration=phoenix_fx::wipeDurationMs){
 if(!elapsed||!duration||elapsed>=duration)return;
 const bool cached=effectsCacheReady&&effectsUseCache;
 const uint16_t* source=cached?preparedEffects.fireAt(elapsed):nullptr;
 const float t=float(elapsed)/duration;
 if(cached&&t>=.42f&&t<.65f){
  // Fully opaque fire hides the entire display, including both HUD bars.
  // A contiguous copy avoids evaluating an unnecessary blend per texel.
  if(dotClipTop==0&&dotClipBottom==H)memcpy(pixels,source,sizeof(pixels));
  else for(int y=std::max(0,dotClipTop);y<std::min(H,dotClipBottom);++y)memcpy(pixels+y*W,source+y*W,size_t(W)*2);
  return;
 }
 // Avoid rebuilding the expensive deformation grid on the cached path.
 alignas(fire_field::Flow) unsigned char fieldStorage[sizeof(fire_field::Flow)];
 const auto* field=cached?nullptr:new(fieldStorage)fire_field::Flow(effect_frames::sample(elapsed),&fireWorkspace);
 float fringe[W];for(int x=0;x<W;++x)fringe[x]=.038f*std::sin(x*.13f+elapsed*.011f)+.022f*std::sin(x*.31f-elapsed*.019f);
 for(int y=0;y<H;++y){for(int x=0;x<W;++x){
   // Independent footage shows fire passing over the header, footer and
   // outer reel edges. Only ordinary reel symbols retain viewport clipping.
   uint16_t flame=0;
   float enter=0; // opacity is independent of game state/RNG.
   if(t<.42f){const float reach=t/.42f*1.4f;enter=(reach-(float(H-y)/H+std::abs(x-W/2.f)/W*.38f+fringe[x]))/.09f;if(enter<=0)continue;}
   else if(t<.65f)enter=1;
   else {
    // Break up the departing edge with moving tongues and the fire's own
    // luminance, rather than clearing the screen with a horizontal crop bar.
    flame=cached?source[y*W+x]:field->pixel(fireX[x],fireY[y]);
    const float heat=float((flame>>5)&63)/63.f;
    enter=(float(H-y)/H-(t-.65f)/.35f+fringe[x]+(heat-.5f)*.05f)/.10f;
   }
   if(t<.65f)flame=cached?source[y*W+x]:field->pixel(fireX[x],fireY[y]);
   const unsigned alpha=unsigned(std::max(0.f,std::min(1.f,enter))*255);if(alpha)dot(x,y,flame,alpha);
 }}
 if(field)field->~Flow();
}

void referenceRasterScene58(uint32_t now){image(pa::backdrop,0,0);drawHeader(now);drawReels(now);drawFooter();drawTransition(now);if(fountainActive&&view==View::Game)coinShower(now-fountainAt);if(view==View::Menu)drawMenu();else if(view==View::Bet)drawBet();else if(view==View::Rules)drawRules();else if(view==View::Options)drawOptions();else if(view==View::Reset)drawReset();else if(view==View::Lucky)drawLucky();else if(view==View::Sound)drawSound();else if(view==View::Volume)drawVolume();if(*statusLine&&view==View::Game&&phase==Phase::Ready){fill(33,109,174,11,0x0000,225);text(statusLine,120,111,7,BRIGHT,1);}}
