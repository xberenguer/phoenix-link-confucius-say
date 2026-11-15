#pragma once
#include "entropy_lease125.h"
#include <esp_idf_version.h>
#include <bootloader_random.h>
#include <esp_random.h>
#include <driver/periph_ctrl.h>
#include <soc/system_reg.h>
#include <soc/apb_saradc_reg.h>
// Reviewed ONLY for the pinned offline StickS3 profile and IDF4.4.7. Adding
// RF/SoCADC users, enabling a microphone, or changing framework requires a new
// platform review. This is not an entropy-source validation/certification.
namespace platform_entropy125 {
#if defined(CONFIG_IDF_TARGET_ESP32S3) && ESP_IDF_VERSION == ESP_IDF_VERSION_VAL(4,4,7)
constexpr bool pinnedSdk=true;
inline bool acquireNative(void*){
 // Reject an unexplained existing SAR owner. Bootloader/our previous release
 // leave the digital module clock gated and reset asserted.
 if(REG_GET_BIT(SYSTEM_PERIP_CLK_EN0_REG,SYSTEM_APB_SARADC_CLK_EN)||!REG_GET_BIT(SYSTEM_PERIP_RST_EN0_REG,SYSTEM_APB_SARADC_RST))return false;
 // v4.4.7 helper disable holds SAR reset; helper enable does not clear it.
 // The balanced official driver lease releases reset on refcount0→1.
 periph_module_enable(PERIPH_SARADC_MODULE);
 if(REG_GET_BIT(SYSTEM_PERIP_RST_EN0_REG,SYSTEM_APB_SARADC_RST)){periph_module_disable(PERIPH_SARADC_MODULE);return false;}
 bootloader_random_enable();
 if(REG_GET_BIT(SYSTEM_PERIP_RST_EN0_REG,SYSTEM_APB_SARADC_RST)||!REG_GET_BIT(APB_SARADC_CTRL2_REG,APB_SARADC_TIMER_EN)){bootloader_random_disable();periph_module_disable(PERIPH_SARADC_MODULE);return false;}
 return true;
}
inline bool fillNative(void*,uint8_t*out,size_t size){esp_fill_random(out,size);return !REG_GET_BIT(SYSTEM_PERIP_RST_EN0_REG,SYSTEM_APB_SARADC_RST)&&REG_GET_BIT(APB_SARADC_CTRL2_REG,APB_SARADC_TIMER_EN);}
inline void releaseNative(void*){bootloader_random_disable();periph_module_disable(PERIPH_SARADC_MODULE);}
#else
constexpr bool pinnedSdk=false;
inline bool acquireNative(void*){return false;}
inline bool fillNative(void*,uint8_t*,size_t){return false;}
inline void releaseNative(void*){}
#endif
inline Hooks nativeHooks(){return {acquireNative,fillNative,releaseNative,nullptr};}
}
