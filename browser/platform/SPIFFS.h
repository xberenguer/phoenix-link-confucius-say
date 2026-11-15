#pragma once
#include <cstdio>
#include <memory>
#include <string>
#include <sys/stat.h>
class BrowserFile {
 std::unique_ptr<FILE,int(*)(FILE*)> file_{nullptr,&std::fclose};
 public:
 explicit BrowserFile(const char*path){file_.reset(std::fopen(path,"rb"));}
 explicit operator bool()const{return bool(file_);}
 size_t size(){if(!file_)return 0;const auto old=std::ftell(file_.get());if(std::fseek(file_.get(),0,SEEK_END))return 0;const auto size=std::ftell(file_.get());std::fseek(file_.get(),old,SEEK_SET);return size<0?0:size_t(size);}
 size_t read(void*data,size_t size){return file_?std::fread(data,1,size,file_.get()):0;}
 void close(){file_.reset();}
};
struct BrowserAssetFS {
 bool begin(bool formatOnFailure){struct stat s{};return !formatOnFailure&&!stat("firmware/data",&s)&&S_ISDIR(s.st_mode);}
 BrowserFile open(const char*path,const char*){return BrowserFile((std::string("firmware/data/")+(path[0]=='/'?path+1:path)).c_str());}
};
inline BrowserAssetFS SPIFFS;
