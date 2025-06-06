cmake_minimum_required(VERSION 3.16)
project(nancyplayer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSES REQUIRED ncurses)
pkg_check_modules(SIDPLAYFP REQUIRED libsidplayfp)
# pkg_check_modules(RESIDFP REQUIRED libresid-builder)
pkg_check_modules(PULSEAUDIO REQUIRED libpulse-simple)

add_executable(nancyplayer
    src/main.cpp
    src/tui.cpp
    src/player.cpp
    src/file_browser.cpp
    src/stil_reader.cpp
    src/search.cpp
    src/config.cpp
)

target_include_directories(nancyplayer PRIVATE
    include
    ${NCURSES_INCLUDE_DIRS}
    ${SIDPLAYFP_INCLUDE_DIRS}
    # ${RESIDFP_INCLUDE_DIRS}
    ${PULSEAUDIO_INCLUDE_DIRS}
)

target_link_libraries(nancyplayer
    ${NCURSES_LIBRARIES}
    ${SIDPLAYFP_LIBRARIES}
    resid-builder
    ${PULSEAUDIO_LIBRARIES}
    pthread
)

target_compile_options(nancyplayer PRIVATE
    ${NCURSES_CFLAGS_OTHER}
    ${SIDPLAYFP_CFLAGS_OTHER}
    # ${RESIDFP_CFLAGS_OTHER}
    ${PULSEAUDIO_CFLAGS_OTHER}
)