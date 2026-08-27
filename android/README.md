# Android build

Native Activity + CMake, sharing `src/`, `include/`, and `assets/` with the
desktop/web builds (see `cmake/GalaxyImpactSources.cmake`). No Java/Kotlin
code - `android.app.NativeActivity` loads `libGalaxyImpact.so` directly and
raylib's Android glue calls our regular `main()`.

## Build

Easiest: open the `android/` folder in Android Studio (Giraffe+), let it
sync, then Run. It'll generate the Gradle wrapper for you on first sync if
one isn't present.

Command line, with `ANDROID_HOME`/`ANDROID_NDK_HOME` set and a system
`gradle` installed:

```
cd android
gradle wrapper          # one-time, generates gradlew
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

Requires NDK 26.1.10909125 (pinned in `app/build.gradle` - change
`ndkVersion` there to match what you have installed) and CMake 3.22+.

## Notes

- `app/src/main/assets/assets` is a symlink to the repo-root `assets/`
  folder - don't duplicate assets into it.
- Touch controls (movement stick, dash/shield/nerve buttons) are wired up
  in `src/entities/player.cpp` (`updateTouchControls`) and drawn in
  `src/draw.cpp` (`drawTouchControls`); they're active on `PLATFORM_ANDROID`
  automatically, and on desktop/web too as soon as a touch is detected.
- Not build-tested here - no Android SDK/NDK in this environment. First
  build will likely need a couple of iterations to shake out anything the
  Gradle/CMake plumbing above got wrong.
