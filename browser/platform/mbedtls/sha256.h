#pragma once
#include <cstddef>
int browserSha256(const unsigned char*,size_t,unsigned char*,int);
inline int mbedtls_sha256_ret(const unsigned char*data,size_t size,unsigned char*out,int sha224){return browserSha256(data,size,out,sha224);}
