#ifndef HS_COMMON_H
#define HS_COMMON_H
#include <math.h>
#define MAXLOGLEVEL 15
#include"Log.h"
namespace hs{
  using namespace Logging;
  using real = float;
  struct real2{
    real x,y;
  };
  struct int2{
    int x,y;
  };
  struct Index2D{
    Index2D(){}
    Index2D(int2 size):size(size){}
    int operator()(int2 coordinates){
      return coordinates.x + size.x*coordinates.y;
    }
    
  private:
    int2 size;
  };
  struct Box{
    real lx, ly;
    real2 apply_pbc(real2 r){      
      r.x -= floorf(r.x/lx + 0.5f)*lx;
      r.y -= floorf(r.y/ly + 0.5f)*lx;
      return r;
    }
  };
  
  struct WindowSize{
    int fw, fh;
  };
  
  struct ParticleList {
    real2 *pos;
    real* radius;
    int numberParticles;
  };
}
#endif
