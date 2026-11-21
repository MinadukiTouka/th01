cmake_minimum_required(VERSION 3.20)
project(th01game CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(REC98 ${CMAKE_CURRENT_SOURCE_DIR}/rec98)
set(PORT  ${CMAKE_CURRENT_SOURCE_DIR}/port)
set(THIRD_PARTY ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)

set(TH01_GAME_SRC
    ${REC98}/th01/egc.cpp
    ${REC98}/th01/frmdelay.cpp
    ${REC98}/th01/graph.cpp
    ${REC98}/th01/grp2xscs.cpp
    ${REC98}/th01/grp_text.cpp
    ${REC98}/th01/grp.cpp
    ${REC98}/th01/grppsafx.cpp
    ${REC98}/th01/initexit.cpp
    ${REC98}/th01/mdrv2.cpp
    ${REC98}/th01/pf.cpp
    ${REC98}/th01/planar.cpp
    ${REC98}/th01/scoredat.cpp
    ${REC98}/th01/regist.cpp
    ${REC98}/th01/resident.cpp
    ${REC98}/th01/tram_x16.cpp
    ${REC98}/th01/vsync.cpp
    ${REC98}/th01/ztext.cpp
    ${REC98}/th01/op_01.cpp
    ${REC98}/th01/input.cpp
    ${REC98}/th01/main_01.cpp
    ${REC98}/th01/bos.cpp
    ${REC98}/th01/ptn.cpp
    ${REC98}/th01/grz.cpp
    ${REC98}/th01/main_09.cpp
    ${REC98}/th01/bullet_l.cpp
    ${REC98}/th01/grpinv32.cpp
    ${REC98}/th01/scrollup.cpp
    ${REC98}/th01/pgtrans.cpp
    ${REC98}/th01/grph1to0.cpp
    ${REC98}/th01/main_14.cpp
    ${REC98}/th01/main_15.cpp
    ${REC98}/th01/main_17.cpp
    ${REC98}/th01/main_18.cpp
    ${REC98}/th01/main_20.cpp
    ${REC98}/th01/main_21.cpp
    ${REC98}/th01/main_23.cpp
    ${REC98}/th01/main_24.cpp
    ${REC98}/th01/main_25.cpp
    ${REC98}/th01/main_26.cpp
    ${REC98}/th01/player.cpp
    ${REC98}/th01/stages.cpp
    ${REC98}/th01/main_28.cpp
    ${REC98}/th01/main_29.cpp
    ${REC98}/th01/main_30.cpp
    ${REC98}/th01/main_31.cpp
    ${REC98}/th01/main_32.cpp
    ${REC98}/th01/particle.cpp
    ${REC98}/th01/main_33.cpp
    ${REC98}/th01/main_34.cpp
    ${REC98}/th01/main_35.cpp
    ${REC98}/th01/main_36.cpp
    ${REC98}/th01/main_37.cpp
    ${REC98}/th01/main_38.cpp
    ${REC98}/th01/fuuin_01.cpp
    ${REC98}/th01/fuuin_03.cpp
    ${REC98}/game/cutscene.cpp
)

add_library(th01game STATIC
    ${TH01_GAME_SRC}
    ${PORT}/pc98_runtime.cpp
    ${PORT}/encoding.cpp
    ${PORT}/audio_ogg.cpp
    ${THIRD_PARTY}/stb_vorbis_impl.c
    ${PORT}/master.cpp
    ${PORT}/gfx.cpp
    ${PORT}/pi.cpp
    ${PORT}/font.cpp
    ${PORT}/platform.cpp
    ${PORT}/time.cpp
    ${PORT}/jis_table.cpp
)

target_include_directories(th01game PRIVATE
    ${PORT}/include
    ${PORT}
    ${REC98}
    ${THIRD_PARTY}
)

target_compile_definitions(th01game PRIVATE
    GAME=1
    CPU=386
    MASTER_FAR
)

target_compile_options(th01game PRIVATE
    -fno-exceptions
    -fno-rtti
    -fno-strict-aliasing
    -fsigned-char
    -ffp-contract=off
    -Wno-invalid-source-encoding
    -Wno-multichar
    -Wno-c++17-extensions
    -Wno-parentheses-equality
    -Wno-c++11-narrowing
    -Wno-narrowing
    -Wno-switch
    -Wno-format
    -Wno-register
    -Wno-unused-variable
    -Wno-unused-but-set-variable
    -Wno-sign-compare
)

# Android app (libmain.so + libSDL2.so).
option(TH01_ANDROID "Build the Android SDL2 host" OFF)
if(TH01_ANDROID)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/SDL2
                     ${CMAKE_BINARY_DIR}/SDL2)

    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freetype
                     ${CMAKE_BINARY_DIR}/freetype)
    target_include_directories(th01game PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freetype/include
    )
    target_link_libraries(th01game PRIVATE freetype)

    add_library(main SHARED android-host/AndroidMain.cpp)
    target_include_directories(main PRIVATE
        ${PORT}/include ${PORT} ${REC98}
    )
    target_compile_definitions(main PRIVATE GAME=1 CPU=386 MASTER_FAR)
    target_link_libraries(main PRIVATE
        th01game SDL2 log android EGL GLESv3 m dl
    )
    set_target_properties(main PROPERTIES OUTPUT_NAME main)
    return()
endif()

# Optional Windows/Linux desktop test harness.
option(TH01_HOST "Build the desktop test harness" OFF)
if(TH01_HOST)
    find_package(Threads REQUIRED)
    add_executable(th01run11 host/host_main.cpp)
    target_include_directories(th01run11 PRIVATE ${PORT}/include ${PORT} ${REC98})
    target_compile_definitions(th01run11 PRIVATE GAME=1 CPU=386 MASTER_FAR)
    target_link_libraries(th01run11 PRIVATE th01game m Threads::Threads)
    return()
endif()

# Link-probe target: pulls every object out of the archive so that the linker
# reports the real missing master.lib/runtime symbols.
add_executable(th01_linkprobe ${PORT}/dummy_main.cpp)
target_include_directories(th01_linkprobe PRIVATE ${PORT}/include ${PORT} ${REC98})
target_compile_definitions(th01_linkprobe PRIVATE GAME=1 CPU=386 MASTER_FAR)
target_link_libraries(th01_linkprobe PRIVATE
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,th01game>"
    m
)
target_link_options(th01_linkprobe PRIVATE -Wl,--no-undefined -Wl,--error-limit=0)
