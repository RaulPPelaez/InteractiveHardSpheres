#ifndef HS_VISUALIZATION_H
#define HS_VISUALIZATION_H
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "common.h"
#include <SDL2/SDL_video.h>
#include <memory>
#include <functional>
#include <vector>
#include "Simulation.h"

namespace hs{
  using Color = SDL_Color;
  class Window{
    WindowSize ws;
    SDL_Window* window;
    SDL_Renderer* renderer;  
  public:
    Window(WindowSize ws):ws(ws){
      log<MESSAGE>("Initializing SDL");
      int st= SDL_Init(SDL_INIT_VIDEO);
      if(st<0)   log<CRITICAL>("SDL Could not initialize with error code: %d message: %s", st, SDL_GetError());
      window = SDL_CreateWindow("Hard Spheres", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, ws.fw, ws.fh, 0);
      if(!window)   log<CRITICAL>("SDL Could not open a window with error %s", SDL_GetError());
      renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
      if(!renderer)   log<CRITICAL>("SDL Could not create a renderer with error %s", SDL_GetError());
    }

    void clear(Color color){
      SDL_SetRenderDrawColor( renderer, color.r, color.g, color.b, color.a );
      SDL_RenderClear(renderer);    
    }
  
    void display(){
      SDL_RenderPresent(renderer);
    }

    SDL_Renderer* getRenderer(){
      return renderer;
    }
  
    WindowSize getWindowSize(){
      return ws;
    }
    
    void setWindowTitle(std::string title){
      SDL_SetWindowTitle(window, title.c_str());
    }
    
    ~Window(){
      SDL_DestroyRenderer(renderer);
      SDL_DestroyWindow(window);
      SDL_Quit();    
      log<MESSAGE>("Exiting");
    }
  };

  class ParticleRenderer{
    std::shared_ptr<Window> w;
  public:
    ParticleRenderer(std::shared_ptr<Window> w):w(w){ };

    void renderParticles(ParticleList list, Box box){
      auto renderer = w->getRenderer();
      auto ws = w->getWindowSize();
      for(int i = 0; i<list.numberParticles; i++){
	real2 p = box.apply_pbc(list.pos[i]);
	int r = list.radius[i]/box.lx*ws.fw;
	int x = ((p.x + box.lx*0.5f)/box.lx)*ws.fw;
	int y = ((p.y + box.ly*0.5f)/box.ly)*ws.fh;
	int st = filledCircleRGBA(renderer, x, y, r, 0x00, 0x00, 0x00, 0xFF);
	bool hasImage = false;
	if(fabs(p.x) >= box.lx*0.5 - 0.5){
	  hasImage = true;
	  p.x = p.x + (box.lx)*(p.x>0?-1:1);
	}
	 if(fabs(p.y) >= box.ly*0.5 - 0.5){
	   hasImage = true;
	   p.y = p.y + (box.ly)*(p.y>0?-1:1);
	 }
	if(hasImage){
	  x = ((p.x + box.lx*0.5f)/box.lx)*ws.fw;
	  y = ((p.y + box.ly*0.5f)/box.ly)*ws.fh;
	  st = filledCircleRGBA(renderer, x, y, r, 0x00, 0x00, 0x00, 0xFF);
	}

      }
    }
 
  };

  static bool quit = false;
  void signalQuit(){
    quit = true;
  }

  using Event = SDL_Event;

  class HandleEvents{
    using Handle = std::function<void(Event &)>;
    std::vector<Handle> handles;

  public:
    HandleEvents(){

      addEvent([](Event &e){
	if (e.type == SDL_QUIT) {
	  signalQuit();
	}
      });
    }
  
    void addEvent(Handle h){
      handles.push_back(h);
    }
  
    void handleEvents(){
      Event e;
      while(SDL_PollEvent(&e)){
	for(auto &h: handles) h(e);
      }    
    }
  
  };



  class Visualization{
    std::shared_ptr<Window> w;
    std::shared_ptr<ParticleRenderer> renderer;   
    HandleEvents events;
  public:
    Visualization(){
      w = std::make_shared<Window>(WindowSize{500, 500});
      renderer = std::make_shared<ParticleRenderer>(w);
      events.addEvent([](Event &e){
	if(e.type == SDL_KEYDOWN){
	  auto key = e.key.keysym.scancode;
	  if(key == SDL_SCANCODE_ESCAPE){
	    signalQuit();
	  }	
	}
      });
    }

    void updateScene(Simulation &simulation){
      events.handleEvents();
      w->clear({0xFF, 0xFF, 0xFF, 0xFF});
      renderer->renderParticles(simulation.getParticles(), simulation.getBox());   
      w->display();
    }
        
    template<class T> void addEventHandle(T handle){
      events.addEvent(handle);
    }

    bool isActive(){
      return !quit;
    }

    void setWindowTitle(std::string title){
      w->setWindowTitle(title);
    }
  };
}
#endif
