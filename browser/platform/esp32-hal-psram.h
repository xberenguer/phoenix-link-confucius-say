#pragma once
#include <cstdlib>
inline void* ps_malloc(size_t size){return std::malloc(size);}
inline void* ps_calloc(size_t count,size_t size){return std::calloc(count,size);}
