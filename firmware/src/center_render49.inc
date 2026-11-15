#pragma once
#include "center_program49.h"
#include "center_word49.h"
// Exact ordered RGB565 programs, not flattened translucent sprites. Partial
// edges retain every rounding operation; a preceding prefix is discarded only
// when a later fully opaque operation makes it independent of the background.
namespace center_render49 {
struct Font {
 unsigned height;
 const uint8_t* advances;
 const uint16_t* offsets;
 const uint32_t* core;
 const uint32_t* pairs;
};
inline constexpr Font font10={10,center_program49::advance10,center_program49::offset10,center_program49::core10,center_program49::pair10};
inline constexpr Font font16={16,center_program49::advance16,center_program49::offset16,center_program49::core16,center_program49::pair16};
inline constexpr Font font20={20,center_program49::advance20,center_program49::offset20,center_program49::core20,center_program49::pair20};
inline int glyph(unsigned char c){
 if(c=='$')return 0;
 if(c>='0'&&c<='9')return int(c-'0')+1;
 return c==','?11:c=='.'?12:c=='-'?13:-1;
}
inline void execute(uint32_t program,int x,int y){
 if(!program||x<0||x>=W||y<0||y>=H||y<dotClipTop||y>=dotClipBottom)return;
 auto& out=pixels[y*W+x];
 if(program&0x80000000u){out=uint16_t(program);return;}
 constexpr uint16_t color[]={BLACK,0xa800,0xe7e6};
 const auto* op=center_program49::operations+program-1;
 const unsigned count=*op++;uint16_t result=out;
 for(unsigned i=0;i<count;++i){const auto instruction=*op++;result=blend(color[instruction&3],result,instruction>>2);}
 out=result;
}
inline void boundary(const Font& f,unsigned previous,unsigned current,int x,int y){
 const auto* program=f.pairs+(previous*center_program49::pairAlphabet+current)*(f.height+3)*2;
 for(unsigned row=0;row<f.height+3;++row){
  execute(*program++,x-1,y+int(row)-1);execute(*program++,x,y+int(row)-1);
 }
}
inline void originalCash(const char* value,int right,int y,unsigned size){
 cashText(value,right+1,y+2,size,BLACK);
 for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)
  if(dx||dy)cashText(value,right+dx,y+dy,size,0xa800);
 cashText(value,right,y,size,0xe7e6);
}
inline void outlinedCash(const char* value,int right,int y,unsigned size){
 const Font* font=size==10?&font10:size==16?&font16:size==20?&font20:nullptr;
 if(!font){originalCash(value,right,y,size);return;}
 // Validate the whole string before any write. Other callers/fonts retain
 // the exact old path instead of drawing a partial optimized prefix.
 for(auto p=(const unsigned char*)value;*p;++p)if(glyph(*p)<0){originalCash(value,right,y,size);return;}
 if(!*value)return;
 const Font& f=*font;int x=right-cashWidth(value,size);unsigned previous=center_program49::alphabetSize;
 for(auto p=(const unsigned char*)value;*p;++p){
  const unsigned current=unsigned(glyph(*p)),width=f.advances[current]-2;
  boundary(f,previous,current,x,y);
  const auto* program=f.core+f.offsets[current];
  for(unsigned row=0;row<f.height+3;++row)for(unsigned col=0;col<width;++col)
   execute(*program++,x+1+int(col),y+int(row)-1);
  x+=int(f.advances[current]);previous=current;
 }
 boundary(f,previous,center_program49::alphabetSize,x,y);
}
inline void word(int left,int top,uint32_t elapsed){
 for(const auto& p:center_word49::pixels){
  const int x=left+p.x,y=top+p.y;
  if(x<0||x>=W||y<0||y>=H||y<dotClipTop||y>=dotClipBottom)continue;
  const auto color=p.alpha>=32?hold_winner::shade(p.color,p.x/2,p.y/2,elapsed):p.color;
  auto& out=pixels[y*W+x];
  if(p.alpha==255){out=color;continue;}
  uint16_t result=p.base==1?BLACK:p.base==2?blend(BLACK,out,210):out;
  if(p.alpha)result=blend(color,result,p.alpha);
  out=result;
 }
}
}
