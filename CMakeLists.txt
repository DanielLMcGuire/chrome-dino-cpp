cmake_minimum_required(VERSION 3.15)

option(XBOX "Build for Xbox (UWP)" OFF)
option(TESTS "Build with autoplayer" OFF)

if(XBOX)
    set(CMAKE_SYSTEM_NAME    WindowsStore)
    set(CMAKE_SYSTEM_VERSION "10.0")
endif()

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

if(XBOX)
    set(SDL_FILESYSTEM ON CACHE BOOL "" FORCE)
    set(SDL_FILE ON CACHE BOOL "" FORCE)
    set(SDL_STDIO OFF CACHE BOOL "" FORCE)
endif()

include(FetchContent)
FetchContent_Declare(SDL2 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-2.30.8)
FetchContent_Declare(SDL2_image GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git GIT_TAG release-2.8.2)
FetchContent_Declare(SDL2_mixer GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git GIT_TAG release-2.8.0)
FetchContent_MakeAvailable(SDL2 SDL2_image SDL2_mixer)

set(SOURCES
    src/game.cpp
    src/entities/obstacle.cpp
    src/entities/trex.cpp
)

if(XBOX)
    list(APPEND SOURCES
        src/entry/main-uwp.cpp
        Package.appxmanifest
    )

    set(UWP_ASSETS
        assets/LockScreenLogo.scale-200.png
        assets/SplashScreen.scale-200.png
        assets/Square150x150Logo.scale-200.png
        assets/Square44x44Logo.scale-200.png
        assets/Square44x44Logo.targetsize-24_altform-unplated.png
        assets/StoreLogo.png
        assets/Wide310x150Logo.scale-200.png
    )
    list(APPEND SOURCES ${UWP_ASSETS})
    set_property(SOURCE ${UWP_ASSETS} PROPERTY VS_DEPLOYMENT_CONTENT 1)
    set_property(SOURCE ${UWP_ASSETS} PROPERTY VS_DEPLOYMENT_LOCATION "assets")

    file(GLOB_RECURSE GAME_RESOURCES "resources/*")
    list(APPEND SOURCES ${GAME_RESOURCES})
    set_property(SOURCE ${GAME_RESOURCES} PROPERTY VS_DEPLOYMENT_CONTENT 1)
    set_property(SOURCE ${GAME_RESOURCES} PROPERTY VS_DEPLOYMENT_LOCATION "resources")

else()
    list(APPEND SOURCES src/entry/main.cpp)
endif()

if(XBOX)
    list(APPEND SOURCES
        "${SDL2_SOURCE_DIR}/src/main/winrt/SDL_winrt_main_NonXAML.cpp"
    )
    set_source_files_properties(
        "${SDL2_SOURCE_DIR}/src/main/winrt/SDL_winrt_main_NonXAML.cpp"
        PROPERTIES COMPILE_FLAGS "/ZW"
    )
endif()

add_executable(DinoGame ${SOURCES})

target_include_directories(DinoGame PRIVATE src)

target_compile_definitions(DinoGame PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
if(XBOX)
    target_compile_definitions(DinoGame PRIVATE UWP)
    target_compile_definitions(DinoGame PRIVATE WINAPI_FAMILY=WINAPI_FAMILY_APP _UNICODE UNICODE)
endif()
if(TESTS)
    target_compile_definitions(DinoGame PRIVATE AUTOPLAYER)
endif()

target_link_libraries(DinoGame PRIVATE
    SDL2::SDL2-static
    SDL2_image::SDL2_image-static
    SDL2_mixer::SDL2_mixer-static
)

if(XBOX)
    target_link_libraries(DinoGame PRIVATE WindowsApp.lib)
else()
    target_link_libraries(DinoGame PRIVATE SDL2::SDL2main)
endif()

set_target_properties(DinoGame PROPERTIES WIN32_EXECUTABLE TRUE)
if(XBOX)
    set_target_properties(DinoGame PROPERTIES
        VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION "10.0.17763.0"
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
        VS_WINRT_EXTENSIONS OFF
    )
endif()

if(NOT XBOX)
    add_custom_command(TARGET DinoGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:DinoGame>/resources"
        COMMENT "Copying resources for desktop build..."
    )
endif()