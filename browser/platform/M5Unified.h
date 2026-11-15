#pragma once
#include "Arduino.h"
#include <cstddef>
#include <cstdint>
void browserPresent(const uint16_t*,int,int,uint8_t,uint8_t);
bool browserAudioAvailable();
void browserAudioMaster(uint8_t,unsigned,bool);
void browserAudioGain(uint8_t,uint8_t);
unsigned browserAudioQueued(unsigned);
bool browserAudioPlay(const int16_t*,size_t,uint32_t,bool,uint32_t,unsigned,bool);
void browserAudioStop(int);
bool browserButtonPressed(unsigned);
struct BrowserDisplay {
 uint8_t rotation=1,brightness=200;bool swap=false;
 void setSwapBytes(bool value){swap=value;}
 void setRotation(uint8_t value){rotation=value;}
 uint8_t getRotation()const{return rotation;}
 void setBrightness(uint8_t value){brightness=value;}
 uint8_t getBrightness()const{return brightness;}
 void pushImage(int x,int y,int width,int height,const uint16_t*data){if(x==0&&y==0&&width==240&&height==135&&swap)browserPresent(data,width,height,rotation,brightness);}
};
struct BrowserSpeakerConfig {int pin_bck=17,pin_data_out=14;uint8_t magnification=1;bool stereo=true;uint32_t sample_rate=44100;size_t dma_buf_len=256,dma_buf_count=8;};
struct BrowserSpeaker {
 BrowserSpeakerConfig configuration{};uint8_t master=192;
 BrowserSpeakerConfig config()const{return configuration;}
 void config(BrowserSpeakerConfig value){configuration=value;browserAudioMaster(master,value.magnification,value.stereo);}
 bool begin(){return browserAudioAvailable();}
 void end(){browserAudioStop(-1);}
 void setVolume(uint8_t value){master=value;browserAudioMaster(value,configuration.magnification,configuration.stereo);}
 void setChannelVolume(uint8_t channel,uint8_t value){browserAudioGain(channel,value);}
 unsigned isPlaying(unsigned channel){return browserAudioQueued(channel);}
 unsigned isPlaying(){unsigned total=0;for(unsigned i=0;i<8;++i)total+=browserAudioQueued(i);return total;}
 bool playRaw(const int16_t*data,size_t count,uint32_t rate,bool stereo,uint32_t repeat,unsigned channel,bool replace){return browserAudioPlay(data,count,rate,stereo,repeat,channel,replace);}
 void stop(int channel=-1){browserAudioStop(channel);}
};
struct BrowserButton {unsigned index;bool isPressed()const{return browserButtonPressed(index);}};
struct BrowserM5 {BrowserDisplay Display;BrowserSpeaker Speaker;BrowserButton BtnA{0},BtnB{1};};
inline BrowserM5 M5;
constexpr int pdPASS=1;
#define pdMS_TO_TICKS(value) (value)
int xTaskCreatePinnedToCore(void(*)(void*),const char*,unsigned,void*,unsigned,void*,unsigned);
void vTaskDelay(uint32_t);
