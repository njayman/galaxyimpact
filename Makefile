.PHONY: configure build run clean

configure:
	cmake -S . -B build -G Ninja

build: configure
	cmake --build build

run: build
	./build/GalaxyImpact

clean:
	rm -rf build
