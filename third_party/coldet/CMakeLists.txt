# ColDet 1.2 (C) 2000 Amir Geva, GNU LGPL (see COPYING). The source in src/ is unmodified from
# the coldet12.zip release; SHA256SUMS lists its files. Platform setup is done here, through
# compile definitions, so the library's own files stay untouched.
set(COLDET_SOURCES
  src/coldet.cpp src/coldet_bld.cpp src/box.cpp src/box_bld.cpp
  src/math3d.cpp src/mytritri.cpp src/sysdep.cpp src/tritri.c)
# tritri.c is C: box.cpp declares tri_tri_intersect extern "C", and Visual C++ (which built
# the original game, via coldet.dsp) compiles .c files as C.

add_library(coldet STATIC ${COLDET_SOURCES})
target_include_directories(coldet PUBLIC src)
# A static library: no dllimport/dllexport decoration.
target_compile_definitions(coldet PUBLIC EXPORT=)
if(WIN32)
  target_compile_definitions(coldet PUBLIC WIN32)
else()
  target_compile_definitions(coldet PUBLIC GCC)
endif()
# Lets the unmodified 2000-era source build on current compilers (see compat/coldet_compat.h).
set(COLDET_COMPAT ${CMAKE_CURRENT_SOURCE_DIR}/compat/coldet_compat.h)
if(MSVC)
  target_compile_options(coldet PRIVATE /W0 /FI${COLDET_COMPAT})
else()
  # third-party code from 2000: keep its warnings out of our build
  target_compile_options(coldet PRIVATE -w -ffp-contract=off -include ${COLDET_COMPAT})
endif()
