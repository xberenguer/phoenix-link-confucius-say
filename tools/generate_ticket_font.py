from pathlib import Path
from PIL import Image,ImageFont,ImageDraw
import math
import argparse
p=argparse.ArgumentParser();p.add_argument('--font-dir',type=Path,required=True);p.add_argument('--out',type=Path,default=Path(__file__).resolve().parents[1]/'firmware/src/ticket_font142.h');args=p.parse_args();base=args.font_dir;output=args.out

out=['#pragma once','#include <cstdint>','// Rasterized Liberation Sans; SIL Open Font License 1.1. See LICENSES/LiberationFonts-OFL.txt.','namespace ticket_font142 {','struct Glyph{uint32_t offset;uint8_t w,h,advance;};']
for size in [5,6,7,8,10,11,12,18]:
 font=ImageFont.truetype(str(base/('LiberationSans-Bold.ttf'if size>=8 else 'LiberationSans-Regular.ttf')),size*4);top=font.getbbox('H')[1];bottom=font.getbbox('gj,')[3];height=math.ceil((bottom-top)/4);data=[];glyphs=[]
 for ch in range(32,127):
  c=chr(ch);advance=max(1,round(font.getlength(c)/4));w=max(advance,math.ceil(font.getbbox(c)[2]/4));im=Image.new('L',(w*4,height*4));ImageDraw.Draw(im).text((0,-top),c,font=font,fill=255);im=im.resize((w,height),Image.Resampling.LANCZOS);glyphs.append((len(data),w,height,advance));data+=list(im.get_flattened_data() if hasattr(im,'get_flattened_data') else im.getdata())
 out+=['inline constexpr uint8_t pixels'+str(size)+'[]={'+','.join(map(str,data))+'};','inline constexpr Glyph glyphs'+str(size)+'[]={'+','.join('{'+','.join(map(str,g))+'}'for g in glyphs)+'};']
args=''.join('case '+str(s)+':return glyphs'+str(s)+'[c-32];'for s in [5,6,7,8,10,11,12,18]);out+=['inline const Glyph& glyph(unsigned size,unsigned c){if(c<32||c>126)c=63;switch(size){'+args+'default:return glyphs6[c-32];}}'];args=''.join('case '+str(s)+':return pixels'+str(s)+';'for s in [5,6,7,8,10,11,12,18]);out+=['inline const uint8_t* pixels(unsigned size){switch(size){'+args+'default:return pixels6;}}','}'];output.write_text('\n'.join(out)+'\n');print('Generated matching sans ticket glyphs.')
