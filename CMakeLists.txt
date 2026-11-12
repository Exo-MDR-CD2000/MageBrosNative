cmake_minimum_required(VERSION 3.20)
# The project version is the single source of the program's version number.
project(MageBrosNative VERSION 0.4.46 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---------------------------------------------------------------------------------------------
# mbn_core: reading the user's MageBros.exe (PE resources, bitmaps, sounds, .x models, levels).
# No SDL dependency, so the tools and tests build anywhere.
set(MBN_CORE_SOURCES
  src/core/game_files.cpp src/core/image.cpp src/core/level.cpp src/core/log.cpp
  src/core/pe_resources.cpp src/core/sha256.cpp src/core/wave.cpp src/core/xfile.cpp)
add_library(mbn_core STATIC ${MBN_CORE_SOURCES})
target_include_directories(mbn_core PUBLIC src)

if(MSVC)
  target_compile_options(mbn_core PUBLIC /W3 /utf-8 /fp:precise)
  target_compile_definitions(mbn_core PUBLIC _CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
  # the original game was built with x87/SSE floats in program order; no FMA contraction
  target_compile_options(mbn_core PUBLIC -Wall -Wno-unused-parameter -Wno-sign-compare -ffp-contract=off)
endif()
if(MINGW)
  # MinGW-w64 / llvm-mingw: link the C++ runtime (libstdc++ or libc++, libunwind, winpthread)
  # into every exe, so they run without extra DLLs next to them.
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

# MageBrosAssets: inventory, self-test and asset export for the user's own copy.
add_executable(MageBrosAssets tools/mbn_assets.cpp)
target_link_libraries(MageBrosAssets PRIVATE mbn_core)

# Tests that need no game files (the format readers on small hand-written inputs).
enable_testing()
add_executable(mbn_tests tests/format_tests.cpp)
target_link_libraries(mbn_tests PRIVATE mbn_core)
add_test(NAME formats COMMAND mbn_tests)

# The level geometry builder, checked against all 40 levels of the user's copy:
#   mbn_map_tests --game PATH   (added to ctest when configured with -DMBN_GAME_EXE=PATH)
add_executable(mbn_map_tests tests/map_tests.cpp src/game/XMapGeometry.cpp src/framework/d3dutil.cpp)
target_link_libraries(mbn_map_tests PRIVATE mbn_core)
set(MBN_GAME_EXE "" CACHE FILEPATH "MageBros.exe for the tests that need the game")
# Player 1's movement on all 40 levels, run without a renderer.
add_executable(mbn_hero_tests tests/hero_tests.cpp tests/jmodel_stub.cpp src/game/XHero.cpp src/game/XMath.cpp src/game/XCollideBox.cpp
  src/game/XMapCollision.cpp src/game/XMapGeometry.cpp src/framework/d3dutil.cpp)
target_link_libraries(mbn_hero_tests PRIVATE mbn_core)
if(MBN_GAME_EXE)
  add_test(NAME map_geometry COMMAND mbn_map_tests --game ${MBN_GAME_EXE})
  add_test(NAME hero_movement COMMAND mbn_hero_tests --game ${MBN_GAME_EXE})
endif()

# ---------------------------------------------------------------------------------------------
# ColDet, the collision library the original game was built with (LGPL-2.0-or-later, its own
# license in third_party/coldet). Built from its published source when that folder is present.
if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/coldet/CMakeLists.txt)
  add_subdirectory(third_party/coldet)
  add_executable(mbn_collision_tests tests/collision_tests.cpp)
  target_link_libraries(mbn_collision_tests PRIVATE coldet)
  add_test(NAME collision COMMAND mbn_collision_tests)
  install(TARGETS mbn_collision_tests RUNTIME DESTINATION .)
endif()

# ---------------------------------------------------------------------------------------------
# The game itself (SDL3 + OpenGL 3.3). The engine layers and the reconstructed game code are
# being brought up module by module; see docs/ROADMAP.md. Until the game logic lands it opens
# the asset viewer.
option(MBN_BUILD_GAME "Build the MageBrosNative game executable" ON)
option(MBN_BUILD_R36S_PROBE "Build the R36S platform probe (uses the game's SDL3)" OFF)
option(MBN_FETCH_SDL "Download and build SDL3 if it is not installed" ON)
# Packaging and the R36S build must not pick up whatever SDL3 the host happens to have:
# this forces the pinned source build even when a system package is installed.
option(MBN_SDL_FROM_SOURCE "Build the pinned SDL3 from source even if a system one is installed" OFF)

if(MBN_BUILD_GAME)
  if(NOT MBN_SDL_FROM_SOURCE)
    find_package(SDL3 CONFIG QUIET)
  endif()
  if(SDL3_FOUND)
    # say which SDL the build will use: the platform ports differ in how they get it
    message(STATUS "SDL3: system package at ${SDL3_DIR}")
  endif()
  if(NOT SDL3_FOUND)
    if(MBN_FETCH_SDL)
      message(STATUS "SDL3: building release-3.4.18 from source")
      include(FetchContent)
      set(SDL_SHARED OFF CACHE BOOL "" FORCE)
      set(SDL_STATIC ON CACHE BOOL "" FORCE)
      set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
      set(SDL_TESTS OFF CACHE BOOL "" FORCE)
      set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
      FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.18
        GIT_SHALLOW TRUE)
      FetchContent_MakeAvailable(SDL3)
    else()
      message(FATAL_ERROR "SDL3 not found. Install it or enable MBN_FETCH_SDL.")
    endif()
  endif()

  # the engine (MIT, from Hopmon Native) and, as it lands, the reconstructed game code
  file(GLOB_RECURSE MBN_GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/gfx/*.cpp ${CMAKE_SOURCE_DIR}/src/audio/*.cpp
    ${CMAKE_SOURCE_DIR}/src/input/*.cpp ${CMAKE_SOURCE_DIR}/src/ui/*.cpp
    ${CMAKE_SOURCE_DIR}/src/framework/*.cpp ${CMAKE_SOURCE_DIR}/src/game/*.cpp)
  # macOS writes AppleDouble "._name" files onto FAT cards and network shares; they are not
  # source, and a compiler that picks them up fails with a screenful of nonsense
  list(FILTER MBN_GAME_SOURCES EXCLUDE REGEX "/\\._[^/]*$")
  list(APPEND MBN_GAME_SOURCES
    src/main.cpp src/core/assets.cpp src/core/config.cpp src/core/mods.cpp src/core/saves.cpp
    src/core/third_party_impl.cpp)
  if(WIN32)
    # the icon and version information (res/magebros_native.rc)
    add_executable(MageBrosNative WIN32 ${MBN_GAME_SOURCES} res/magebros_native.rc)
    # the resource compiler doesn't track header changes: rebuild the version info when it changes
    set_source_files_properties(res/magebros_native.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/generated/version.h)
    if(MINGW)
      # windres looks for the icon relative to the build folder, so point it at res/ as well
      set_property(SOURCE res/magebros_native.rc APPEND PROPERTY INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/res)
    endif()
  else()
    if(APPLE)
      # a bundle: double-clickable, and it keeps the executable where SDL's base path expects it
      add_executable(MageBrosNative MACOSX_BUNDLE ${MBN_GAME_SOURCES})
    else()
      add_executable(MageBrosNative ${MBN_GAME_SOURCES})
    endif()
  endif()
  configure_file(res/version.h.in ${CMAKE_BINARY_DIR}/generated/version.h @ONLY)
  if(APPLE)
    set_target_properties(MageBrosNative PROPERTIES
      MACOSX_BUNDLE_BUNDLE_NAME MageBrosNative
      MACOSX_BUNDLE_GUI_IDENTIFIER com.magebros.native
      MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
      MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/res/Info.plist.in)
  endif()
  target_include_directories(MageBrosNative PRIVATE src third_party ${CMAKE_BINARY_DIR}/generated)
  target_link_libraries(MageBrosNative PRIVATE mbn_core)
  if(TARGET coldet)
    target_link_libraries(MageBrosNative PRIVATE coldet)
  endif()
  if(TARGET SDL3::SDL3)
    target_link_libraries(MageBrosNative PRIVATE SDL3::SDL3)
  else()
    target_link_libraries(MageBrosNative PRIVATE SDL3::SDL3-static)
  endif()
  if(WIN32)
    target_link_libraries(MageBrosNative PRIVATE winmm)
  elseif(NOT APPLE)
    target_link_libraries(MageBrosNative PRIVATE m pthread)
  endif()
  if(MSVC)
    # main() entry point in a Windows GUI program
    target_link_options(MageBrosNative PRIVATE /ENTRY:mainCRTStartup)
  endif()
  if(APPLE)
    install(TARGETS MageBrosNative BUNDLE DESTINATION .)
  else()
    install(TARGETS MageBrosNative RUNTIME DESTINATION .)
  endif()

  # The R36S platform probe: does SDL3's KMSDRM video, GLES context, ALSA and gamepad work on
  # this device? A development tool, never packaged with the game.
  if(MBN_BUILD_R36S_PROBE)
    add_executable(mbn_r36s_probe tools/r36s/probe.cpp src/gfx/gl.cpp src/core/log.cpp)
    target_include_directories(mbn_r36s_probe PRIVATE src third_party)
    target_link_libraries(mbn_r36s_probe PRIVATE mbn_core)
    if(TARGET SDL3::SDL3)
      target_link_libraries(mbn_r36s_probe PRIVATE SDL3::SDL3)
    else()
      target_link_libraries(mbn_r36s_probe PRIVATE SDL3::SDL3-static)
    endif()
    if(NOT WIN32 AND NOT APPLE)
      target_link_libraries(mbn_r36s_probe PRIVATE m pthread)
    endif()
  endif()
endif()

# ---------------------------------------------------------------------------------------------
# Release zip: `cmake --build build --config Release --target package` (or make_release.bat)
install(TARGETS MageBrosAssets mbn_tests RUNTIME DESTINATION .)
install(FILES README.md LICENSE THIRD_PARTY_NOTICES.md DESTINATION .)
# the LGPL text must travel with binaries that contain ColDet
if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/coldet/COPYING)
  install(FILES third_party/coldet/COPYING DESTINATION licenses RENAME coldet-LGPL.txt)
endif()
file(GLOB MBN_THIRD_PARTY_LICENSES ${CMAKE_SOURCE_DIR}/third_party/*-LICENSE)
install(FILES ${MBN_THIRD_PARTY_LICENSES} DESTINATION licenses)
install(DIRECTORY docs/ DESTINATION docs)
set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_NAME MageBrosNative)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
if(WIN32)
  set(CPACK_SYSTEM_NAME windows-x64)
elseif(APPLE)
  set(CPACK_SYSTEM_NAME macos-arm64)  # Apple Silicon only, as the port targets it
endif()
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)
