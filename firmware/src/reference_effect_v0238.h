#pragma once
// Diagnostic-only baseline bodies copied verbatim from installed0.2.38.
// Called only by the explicit idle/offscreen parity benchmark, never normal
// gameplay. SourceSHA256:f58750c7efa641bd0dc75116c2f6588f2288aafb830f11f8bd5a3a320b9b82a0

void referenceCollectEmblem38(uint32_t elapsed){
 const auto& im=collect_art::emblem();if(!im.rgb||!im.alpha)return;
 const float progress=elapsed<kCollectedHold?0.f:std::min(1.f,float(elapsed-kCollectedHold)/kCollectedBurn);
 const unsigned tail=elapsed+100>kCollectedIntro?(elapsed>=kCollectedIntro?0:(kCollectedIntro-elapsed)*255/100):255;
 float windX[100],windY[100];
 for(unsigned r=0;r<100;++r)windX[r]=.65f*std::sin(r*.41f+elapsed*.0107f)+.35f*std::sin(r*.19f-elapsed*.0131f);
 for(unsigned c=0;c<100;++c)windY[c]=.75f*std::sin(c*.27f-elapsed*.0127f)+.40f*std::sin(c*.11f+elapsed*.017f);
 for(unsigned y=1;y+1<im.h;++y)for(unsigned x=1;x+1<im.w;++x){
  const float dx=(float(x)-50)/50,dy=(float(y)-51)/50;
  const float outer=std::max(0.f,std::min(1.f,(dx*dx+dy*dy-.28f)*2.6f));
  const float sx=x+windX[y]*outer,sy=y+windY[x]*outer;
  const int ix=int(std::floor(sx)),iy=int(std::floor(sy));
  if(ix<0||iy<0||ix+1>=im.w||iy+1>=im.h)continue;
  const unsigned u=unsigned((sx-ix)*256),v=unsigned((sy-iy)*256);
  const unsigned weights[]={(256-u)*(256-v),u*(256-v),(256-u)*v,u*v};
  const unsigned at=unsigned(iy)*im.w+unsigned(ix),offsets[]={at,at+1,at+im.w,at+im.w+1};
  uint32_t a=0,r=0,g=0,b=0;
  for(unsigned n=0;n<4;++n){const uint32_t w=weights[n]*im.alpha[offsets[n]];const auto color=im.rgb[offsets[n]];a+=w;r+=(color>>11)*w;g+=((color>>5)&63)*w;b+=(color&31)*w;}
  if(!a)continue;
  auto color=uint16_t(((r+a/2)/a)<<11|((g+a/2)/a)<<5|(b+a/2)/a);
  unsigned opacity=std::min(255u,(a+32768)>>16);
  if(progress>0){
   const float nx=(float(x)-im.w*.5f)/(im.w*.65f),ny=(float(y)-im.h*(.43f-.18f*progress))/(im.h*.70f);
   const float radius=1.05f*(1-progress),edge=radius*radius-nx*nx-ny*ny+windY[x]*.015f;
   opacity=unsigned(opacity*std::max(0.f,std::min(1.f,(edge+.05f)*10)))*tail/255;
   if(edge>-.05f&&edge<.1f)color=blend(0xffe5,color,180);
  }
  dot(70+int(x),18+int(y),color,opacity);
 }
}

void referenceEntryBird38(uint32_t elapsed){
 if(elapsed>440)return;
 if(!entry_art::ready()){returnBird(elapsed);return;}
 const auto& im=entry_art::sprite();
 const float top=feature_entry::birdTop(elapsed);
 constexpr int left=3,width=234,height=176;
 // Retarget the generated crest/neck to the measured40px head-to-wing rise.
 // The collection bird has a different head/pose and never uses this mesh.
 constexpr float wingY=40.f;
 const float sourceWing=2.f+(im.h-4)*.31f;
 const float time=elapsed*.001f;
 float horizontal[width];
 for(int x=0;x<width;++x)horizontal[x]=std::sin(x*.085f-time*23.f);
 for(int y=std::max(RY,int(std::floor(top)));y<std::min(RY+RH,int(std::ceil(top+height)));++y){
  const float ly=y-top;
  if(ly<0||ly>=height)continue;
  const float baseY=ly<wingY?ly*sourceWing/wingY:sourceWing+(ly-wingY)*(im.h-sourceWing)/(height-wingY);
  const float headWeight=std::max(0.f,1.f-ly/52.f);
  const float wave=std::sin(ly*.095f-time*27.f);
  const float trailing=std::max(0.f,std::min(1.f,(ly-wingY)/40.f));
  for(int x=0;x<width;++x){
   const float dx=x-width*.5f;
   const float headZone=std::max(0.f,1.f-std::abs(dx)/48.f);
   const float sx=(dx/(1.f-.35f*headWeight*headZone)+width*.5f)*im.w/width;
   const float feather=std::max(trailing*std::min(1.f,std::abs(dx)/50.f),std::max(0.f,(ly-100.f)/76.f));
   const float sampleX=sx+wave*.65f*feather,sampleY=baseY+horizontal[x]*.70f*feather;
   const int ix=int(std::floor(sampleX)),iy=int(std::floor(sampleY));
   if(ix<0||iy<0||ix+1>=im.w||iy+1>=im.h)continue;
   const unsigned u=unsigned((sampleX-ix)*256),v=unsigned((sampleY-iy)*256);
   const unsigned weights[]={(256-u)*(256-v),u*(256-v),(256-u)*v,u*v};
   const unsigned at=unsigned(iy)*im.w+unsigned(ix),offsets[]={at,at+1,at+im.w,at+im.w+1};
   uint32_t a=0,r=0,g=0,b=0;
   for(unsigned n=0;n<4;++n){
    const uint32_t w=weights[n]*im.alpha[offsets[n]];const uint16_t color=im.rgb[offsets[n]];
    a+=w;r+=(color>>11)*w;g+=((color>>5)&63)*w;b+=(color&31)*w;
   }
   if(!a)continue;
   // Premultiplied-alpha filtering avoids dark fringes at moving flame tips.
   const uint16_t color=uint16_t(((r+a/2)/a)<<11|((g+a/2)/a)<<5|(b+a/2)/a);
   dot(left+x,y,color,std::min(255u,(a+32768u)>>16));
  }
 }
}

