#pragma once
// One version supplies the menu mark and the native USB status report.
#define PHOENIX_BUILD_VERSION "0.2.171"
namespace build_version {
inline constexpr char number[]=PHOENIX_BUILD_VERSION;
inline constexpr char firmware[]=PHOENIX_BUILD_VERSION;
inline constexpr char menu[]="v" PHOENIX_BUILD_VERSION;
}
#undef PHOENIX_BUILD_VERSION
