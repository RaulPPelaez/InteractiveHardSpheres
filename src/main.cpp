
#include "common.h"
#include"Simulation.h"
#include "Visualization.h"
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>
#include <string>

void printControls();

void addEvents(hs::Visualization &vis, hs::Simulation &simulation);

bool pause = false;

struct FrameCounter{
  int framesSinceLastCheck = 0;
  int initialTicks;
  
  FrameCounter(){
    reset();
  }
  
  void reset(){
    framesSinceLastCheck = 0;
    initialTicks = SDL_GetTicks();
  }
  
  void registerFrame(){
    framesSinceLastCheck++;    
  }

  float getAverageFPS(){
    float fps = std::min(10000.0f, framesSinceLastCheck/((SDL_GetTicks() - initialTicks) / 1000.f));
    return fps;
  }
  
};
int main(){
  hs::Simulation simulation;
  hs::Visualization vis;
  printControls();
  addEvents(vis, simulation);
  FrameCounter frameCounter;  
  int start = SDL_GetTicks();
  int updatesPerDisplay = 1;
  int targetFPS = 30;
  while(vis.isActive()){
    float fps = (frameCounter.getAverageFPS());
    if(!pause){
      updatesPerDisplay = std::max(1, int(updatesPerDisplay*fps/targetFPS + 0.5));
      for(int i = 0; i< updatesPerDisplay; i++){
	simulation.advance();
      }
    }
    else{
      SDL_Delay(1000*(std::max(0.0, 1.0-targetFPS/fps)));
    }
    vis.updateScene(simulation);
    frameCounter.registerFrame();
    if((SDL_GetTicks()-start)/1000.0f > 1){
      start = SDL_GetTicks();
      vis.setWindowTitle("Hard Spheres - FPS: " + std::to_string(int(fps+0.5)) +
			 " - Particle updates per frame: " + std::to_string(updatesPerDisplay));
    }
  }
  return 0;
}

void printControls(){
  hs::log<hs::MESSAGE>("LIST OF CONTROLS:");
  hs::log<hs::MESSAGE>("n: Try to add a particle at a random location");
  hs::log<hs::MESSAGE>("N: Try to add 10 particles at a random location");
  hs::log<hs::MESSAGE>("l: Reduce box size");
  hs::log<hs::MESSAGE>("L: Increase box size");
  hs::log<hs::MESSAGE>("p: Print simulation information");
  hs::log<hs::MESSAGE>("SPACE: Toggle pause");
  hs::log<hs::MESSAGE>("ESC: exit");
}

void addEvents(hs::Visualization &vis, hs::Simulation &simulation){
  vis.addEventHandle([&](hs::Event &e){
    if(e.type == SDL_KEYDOWN){
      auto key = e.key.keysym.scancode;
      if(key == SDL_SCANCODE_SPACE){
	pause = !pause;
      }
    }
  });
  
  vis.addEventHandle([&](hs::Event &e){
    if(e.type == SDL_KEYDOWN){
      auto key = e.key.keysym.scancode;
      if(key == SDL_SCANCODE_P){
	simulation.print();
      }
    }
  });

  vis.addEventHandle([&](hs::Event &e){
    if(e.type == SDL_KEYDOWN){
      auto key = e.key.keysym.scancode;
      if(key == SDL_SCANCODE_N){
	auto mod = SDL_GetModState();
	if(mod & KMOD_CAPS or mod & KMOD_SHIFT){
	  for(int i = 0; i< 10; i++){
	    simulation.addParticle();
	  }
	}
	simulation.addParticle();
      }
    }
  });

  vis.addEventHandle([&](hs::Event &e){
    if(e.type == SDL_KEYDOWN){
      auto key = e.key.keysym.scancode;
      if(key == SDL_SCANCODE_L){
	auto mod = SDL_GetModState();
	if(mod & KMOD_CAPS or mod & KMOD_SHIFT)
	  simulation.enlargeBox();
	else
	  simulation.reduceBox();
      }
    }
  });
}
