#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#define PROGMEM
// Browser platform adapter. Game/UI sources retain their Arduino-facing API.
class String : public std::string {
 public:
  using std::string::string;
  String(const std::string& value):std::string(value){}
  template<class T,typename std::enable_if<std::is_arithmetic<T>::value,int>::type=0>
  String(T value):std::string(std::to_string(value)){}
  using std::string::operator+=;
  String& operator+=(int value){append(std::to_string(value));return *this;}
  String substring(size_t first,size_t last)const{return substr(first,last-first);}
  void remove(size_t first){erase(first);}
  size_t write(uint8_t c){push_back(char(c));return 1;}
  size_t write(const uint8_t* data,size_t size){append(reinterpret_cast<const char*>(data),size);return size;}
  friend String operator+(const String&a,const String&b){return std::string(a)+std::string(b);}
  friend String operator+(const String&a,int b){return std::string(a)+std::to_string(b);}
  friend String operator+(const String&a,const char*b){return std::string(a)+b;}
  friend String operator+(const char*a,const String&b){return a+std::string(b);}
};
uint32_t millis();
uint32_t micros();
void delay(uint32_t ms);
