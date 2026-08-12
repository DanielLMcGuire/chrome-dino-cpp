cmake_minimum_required(VERSION 3.24)

option(UWP "Build for UWP" OFF)
option(PS2 "Build for PS2" OFF)
option(PCSX2 "Build for PS2 (PCSX2 Emulator)" OFF)
option(TESTS "Build with autoplayer" OFF)

if(UWP)
    set(CMAKE_SYSTEM_NAME WindowsStore)
    set(CMAKE_SYSTEM_VERSION "10.0")
endif()

project(DinoGame CXX)

if(PS2 OR PCSX2)
    set(CMAKE_EXECUTABLE_SUFFIX ".ELF")
endif()

if(PS2 OR PCSX2)
    add_executable(DinoGame
        src/game.cpp
        src/entities/obstacle.cpp
        src/entities/trex.cpp
        src/entry/main-ps2.cpp
        src/ps2/sdl_gamepad.cpp
    )

    target_compile_features(DinoGame PRIVATE cxx_std_20)

    set(PS2_ASSETS_DIR "${CMAKE_BINARY_DIR}/ps2assets")
    set(PS2_ASSETS_HEADER "${PS2_ASSETS_DIR}/ps2_assets.h")

    target_include_directories(DinoGame PRIVATE
        src
        "${PS2_ASSETS_DIR}"
        "$ENV{PS2SDK}/ports/include/SDL2"
    )

    set_target_properties(
        DinoGame PROPERTIES
        OUTPUT_NAME "DINOGAME"
    )

    target_compile_definitions(DinoGame PRIVATE PS2 __PS2__)

    if(PCSX2)
        target_compile_definitions(DinoGame PRIVATE PCSX2 __PCSX2__)
    endif()

    if(TESTS)
        target_compile_definitions(DinoGame PRIVATE AUTOPLAYER)
    endif()

    find_program(
        PS2_PKG_CONFIG
        NAMES mips64r5900el-ps2-elf-pkg-config
        HINTS "$ENV{PS2SDK}/bin"
    )

    set(ENV{PKG_CONFIG_PATH} "$ENV{PS2SDK}/ports/lib/pkgconfig")

    execute_process(
        COMMAND ${PS2_PKG_CONFIG} --static --libs sdl2 SDL2_image SDL2_mixer
        OUTPUT_VARIABLE PS2_SDL_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    separate_arguments(PS2_SDL_LIBS)

    target_link_libraries(DinoGame PRIVATE
        ${PS2_SDL_LIBS}
        SDL2main
    )

    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import PIL"
        RESULT_VARIABLE PILLOW_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
    )

    if(NOT PILLOW_RESULT EQUAL 0)
        message(FATAL_ERROR
            "Python Pillow is required to build assets, but it is not installed.\n"
            "Install it with:\n"
            "  ${Python3_EXECUTABLE} -m pip install Pillow"
        )
    endif()

    message(STATUS "Found Python3: ${Python3_EXECUTABLE}")
    message(STATUS "Found Pillow")

    set(EMBED_ASSETS_COMMAND
        ${Python3_EXECUTABLE}
        "${CMAKE_SOURCE_DIR}/tools/embed_assets.py"
        --output "${PS2_ASSETS_HEADER}"
    )

    if(PCSX2)
        list(APPEND EMBED_ASSETS_COMMAND --pcsx2)
    endif()

    add_custom_command(
        OUTPUT "${PS2_ASSETS_HEADER}"
        COMMAND ${EMBED_ASSETS_COMMAND}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        DEPENDS
            "${CMAKE_SOURCE_DIR}/tools/embed_assets.py"
            "${CMAKE_SOURCE_DIR}/resources/sounds/hit.wav"
            "${CMAKE_SOURCE_DIR}/resources/sounds/button-press.wav"
            "${CMAKE_SOURCE_DIR}/resources/sounds/score-reached.wav"
            "${CMAKE_SOURCE_DIR}/resources/images/ps2_100_percent/offline/100-offline-sprite.png"
            "${CMAKE_SOURCE_DIR}/resources/images/default_100_percent/offline/100-offline-sprite.png"
        COMMENT "Embedding PS2 game assets..."
        VERBATIM
    )

    add_custom_target(
        PS2Assets
        DEPENDS "${PS2_ASSETS_HEADER}"
    )

    add_dependencies(DinoGame PS2Assets)

    install(TARGETS DinoGame
        RUNTIME DESTINATION cdrom0
    )

    install(FILES src/ps2/SYSTEM.CNF DESTINATION cdrom0)

    return()
endif()

if(ANDROID)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
else()
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
endif()

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

if(UWP)
    set(SDL_FILESYSTEM ON CACHE BOOL "" FORCE)
    set(SDL_FILE ON CACHE BOOL "" FORCE)
    set(SDL_STDIO OFF CACHE BOOL "" FORCE)
endif()

include(FetchContent)

FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.8
)

FetchContent_Declare(
    SDL2_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-2.8.2
)

FetchContent_Declare(
    SDL2_mixer
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
    GIT_TAG release-2.8.0
)

FetchContent_MakeAvailable(SDL2 SDL2_image SDL2_mixer)

set(SOURCES
    src/game.cpp
    src/entities/obstacle.cpp
    src/entities/trex.cpp
)

if(UWP)
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

    set_property(
        SOURCE ${UWP_ASSETS}
        PROPERTY VS_DEPLOYMENT_CONTENT 1
    )

    set_property(
        SOURCE ${UWP_ASSETS}
        PROPERTY VS_DEPLOYMENT_LOCATION "assets"
    )

    file(GLOB_RECURSE GAME_RESOURCES "resources/*")

    list(FILTER GAME_RESOURCES EXCLUDE REGEX "ps2_100_percent")

    list(APPEND SOURCES ${GAME_RESOURCES})

    set_property(
        SOURCE ${GAME_RESOURCES}
        PROPERTY VS_DEPLOYMENT_CONTENT 1
    )

    set_property(
        SOURCE ${GAME_RESOURCES}
        PROPERTY VS_DEPLOYMENT_LOCATION "resources"
    )

elseif(ANDROID)
    list(APPEND SOURCES
        src/entry/main.cpp
    )

else()
    list(APPEND SOURCES
        src/entry/main.cpp
        src/entry/main.rc
    )
endif()

if(UWP)
    list(APPEND SOURCES
        "${SDL2_SOURCE_DIR}/src/main/winrt/SDL_winrt_main_NonXAML.cpp"
    )

    set_source_files_properties(
        "${SDL2_SOURCE_DIR}/src/main/winrt/SDL_winrt_main_NonXAML.cpp"
        PROPERTIES COMPILE_FLAGS "/ZW /std:c++17 /EHsc"
    )
endif()

if(ANDROID)
    add_library(DinoGame SHARED ${SOURCES})
else()
    add_executable(DinoGame ${SOURCES})
endif()

target_compile_features(DinoGame PRIVATE cxx_std_20)

target_include_directories(DinoGame PRIVATE src)

target_compile_definitions(
    DinoGame PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

if(UWP)
    target_compile_definitions(
        DinoGame PRIVATE
        UWP
        WINAPI_FAMILY=WINAPI_FAMILY_APP
        _UNICODE
        UNICODE
    )
endif()

if(TESTS)
    target_compile_definitions(DinoGame PRIVATE AUTOPLAYER)
endif()

if(ANDROID)
    target_link_libraries(DinoGame PRIVATE
        SDL2::SDL2
        SDL2_image::SDL2_image-static
        SDL2_mixer::SDL2_mixer-static
    )
else()
    target_link_libraries(DinoGame PRIVATE
        SDL2::SDL2-static
        SDL2_image::SDL2_image-static
        SDL2_mixer::SDL2_mixer-static
    )
endif()

if(UWP)
    target_link_libraries(DinoGame PRIVATE WindowsApp.lib)

elseif(ANDROID)
    target_link_libraries(
        DinoGame PRIVATE
        SDL2::SDL2main
        log
        android
        GLESv1_CM
        GLESv2
    )

else()
    target_link_libraries(DinoGame PRIVATE SDL2::SDL2main)
endif()

if(NOT ANDROID)
    set_target_properties(
        DinoGame PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
else()
    set_target_properties(
        DinoGame PROPERTIES
        OUTPUT_NAME "main"
    )
endif()

if(UWP)
    set_target_properties(
        DinoGame PROPERTIES
        VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION "10.0.17763.0"
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
        VS_WINRT_EXTENSIONS OFF
    )
endif()

if(NOT UWP AND NOT ANDROID)
    add_custom_command(
        TARGET DinoGame
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:DinoGame>/resources"
        COMMENT "Copying resources for desktop build..."
    )
endif()