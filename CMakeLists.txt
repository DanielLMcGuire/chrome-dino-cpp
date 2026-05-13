cmake_minimum_required(VERSION 3.15)
project(DinoGame CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_DEPS_SHARED OFF CACHE BOOL "" FORCE)

set(SDL2IMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_WEBP OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_TIF OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_JPG OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_BMP OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_GIF OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_LBM OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_PCX OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_PNM OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_QOI OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_SVG OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_TGA OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_XCF OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_XPM OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_XV OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_VENDORED OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_FLAC OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_MOD OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_MIDI OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_OPUS OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_OGG OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_WAVPACK OFF CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.8
)

FetchContent_Declare(
    SDL2_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-2.8.12
)

FetchContent_Declare(
    SDL2_mixer
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
    GIT_TAG release-2.8.0
)

FetchContent_MakeAvailable(SDL2 SDL2_image SDL2_mixer)

option(TESTS "Build with test runner" OFF)

file(GLOB_RECURSE ALL_SOURCES "src/*.cpp")
add_executable(DinoGame ${ALL_SOURCES})

target_link_libraries(DinoGame PRIVATE
    SDL2::SDL2-static
    SDL2_image::SDL2_image-static
    SDL2_mixer::SDL2_mixer-static
    SDL2::SDL2main
)

if(MSVC)
    target_compile_options(DinoGame PRIVATE
        $<$<CONFIG:Release,RelWithDebInfo>:
            /O2
            /GL
            /Gy
            /fp:fast
        >
    )
    target_link_options(DinoGame PRIVATE
        $<$<CONFIG:Release,RelWithDebInfo>:
            /LTCG
        >
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(DinoGame PRIVATE
        $<$<CONFIG:Release,RelWithDebInfo>:
            -O3
            -march=native
            -funroll-loops
            -ffast-math
            -flto=thin
        >
    )
    target_link_options(DinoGame PRIVATE
        $<$<CONFIG:Release,RelWithDebInfo>:
            -flto=thin
        >
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(DinoGame PRIVATE
        $<$<CONFIG:Release,RelWithDebInfo>:
            -O3
            -march=native
            -funroll-loops
            -ffast-math
            -flto
        >
    )
    target_link_options(DinoGame PRIVATE
        $<$<CONFIG:Release,RelWithDebInfo>:
            -flto
        >
    )
endif()

if (WIN32)
    set_target_properties(DinoGame PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

if (TESTS)
    target_compile_definitions(DinoGame PRIVATE
        AUTOPLAYER
    )
endif()

add_custom_command(TARGET DinoGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:DinoGame>/resources"
    COMMENT "Copying resources..."
)