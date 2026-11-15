#pragma once
#include "checkpoint_bundle.h"
#include <nvs.h>
#include <cstring>
namespace checkpoint_store {
constexpr const char*key="checkpoint";
enum class Read:uint8_t {Absent,Valid,Invalid,Error};
struct Result {Read state=Read::Error;esp_err_t error=ESP_OK;size_t length=0;bool observedPresent=false;};
// Only the first raw length query returning NOT_FOUND establishes absence.
// A failed second query is an error, never a license for legacy/fresh fallback.
inline Result readBlob(nvs_handle_t handle,const char*name,void*out,size_t capacity){
 Result r;size_t needed=0;const auto probe=nvs_get_blob(handle,name,nullptr,&needed);r.error=probe;
 if(probe==ESP_ERR_NVS_NOT_FOUND){r.state=Read::Absent;return r;}
 if(probe==ESP_ERR_NVS_TYPE_MISMATCH){r.state=Read::Invalid;r.observedPresent=true;return r;}
 if(probe!=ESP_OK)return r;
 r.observedPresent=true;r.length=needed;
 if(!needed||needed>capacity||!out){r.state=Read::Invalid;return r;}
 size_t received=needed;r.error=nvs_get_blob(handle,name,out,&received);
 if(r.error!=ESP_OK)return r;
 if(received!=needed){r.state=Read::Invalid;r.error=ESP_ERR_NVS_INVALID_LENGTH;return r;}
 r.state=Read::Valid;return r;
}
inline Result read(nvs_handle_t handle,void*out,size_t capacity){
 auto r=readBlob(handle,key,out,capacity);
 if(r.state==Read::Valid){checkpoint_bundle::View v;if(!checkpoint_bundle::decode(out,r.length,v))r.state=Read::Invalid;}
 return r;
}
// Caller may use this only with a trusted immutable pending bank. Readback
// equality does not replace successful nvs_commit, including on a lost ack.
inline bool commit(nvs_handle_t handle,const void*record,size_t length,void*readback,size_t capacity,esp_err_t&error){
 checkpoint_bundle::View v;error=ESP_OK;
 if(!record||!readback||length>capacity||!checkpoint_bundle::decode(record,length,v)){error=ESP_ERR_INVALID_ARG;return false;}
 auto current=readBlob(handle,key,readback,capacity);
 const bool already=current.state==Read::Valid&&current.length==length&&!std::memcmp(record,readback,length);
 if(!already){error=nvs_set_blob(handle,key,record,length);if(error!=ESP_OK)return false;}
 error=nvs_commit(handle);if(error!=ESP_OK)return false;
 current=readBlob(handle,key,readback,capacity);error=current.error;
 if(current.state!=Read::Valid||current.length!=length||std::memcmp(record,readback,length)){if(error==ESP_OK)error=ESP_ERR_NVS_INVALID_LENGTH;return false;}
 return true;
}
} // namespace checkpoint_store
