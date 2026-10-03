.PHONY: build steam web android all run sandbox clean

build/CMakeCache.txt:
	cmake -S . -B build -G Ninja

build steam: build/CMakeCache.txt
	cmake --build build

build-web/CMakeCache.txt:
	cd $(HOME)/dev-tools/emsdk && . ./emsdk_env.sh && cd - && emcmake cmake -S . -B build-web -G Ninja

web: build-web/CMakeCache.txt
	cd $(HOME)/dev-tools/emsdk && . ./emsdk_env.sh && cd - && cmake --build build-web

android:
	cd android && ./gradlew assembleDebug

# Builds desktop, web, and Android. Each has its own isolated build
# directory, so this is safe to run in parallel: `make -j3 all`.
all: build web android

run: build
	./build/GalaxyImpact

sandbox: build
	./build/GalaxyImpact -sandbox

clean:
	rm -rf build build-web
	cd android && ./gradlew clean
