cmake_minimum_required(VERSION 3.20)
project(Solitaire LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Link the C/C++ runtime statically so the exe needs no Visual C++ Redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(Solitaire WIN32
    src/main.cpp
    src/App.cpp
    src/core/CardRenderer.cpp
    src/core/Deck.cpp
    src/core/Game.cpp
    src/core/GameRegistry.cpp
    src/core/Pile.cpp
    src/core/Settings.cpp
    src/games/BakersDozen.cpp
    src/games/FortyThieves.cpp
    src/games/FreeCell.cpp
    src/games/KingsCorners.cpp
    src/games/Klondike.cpp
    src/games/Pyramid.cpp
    src/games/Scorpion.cpp
    src/games/Spider.cpp
    src/games/Things.cpp
    src/games/TriPeaks.cpp
    src/games/Yukon.cpp
    res/app.rc
    res/app.manifest
)

# Restrict the Windows headers to the Windows XP API set so nothing newer is
# used by accident. (Newer DPI functions are loaded at runtime when available.)
target_compile_definitions(Solitaire PRIVATE
    UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX
    WINVER=0x0501 _WIN32_WINNT=0x0501)
target_compile_options(Solitaire PRIVATE /W4 /permissive- /utf-8)
target_link_libraries(Solitaire PRIVATE comctl32 comdlg32 msimg32 shell32)

# The .rc references these files; rebuild resources when they change.
set_source_files_properties(res/app.rc PROPERTIES OBJECT_DEPENDS
    "${CMAKE_SOURCE_DIR}/res/cards.bmp;${CMAKE_SOURCE_DIR}/res/app.ico")
