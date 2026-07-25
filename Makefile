.PHONY: build run clean

build/CMakeCache.txt:
	cmake -S . -B build -G Ninja

build: build/CMakeCache.txt
	cmake --build build

run: build
	./build/GalaxyImpact

clean:
	rm -rf build
