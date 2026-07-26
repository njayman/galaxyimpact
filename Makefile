.PHONY: build steam web run sandbox clean

build/CMakeCache.txt:
	cmake -S . -B build -G Ninja

build steam: build/CMakeCache.txt
	cmake --build build

build-web/CMakeCache.txt:
	cd $(HOME)/dev-tools/emsdk && . ./emsdk_env.sh && cd - && emcmake cmake -S . -B build-web -G Ninja

web: build-web/CMakeCache.txt
	cd $(HOME)/dev-tools/emsdk && . ./emsdk_env.sh && cd - && cmake --build build-web

run: build
	./build/GalaxyImpact

sandbox: build
	./build/GalaxyImpact -sandbox

clean:
	rm -rf build build-web
