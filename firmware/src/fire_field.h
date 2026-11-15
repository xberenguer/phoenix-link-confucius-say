#pragma once
#include <cstdint>

// Fresh CGI material deforms continuously in native display coordinates.
// This module has no game/RNG/audio/flash state. One instance describes one
// instant; sampling order, dropped frames and renderer cadence cannot alter it.
namespace fire_field {
constexpr unsigned width=235,height=105;
struct Point { int32_t u,v; }; // signed 16.8 material coordinates
constexpr unsigned verticalSegments=8;
// Optional caller-owned scratch. Keep it alive and unchanged until its Flow
// is finished. The UI reuses one bounded arena between sequential effects;
// separate simultaneous Flows require separate arenas. No heap allocation.
struct Workspace {Point horizontal[width*(verticalSegments+1)];};
class Flow {
 public:
  explicit Flow(uint32_t elapsed,Workspace* workspace=nullptr);
  class Row {
   public:
    uint16_t pixel(unsigned x) const;
    Point uv(unsigned x) const;
   private:
    friend class Flow;
    Row(const Point* upper,const Point* lower,unsigned fraction,bool cached):a(upper),b(lower),fy(fraction),cached(cached){}
    const Point *a,*b;unsigned fy;bool cached;
  };
  Row row(unsigned y) const;
  uint16_t pixel(unsigned x,unsigned y) const;
  Point uv(unsigned x,unsigned y) const;
 private:
  static constexpr unsigned columns=16,rows=verticalSegments;
  Point nodes[(columns+1)*(rows+1)];
  const Point* horizontal=nullptr;
};
unsigned storageBytes();
uint32_t decodedHash();
}
