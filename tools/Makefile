CXXFLAGS = -std=c++20 -O0 -g
RAYLIB_DIR = ../build/_deps/raylib-src/src
RAYLIB_LIB = ../build/_deps/raylib-build/raylib
LDLIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

bgm_prototype: bgm_prototype.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ -I $(RAYLIB_DIR) -L $(RAYLIB_LIB) $(LDLIBS)

synth: synth.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ -I $(RAYLIB_DIR) -L $(RAYLIB_LIB) $(LDLIBS)

ship_playground: ship_playground.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ -I $(RAYLIB_DIR) -L $(RAYLIB_LIB) $(LDLIBS)

shiprun: ship_playground
	./ship_playground

clean:
	rm -f bgm_prototype synth ship_playground

.PHONY: shiprun clean
