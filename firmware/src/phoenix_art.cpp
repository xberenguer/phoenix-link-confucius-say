#include "phoenix_art.h"
#include <cstddef>
#include <cstdlib>
#include <new>
#include "phoenix_asset.h"
#include "phoenix_alpha_spans.h"
#ifdef ESP32S3
#include <SPIFFS.h>
#include <esp_heap_caps.h>
#include <esp32s3/rom/miniz.h>
#else
#include <cstdio>
#include <zlib.h>
#endif

namespace {
#include "phoenix_art_data.h"
constexpr unsigned pixels=phoenix_asset::width*phoenix_asset::height;
struct Record { uint16_t rgb[pixels]; uint8_t alpha[pixels]; uint8_t padding; };
static_assert(offsetof(Record,alpha)==pixels*2,"RGB/alpha atlas layout");
static_assert(sizeof(Record)==phoenix_asset::recordBytes,"Exact native record stride");
static_assert(sizeof(Record)*phoenix_asset::count==phoenix_asset::decodedBytes,"Exact atlas size");
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "Phoenix RGB565 atlas requires a little-endian target"
#endif
const pa::Image fallback={phoenix_asset::width,phoenix_asset::height,kFallbackRgb,kFallbackAlpha,0};
Record* atlas=nullptr;
pa::Image images[phoenix_asset::count];
bool checked=false,valid=false,spansValid=false;
uint32_t observedHash=0,sourceReadBytes=0,sourceReadHash=0;
uint32_t checksum(const uint8_t* data,size_t bytes){uint32_t h=2166136261u;while(bytes--)h=(h^*data++)*16777619u;return h;}
bool validateAlphaSpans(){
 // A future atlas may still load normally while stale metadata is ignored.
 if(phoenix_alpha_span_data::width!=phoenix_asset::width||
    phoenix_alpha_span_data::height!=phoenix_asset::height||
    phoenix_alpha_span_data::count!=phoenix_asset::count||
    phoenix_alpha_span_data::recordBytes!=phoenix_asset::recordBytes||
    phoenix_alpha_span_data::decodedFNV!=phoenix_asset::decodedFNV)return false;
 for(unsigned frame=0;frame<phoenix_asset::count;++frame)
  for(unsigned row=0;row<phoenix_asset::height;++row){
   const auto span=phoenix_alpha_span_data::rows[frame][row];
   if(span.left>span.right||span.right>phoenix_asset::width)return false;
   const auto* alpha=atlas[frame].alpha+row*phoenix_asset::width;
   // Verify precisely the omitted texels once after checked decoding. A
   // bad table disables this optimization, never the image/artwork bank.
   for(unsigned x=0;x<span.left;++x)if(alpha[x])return false;
   for(unsigned x=span.right;x<phoenix_asset::width;++x)if(alpha[x])return false;
  }
 return true;
}
void* allocate(size_t bytes){
#ifdef ESP32S3
 return heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
 return std::malloc(bytes);
#endif
}
void release(void* pointer){
#ifdef ESP32S3
 heap_caps_free(pointer);
#else
 std::free(pointer);
#endif
}
uint8_t* readSource(){
#ifdef ESP32S3
 // Never format. Both graphics and music use this existing read-only FS;
 // Arduino SPIFFS.begin(false) safely returns true when already mounted.
 if(!SPIFFS.begin(false))return nullptr;
 auto file=SPIFFS.open(phoenix_asset::file,"r");if(!file||file.size()!=phoenix_asset::compressedBytes)return nullptr;
#else
 auto* file=std::fopen("firmware/data/phoenix_atlas.zlib","rb");if(!file)return nullptr;
 if(std::fseek(file,0,SEEK_END)||std::ftell(file)!=long(phoenix_asset::compressedBytes)||std::fseek(file,0,SEEK_SET)){std::fclose(file);return nullptr;}
#endif
 auto* data=static_cast<uint8_t*>(allocate(phoenix_asset::compressedBytes));
 if(data)while(sourceReadBytes<phoenix_asset::compressedBytes){
#ifdef ESP32S3
  const size_t n=file.read(data+sourceReadBytes,phoenix_asset::compressedBytes-sourceReadBytes);
#else
  const size_t n=std::fread(data+sourceReadBytes,1,phoenix_asset::compressedBytes-sourceReadBytes,file);
#endif
  if(!n)break;sourceReadBytes+=n;
 }
#ifdef ESP32S3
 file.close();
#else
 std::fclose(file);
#endif
 if(data&&sourceReadBytes==phoenix_asset::compressedBytes)sourceReadHash=checksum(data,sourceReadBytes);
 if(!data||sourceReadBytes!=phoenix_asset::compressedBytes||sourceReadHash!=phoenix_asset::compressedFNV){release(data);return nullptr;}
 return data;
}

bool inflate(const uint8_t* input,size_t bytes,unsigned char* output){
#ifdef ESP32S3
 // The ROM helper that allocates its own context uses too much loop stack.
 // Keep the context static; the full texture atlas goes in the8MB PSRAM.
 static tinfl_decompressor context;
 tinfl_init(&context);size_t inputBytes=bytes,outputBytes=phoenix_asset::decodedBytes;
 const auto status=tinfl_decompress(&context,input,&inputBytes,output,output,&outputBytes,
     TINFL_FLAG_PARSE_ZLIB_HEADER|TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
 return status==TINFL_STATUS_DONE&&inputBytes==bytes&&outputBytes==phoenix_asset::decodedBytes;
#else
 uLongf outputBytes=phoenix_asset::decodedBytes;uLong inputBytes=bytes;
 return uncompress2(output,&outputBytes,input,&inputBytes)==Z_OK&&outputBytes==phoenix_asset::decodedBytes&&inputBytes==bytes;
#endif
}
}

namespace phoenix_art {
bool init(){
 if(checked)return valid;
 checked=true;
 auto* input=readSource();if(!input)return false;
 atlas=static_cast<Record*>(allocate(phoenix_asset::decodedBytes));
 if(!atlas){release(input);return false;}
 for(unsigned i=0;i<phoenix_asset::count;++i)new(&atlas[i])Record{};
 auto* raw=reinterpret_cast<unsigned char*>(atlas);
 if(inflate(input,phoenix_asset::compressedBytes,raw)){
  observedHash=checksum(raw,phoenix_asset::decodedBytes);
  valid=observedHash==phoenix_asset::decodedFNV;
 }
 release(input);
 if(!valid){
  release(atlas);
  atlas=nullptr;return false;
 }
 for(unsigned i=0;i<phoenix_asset::count;++i)images[i]={phoenix_asset::width,phoenix_asset::height,atlas[i].rgb,atlas[i].alpha,0};
 spansValid=validateAlphaSpans();
 return true;
}
const AlphaSpan* alphaSpans(const pa::Image& image){
 if(!valid||!spansValid||image.w!=phoenix_asset::width||
    image.h!=phoenix_asset::height||image.key||!image.alpha)return nullptr;
 // Integer-address membership avoids relational/subtraction operations on
 // unrelated Image objects. Only exact existing registry entries qualify.
 const uintptr_t address=reinterpret_cast<uintptr_t>(&image);
 const uintptr_t first=reinterpret_cast<uintptr_t>(images);
 if(address<first||address-first>=sizeof(images))return nullptr;
 const uintptr_t offset=address-first;
 if(offset%sizeof(images[0]))return nullptr;
 const unsigned index=unsigned(offset/sizeof(images[0]));
 if(image.rgb!=atlas[index].rgb||image.alpha!=atlas[index].alpha)return nullptr;
 return phoenix_alpha_span_data::rows[index];
}
bool ready(){return init();}
bool alphaReady(){return init()&&spansValid;}
unsigned alphaMetadataBytes(){return alphaReady()?sizeof(phoenix_alpha_span_data::rows):0;}
unsigned frameCount(){return phoenix_asset::count;}
uint32_t periodMs(){return phoenix_asset::periodMs;}
uint32_t decodedHash(){init();return observedHash;}
unsigned verifiedFrames(){return init()?phoenix_asset::count:0;}
unsigned assetReadBytes(){init();return sourceReadBytes;}
uint32_t assetReadHash(){init();return sourceReadHash;}
const pa::Image& frameIndex(unsigned index){return init()?images[index%phoenix_asset::count]:fallback;}
const pa::Image& frame(uint32_t elapsedMs){return frameIndex((elapsedMs%phoenix_asset::periodMs)*phoenix_asset::count/phoenix_asset::periodMs);}
}
