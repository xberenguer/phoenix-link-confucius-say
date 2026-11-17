#pragma once
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include "major_material153_data.h"
#ifdef ESP32S3
#include <esp_heap_caps.h>
#include <esp32s3/rom/miniz.h>
#else
#include <zlib.h>
#endif

// Startup-only, lossless cache for the original title/frame excerpt. The
// immutable native samples decode into external RAM, leaving frame rendering
// allocation-free. A failed cache is never exposed to a decoder.
namespace major_material153 {
inline uint8_t* cache=nullptr;
inline bool checked=false;
inline uint32_t decodedHash=0;
inline uint32_t checksum(const uint8_t* data,size_t count){
 uint32_t h=2166136261u;while(count--)h=(h^*data++)*16777619u;return h;
}
inline bool init(){
 if(checked)return cache!=nullptr;
 checked=true;
#ifdef ESP32S3
 auto* output=static_cast<uint8_t*>(heap_caps_malloc(major_material153_data::decodedBytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
#else
 auto* output=static_cast<uint8_t*>(std::malloc(major_material153_data::decodedBytes));
#endif
 if(!output)return false;
 bool valid=false;
#ifdef ESP32S3
 auto* context=static_cast<tinfl_decompressor*>(heap_caps_malloc(sizeof(tinfl_decompressor),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
 if(!context){heap_caps_free(output);return false;}
 tinfl_init(context);
 size_t inputBytes=sizeof(major_material153_data::compressed),outputBytes=major_material153_data::decodedBytes;
 const auto result=tinfl_decompress(context,major_material153_data::compressed,&inputBytes,output,output,&outputBytes,TINFL_FLAG_PARSE_ZLIB_HEADER|TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
 valid=result==TINFL_STATUS_DONE&&inputBytes==sizeof(major_material153_data::compressed)&&outputBytes==major_material153_data::decodedBytes;
 heap_caps_free(context);
#else
 uLongf outputBytes=major_material153_data::decodedBytes;uLong inputBytes=sizeof(major_material153_data::compressed);
 valid=uncompress2(output,&outputBytes,major_material153_data::compressed,&inputBytes)==Z_OK&&outputBytes==major_material153_data::decodedBytes&&inputBytes==sizeof(major_material153_data::compressed);
#endif
 if(valid){decodedHash=checksum(output,major_material153_data::decodedBytes);valid=decodedHash==major_material153_data::decodedFNV;}
 if(!valid){
#ifdef ESP32S3
  heap_caps_free(output);
#else
  std::free(output);
#endif
  return false;
 }
 cache=output;return true;
}
inline const uint8_t* bytes(){return cache;}
}
