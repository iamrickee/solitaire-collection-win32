cmake_minimum_required(VERSION 3.20)
project(Solitaire LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Solitaire WIN32
    src/main.cpp
    src/App.cpp
    src/core/CardRenderer.cpp
    src/core/Deck.cpp
    src/core/Game.cpp
    src/core/GameRegistry.cpp
    src/core/Pile.cpp
    src/core/Settings.cpp
    src/games/Klondike.cpp
    res/app.rc
    res/app.manifest
)

target_compile_definitions(Solitaire PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(Solitaire PRIVATE /W4 /permissive- /utf-8)
target_link_libraries(Solitaire PRIVATE comctl32 comdlg32 msimg32 shell32 ole32)

# The .rc references cards.bmp; rebuild resources when it changes.
set_source_files_properties(res/app.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/res/cards.bmp)
