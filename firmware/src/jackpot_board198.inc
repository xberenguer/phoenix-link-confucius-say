#include "jackpot_brand207.h"
// Current future-landing units, read without changing a cache or outcome.
bool jackpotBoardAmount198(unsigned tier,int64_t& out){
 int64_t cached=0;
 if(!engine.jackpotAmountMinor(tier,cached))return false;
 return phoenix::money::toMinor(engine.jackpotValueAmount(tier),engine.state.coinValue,1,out);
}
constexpr uint16_t jackpotColors207[4]={0xfc43,0x229f,0x05a3,0xe122};
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
void jackpotFrame198(uint32_t now,int x,int y,int w,int h,uint16_t top,uint16_t bottom){
 for(int r=0;r<h;++r){const int inset=r<3?3-r:r>=h-3?r-h+4:0;
  fill(x+inset,y+r,w-inset*2,1,counter_fire89::mix565(top,bottom,unsigned(r)*255/unsigned(h-1)));}
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
 return {left,right,(tier==3?33:tier==2?69:103)+((wide?20:16)-int(size))/2,size};
}
void jackpotCash207(const char* amount,const JackpotAmountLayout207& placement,unsigned tier){
 const int right=(placement.left+placement.right+cashWidth(amount,placement.size))/2;
 cashText(amount,right+1,placement.y+1,placement.size,0x0000);
 constexpr uint16_t rim=0xb560;
 cashText(amount,right-1,placement.y,placement.size,rim);
 cashText(amount,right+1,placement.y,placement.size,rim);
 cashText(amount,right,placement.y-1,placement.size,rim);
 cashText(amount,right,placement.y+1,placement.size,rim);
 cashText(amount,right,placement.y,placement.size,jackpotColors207[tier]);
}
void jackpotRow207(uint32_t now,unsigned tier,const char* label){
 const bool wide=tier>=2;const int top=tier==3?24:tier==2?60:96;
 constexpr uint16_t panelTop[4]={0x5840,0x0010,0x0240,0x5800};
 constexpr uint16_t panelBottom[4]={0x1800,0x0003,0x0060,0x1800};
 if(wide)jackpotFrame198(now,20,top+4,200,31,panelTop[tier],panelBottom[tier]);
 else jackpotFrame198(now,tier==1?20:120,98,100,26,panelTop[tier],panelBottom[tier]);
 int64_t value=0;
 if(jackpotBoardAmount198(tier,value)){const auto amount=jackpotBoardMoney207(value,tier);jackpotCash207(amount.c_str(),jackpotAmountLayout207(amount.c_str(),tier),tier);}
 else text("UNAVAILABLE",wide?120:tier==1?77:163,top+(wide?14:11),7,WHITE,1);
 if(wide){jackpotBadge207(now,tier,label,0,top,true,false);jackpotBadge207(now,tier,label,192,top,true,true);}
 else jackpotBadge207(now,tier,label,tier==1?1:203,96,false,tier==0);
}
void jackpotTopper207(uint32_t now){
 const auto& im=jackpot_brand207::image;const int left=(W-int(im.w))/2;
 const unsigned phase=(now/160)%20,gain=246+(phase<10?phase:19-phase);
 for(unsigned r=0;r<im.h;++r)for(unsigned c=0;c<im.w;++c){const unsigned at=r*im.w+c;
  if(im.alpha[at])dot(left+int(c),int(r),im.rgb[at],unsigned(im.alpha[at])*gain/255);
 }
}
void drawJackpots198(uint32_t now){
 fill(0,0,W,H,0x0800);
 for(int row=0;row<20;++row)fill(0,row,W,1,counter_fire89::mix565(0x0800,0x2000,unsigned(row)*255/19));
 jackpotTopper207(now);
 jackpotRow207(now,3,"GRAND");jackpotRow207(now,2,"MAJOR");
 jackpotRow207(now,1,"MINOR");jackpotRow207(now,0,"MINI");
 text("BLUE OR GREY: BACK",120,128,6,BRIGHT,1);
}
