# Shared source list for the GalaxyImpact target, used by both the root
# CMakeLists.txt (desktop/web builds) and android/app/src/main/cpp/CMakeLists.txt
# (Android build), so the two build definitions can't drift apart.
set(GALAXYIMPACT_SOURCES
    src/achievements.cpp
    src/bestiary.cpp
    src/democonfig.cpp
    src/draw.cpp
    src/entities/boss.cpp
    src/entities/enemy.cpp
    src/entities/player.cpp
    src/entities/space.cpp
    src/game.cpp
    src/guitheme.cpp
    src/highscore.cpp
    src/main.cpp
    src/menu.cpp
    src/platform.cpp
    src/raygui_impl.cpp
    src/sandbox.cpp
    src/settings.cpp
    src/ships/bastion.cpp
    src/ships/hull.cpp
    src/ships/interceptor.cpp
    src/ships/ranger.cpp
    src/sound.cpp
    src/update.cpp
)
