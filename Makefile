all:
	g++ -O3 -march=native src/main.cpp -lSDL2 -o hs -I third_party third_party/SDL2_glx/*c 
