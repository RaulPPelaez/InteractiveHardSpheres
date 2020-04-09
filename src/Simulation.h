#ifndef HS_SIMULATION_H
#define HS_SIMULATION_H

#include"common.h"
#include <vector>
#include <chrono>
#include <random>
#include<functional>
#include <memory>
#include <algorithm>
#include<iostream>
namespace hs{
 
  struct Grid{
    int2 size;
    
    int2 getCell(real2 pos, Box box){
      int n = int( (0.5 + pos.x/box.lx)*size.x);
      int m = int( (0.5 + pos.y/box.ly)*size.y);
      if(n == size.x) n = 0;
      if(m == size.y) m = 0;
      return {n,m};
    }

    int2 pbc_cell(int2 cell){
      if(cell.x == -1) cell.x = size.x-1;
      else if(cell.x == size.x) cell.x = 0;
      if(cell.y == -1) cell.y = size.y-1;
      else if(cell.y == size.y) cell.y = 0;
      return cell;
    }
  };

  struct CellListData{
    int* head;
    int *list;
    real rcut;
    Box box;
    Grid grid;
    Index2D storageIndex;
  };
  
  class CellList{
    std::vector<int> head, list;
    std::vector<int> redo;
    real rcut;
    Box box;
    Grid grid;
    Index2D storageIndex;
  public:
    
    CellList(Box box, real rc):box(box), rcut(rc){
      grid.size.x = std::max(3, int(box.lx/rc));
      grid.size.y = std::max(3, int(box.lx/rc));
      head.resize(grid.size.x*grid.size.y);
      storageIndex = Index2D(grid.size);
    }

    
    void update(ParticleList particles){
      list.resize(particles.numberParticles+1);
      std::fill(head.begin(), head.end(), 0);
      for(int i = 0; i < particles.numberParticles; i++){
	auto p = box.apply_pbc(particles.pos[i]);
	int icell = getCellIndex(p);
	list[i+1] = head[icell];
	head[icell] = i+1;
      }
    }

    void heal_cells(ParticleList particles, int2 cellia, int2 cellib){
      list.resize(particles.numberParticles+1);
      int cella = storageIndex(cellia);
      int cellb = storageIndex(cellib);
      int to_redo=0;
      int i = head[cella];
      redo.resize(particles.numberParticles);
      while(i!=0){
	redo[to_redo]=i;
	i = list[i];
	to_redo++;
      }
      i = head[cellb];
      while(i!=0){
	redo[to_redo]=i;
	i = list[i];
	to_redo++;
      }
      head[cella] = head[cellb] = 0;
      for(int j = 0; j< to_redo; j++){
	i = redo[j];
	auto p = box.apply_pbc(particles.pos[i-1]);
	int icell = getCellIndex(p);
	list[i] = head[icell];
	head[icell] = i;
      }
    }
    
    int getCellIndex(real2 pos){
      auto cell = getCell(pos);
      int icell = storageIndex(cell);
      return icell;
    }
        
    int2 getCell(real2 pos){
      auto cell = grid.getCell(pos, box);
      return cell;
    }

    CellListData getList(){
      CellListData cl;
      cl.list = list.data();
      cl.head = head.data();
      cl.rcut = rcut;
      cl.box = box;
      cl.grid = grid;
      cl.storageIndex = storageIndex;
      return cl;
    }
  };

  class Optimize{
    int naccept;
    int ntry;
    int ncontrol;
    real jumpSize;
    real currentAcceptanceRatio = 0;
    const real targetRatio = 0.4;
    
    void adjustJumpSize(){
      float ratio = naccept/ (float)ntry;
      constexpr float updateRate = 1.01;
      if(ratio > targetRatio){
	if(jumpSize<2){
	  jumpSize*=updateRate;
	}
      }
      else if(ratio < targetRatio){
	jumpSize/=updateRate;
      }
      if(jumpSize<0.005)jumpSize = 1;
      ntry = naccept = 0;
      currentAcceptanceRatio = ratio;
    }
    
  public:
    
    Optimize(){
      naccept = 0;
      ntry = 0;
      ncontrol = 3000;
      jumpSize = 1;
    }
    
    void registerAccept(){
      naccept++;
      ntry++;
      if(ntry%ncontrol == 0)
	adjustJumpSize();
    }
    
    void registerReject(){
      ntry++;
      if(ntry%ncontrol == 0)
	adjustJumpSize();
    }
    
    real getJumpSize(){
      return jumpSize;
    }

    real getAcceptanceRatio(){
      return currentAcceptanceRatio;
    }
    
  };
  
  class Simulation{
    std::vector<real2> pos;
    std::vector<real> radius;
    int numberParticles;
    Box box;
    std::shared_ptr<CellList> cl;
    real rcut;
    Optimize optimize;
    using Engine = std::mt19937;
    Engine generator;
  public:

    Simulation(){
      auto secondsSinceEpoch = std::chrono::system_clock::now().time_since_epoch().count();
      auto seed = 0xbada55d00d^secondsSinceEpoch;
      generator = Engine(seed);      
      initializeParameters();
      initializeParticles();
      cl = std::make_shared<CellList>(box, rcut);
      cl->update(getParticles());
    }

    void initializeParameters(){
      box = Box({16,16});
      numberParticles = 100;
      rcut = 1; 
    }
 
    void initializeParticles(){
      pos.resize(numberParticles);     
      using Distribution = std::uniform_real_distribution<real>;
      auto rng = [&](float min, float max){return Distribution(min, max)(generator); };
      for(int i = 0; i< numberParticles; i++){    
	pos[i] = {rng(-box.lx*0.5, box.lx*0.5), rng(-box.ly*0.5, box.ly*0.5)};
      }    
      radius = std::vector<real>(numberParticles, 0.5);
    }
  
    ParticleList getParticles(){
      ParticleList particles;    
      particles.pos = pos.data();
      particles.radius = radius.data();
      particles.numberParticles = numberParticles;
      return particles;
    }

    Box getBox(){
      return box;
    }

    void advance(){
      int i = std::uniform_int_distribution<int>(0, numberParticles-1)(generator);
      auto p = box.apply_pbc(pos[i]);
      auto oldcell = cl->getCell(p);
      real2 jump = getRandomDisplacement();
      real2 newpos = {p.x + jump.x, p.y + jump.y};
      int2 newcell = cl->getCell(box.apply_pbc(newpos));
      if(!collidesWithOtherParticles(i, newcell, newpos)){
	pos[i] = newpos;
	if(newcell.x != oldcell.x or newcell.y != oldcell.y){
	  cl->heal_cells(getParticles(), newcell, oldcell);
	}
	optimize.registerAccept();
      }
      else
	optimize.registerReject();
    }

    bool collidesWithOtherParticles(int i, int2 cell, real2 p){
      auto cld = cl->getList();
      constexpr int numberNeighbourCells = 9;
      for(int neigh = 0; neigh< numberNeighbourCells; neigh++){
	int2 cellj;
	cellj.x = cell.x + neigh%3-1;
	cellj.y = cell.y + neigh/3-1;
	cellj = cld.grid.pbc_cell(cellj);
	int jcell = cld.storageIndex(cellj);
	int j = cld.head[jcell];
	if(j==0) continue;
	int c = 0;
	do{
	  if(i != (j-1)){
	    if(interactHS(p, pos[j-1]))
	      return true;
	  }
	  j = cld.list[j];
	}while(j>0);

      }
      return false;
    }
    
    bool interactHS(real2 pi, real2 pj){
      real2 rij = box.apply_pbc({ pj.x - pi.x, pj.y - pi.y});
      real r2 = rij.x*rij.x + rij.y*rij.y;      
      if(r2 < rcut*rcut)
	return true;
      else
	return false;
    }
    
    real2 getRandomDisplacement(){
      real jumpSize = optimize.getJumpSize();
      auto rng = [&](){
	return std::uniform_real_distribution<real>(-jumpSize, jumpSize)(generator);
      };
      return {rng(), rng()};
    }

    void addParticle(){
      using Distribution = std::uniform_real_distribution<real>;
      auto rng = [&](float min, float max){return Distribution(min, max)(generator); };
      int adding_tries = 0;
      while(adding_tries++ < 10000){
	real2 newpos = {rng(-box.lx*0.5+1.0, box.lx*0.5), rng(-box.ly*0.5+1.0, box.ly*0.5)};
	int2 cell = cl->getCell(newpos);
	if(!collidesWithOtherParticles(-1, cell, newpos)){
	  pos.push_back(newpos);
	  radius.push_back(0.5);
	  numberParticles++;
	  cl->update(getParticles());
	  return;
	}
      }      
    }
    
    void enlargeBox(){
      real change = 1.01f;
      for(auto &p: pos){ p.x *=change; p.y *= change;}
      changeBox(Box({box.lx*change, box.ly*change}));
    }
    
    void reduceBox(){
      real change = 1.0f-std::min(0.01f, 1.0f/(optimize.getJumpSize()*100.0f));
      for(auto &p: pos){ p.x *=change; p.y *= change;}
      changeBox( Box({box.lx*change, box.ly*change}));
    }

    void print(){
      log<MESSAGE>("Number of particles: %d", numberParticles);
      log<MESSAGE>("Box size: %g %g", box.lx, box.ly);
      log<MESSAGE>("Surface fraction: %g", M_PI*0.5*0.5*numberParticles/(box.lx*box.ly));
      log<MESSAGE>("Jump length: %g", optimize.getJumpSize());
      log<MESSAGE>("Current acceptance ratio: %g", optimize.getAcceptanceRatio());
    }
  private:
    void changeBox(Box newbox){
      box = newbox;
      cl = std::make_shared<CellList>(box, rcut);
      cl->update(getParticles());
    }
    
  };
}
#endif
