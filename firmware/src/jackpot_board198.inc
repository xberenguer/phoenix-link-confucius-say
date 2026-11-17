#include "jackpot_brand207.h"
// Current future-landing units, read without changing a cache or outcome.
bool jackpotBoardAmount198(unsigned tier,int64_t& out){
 int64_t cached=0;
 if(!engine.jackpotAmountMinor(tier,cached))return false;
 return phoenix::money::toMinor(engine.jackpotValueAmount(tier),engine.state.coinValue,1,out);
}
constexpr uint16_t jackpotColors207[4]={0xfa61,0x121f,0x2e63,0xf840};
void jackpotBold207(const char* label,int center,int y,unsigned size,uint16_t color){
 int x=center-(textWidth(label,size)+1)/2;const auto* opacity=pf::pixels(size);
 for(auto p=reinterpret_cast<const unsigned char*>(label);*p;++p){const auto& glyph=pf::glyph(size,*p);
  for(int r=0;r<glyph.h;++r)for(int c=0;c<=glyph.w;++c){
   const unsigned alpha=std::max(c<int(glyph.w)?unsigned(opacity[glyph.offset+r*glyph.w+c]):0u,c?unsigned(opacity[glyph.offset+r*glyph.w+c-1]):0u);
   if(alpha)dot(x+c,y+r,color,alpha);
  }
  x+=glyph.advance;
 }
}
void jackpotEmblem207(uint32_t now,int x,int y,int w,int h,unsigned tier,bool mirror){
 const uint32_t period=phoenix_art::periodMs();
 const auto& im=phoenix_art::frame(period?(now%period+tier*130u)%period:0);
 if(!im.rgb||!im.w||!im.h)return;
 for(int r=0;r<h;++r)for(int c=0;c<w;++c){
  const unsigned sourceX=unsigned(mirror?w-1-c:c)*im.w/unsigned(w);
  const unsigned at=unsigned(r)*im.h/unsigned(h)*im.w+sourceX;
  const unsigned alpha=im.alpha?im.alpha[at]:255;
  if(alpha)dot(x+c,y+r,im.rgb[at],alpha);
 }
}
void jackpotBadge207(uint32_t now,unsigned tier,const char* label,int x,int y,bool wide,bool mirror){
 const int w=wide?48:36,h=wide?36:27,cx=x+w/2;
 jackpotEmblem207(now,x,y,w,h,tier,mirror);
 const int rx=wide?15:12,ry=wide?10:8,cy=y+(wide?21:16);
 for(int dy=-ry;dy<=ry;++dy)for(int dx=-rx;dx<=rx;++dx)
  if(dx*dx*ry*ry+dy*dy*rx*rx<=rx*rx*ry*ry)dot(cx+dx,cy+dy,0x0800,225);
 const unsigned size=wide?8:7;const int labelY=y+(wide?12:8);
 jackpotBold207(label,cx+1,labelY+1,size,0x0000);
 jackpotBold207(label,cx,labelY,size,jackpotColors207[tier]);
 text(wide?"JACKPOT":"BONUS",cx,y+(wide?22:17),6,BRIGHT,1);
}
void jackpotFrame198(uint32_t now,int x,int y,int w,int h){
 // Original topper uses a flat dark field behind the jackpot amounts.
 fill(x,y,w,h,0x0800);
 const auto rim=[&](int inset,uint16_t color){
  const int xx=x+inset,yy=y+inset,ww=w-inset*2,hh=h-inset*2;
  line(xx+3,yy,xx+ww-4,yy,color);line(xx+3,yy+hh-1,xx+ww-4,yy+hh-1,color);
  line(xx,yy+3,xx,yy+hh-4,color);line(xx+ww-1,yy+3,xx+ww-1,yy+hh-4,color);
  line(xx,yy+3,xx+3,yy,color);line(xx+ww-4,yy,xx+ww-1,yy+3,color);
  line(xx,yy+hh-4,xx+3,yy+hh-1,color);line(xx+ww-4,yy+hh-1,xx+ww-1,yy+hh-4,color);
 };
 rim(0,0x8ba0);rim(1,GOLD);
 line(x+5,y+2,x+w-6,y+2,BRIGHT,175);
 line(x+5,y+h-3,x+w-6,y+h-3,0x52a0,210);
 for(int c=6;c<w-6;++c){
  const unsigned u=unsigned(c-6)*(counter_fire89::width-1)/unsigned(w-13);
  const unsigned flame=counter_fire89::contour((now%4000)/2,u)/55;
  if(flame>1)dot(x+c,y+3,counter_calm90::pigment(counter_fire89::flame(195)),100);
 }
}
struct JackpotAmountLayout207{int left,right,y;unsigned size;};
std::string jackpotBoardMoney207(int64_t minor,unsigned tier){
 const auto whole=std::to_string(minor/100);std::string result="$";
 for(size_t i=0;i<whole.size();++i){if(i&&(whole.size()-i)%3==0)result+=',';result+=whole[i];}
 const unsigned cents=unsigned(minor%100);
 if(tier>=2||cents){result+='.';result+=char('0'+cents/10);result+=char('0'+cents%10);}
 return result;
}
JackpotAmountLayout207 jackpotAmountLayout207(const char* amount,unsigned tier){
 const bool wide=tier>=2;const int left=wide?50:tier==1?40:126,right=wide?190:tier==1?114:200;
 unsigned size=wide?20:16;
 for(unsigned candidate:wide?std::initializer_list<unsigned>{20,16,10,8,7}:std::initializer_list<unsigned>{16,10,8,7}){
  size=candidate;if(cashWidth(amount,candidate)<=right-left-2)break;
 }
 return {left,right,(tier==3?37:tier==2?73:107)+((wide?20:16)-int(size))/2,size};
}
void jackpotCash207(const char* amount,const JackpotAmountLayout207& placement,unsigned tier){
 const unsigned scale=tier>=2&&cashWidth(amount,placement.size)*6<=(placement.right-placement.left-2)*5?6:5;
 const int width=(cashWidth(amount,placement.size)*int(scale)+2)/5;
 const int left=(placement.left+placement.right-width)/2,y=placement.y-(int(placement.size*scale/5)-int(placement.size))/2;
 const auto draw=[&](int dx,int dy,uint16_t color){int advance=0;const auto* alpha=pmoney::pixels(placement.size);
  for(auto p=(const unsigned char*)amount;*p;++p){const auto& g=pmoney::glyph(placement.size,*p);const int x=left+(advance*int(scale)+2)/5;
   for(unsigned r=0;r<(g.h*scale+4)/5;++r)for(unsigned c=0;c<(g.w*scale+4)/5;++c){const unsigned srcR=std::min<unsigned>(g.h-1,r*5/scale),srcC=std::min<unsigned>(g.w-1,c*5/scale);const auto opacity=alpha[g.offset+srcR*g.w+srcC];if(opacity)dot(x+int(c)+dx,y+int(r)+dy,color,opacity);}
   advance+=g.advance;
  }
 };
 draw(1,1,0x0000);draw(-1,0,0xff20);draw(1,0,0xff20);draw(0,-1,0xff20);draw(0,1,0xff20);draw(0,0,jackpotColors207[tier]);
}
void jackpotRow207(uint32_t now,unsigned tier,const char* label){
 const bool wide=tier>=2;const int top=tier==3?28:tier==2?64:100;
 if(wide)jackpotFrame198(now,20,top+4,200,31);
 else jackpotFrame198(now,tier==1?20:120,104,100,22);
 int64_t value=0;
 if(jackpotBoardAmount198(tier,value)){const auto amount=jackpotBoardMoney207(value,tier);jackpotCash207(amount.c_str(),jackpotAmountLayout207(amount.c_str(),tier),tier);}
 else text("UNAVAILABLE",wide?120:tier==1?77:163,top+(wide?14:11),7,WHITE,1);
 if(wide){jackpotBadge207(now,tier,label,0,top,true,false);jackpotBadge207(now,tier,label,192,top,true,true);}
 else jackpotBadge207(now,tier,label,tier==1?1:203,100,false,tier==0);
}
// Local reconstruction of the filmed letter-edge fire. The underlying
// publisher wordmark remains stationary; only its warm fringe is animated.
unsigned jackpotFlame143(unsigned x,unsigned step){
 uint32_t v=uint32_t(x+17)*0x45d9f3bu^uint32_t(step%32+1)*0x9e3779b9u;
 v^=v>>16;v*=0x45d9f3bu;return (v^(v>>16))&255u;
}
unsigned jackpotFlow143(unsigned x,uint32_t now){
 const unsigned tick=(now%2560)/80,fraction=(now%80)*255/80;
 return (jackpotFlame143(x,tick)*(255-fraction)+jackpotFlame143(x,tick+1)*fraction+127)/255;
}
void jackpotTopper207(uint32_t now){
 const auto& im=jackpot_brand207::image;constexpr int width=224,height=32,left=(W-width)/2;
 for(int c=0;c<width;++c){
  const unsigned flow=jackpotFlow143(unsigned(c/3),now);int top=-1;
  for(int r=0;r<height;++r){const unsigned at=unsigned(r)*im.h/height*im.w+unsigned(c)*im.w/width;
   if(!im.alpha[at])continue;
   const uint16_t pigment=im.rgb[at];const unsigned red=(pigment>>11)&31,green=(pigment>>5)&63,blue=pigment&31;
   const bool warm=red>15&&green>15&&green<red*2&&blue<12;
   if(warm&&im.alpha[at]>160&&top<0)top=r;
   dot(left+c,r,pigment,im.alpha[at]);
   if(warm){const unsigned light=jackpotFlow143(unsigned(c/3+r/4),now+unsigned(r)*24u);
    dot(left+c,r,light>150?0xff8c:0xfaa0,40+light/3);}
  }
  if(top>0&&top<height/2){
   const unsigned tongue=flow>192?3:flow>96?2:1;
   for(unsigned k=1;k<=tongue;++k){const int yy=top-int(k);if(yy<0)break;
    dot(left+c,yy,k==1?0xfd60:0xfb00,(unsigned(tongue+1-k)*(60+flow/2))/tongue);}
  }
 }
}
void drawJackpots198(uint32_t now){
 fill(0,0,W,H,0x0800);
 jackpotTopper207(now);
 jackpotRow207(now,3,"GRAND");jackpotRow207(now,2,"MAJOR");
 jackpotRow207(now,1,"MINOR");jackpotRow207(now,0,"MINI");
}
