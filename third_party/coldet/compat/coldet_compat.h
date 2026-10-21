// Part of MageBrosNative's build of ColDet (not part of ColDet itself), force-included into
// every ColDet source file so the library builds unmodified on current compilers.
//
// ColDet 1.2 declares PitchMatrix3D, YawMatrix3D and RollMatrix3D only as friends of Matrix3D,
// then calls them before their definitions further down math3d.h. Compilers of 2000 made a
// friend declaration visible at namespace scope ("friend injection"); standard C++ does not.
// Declaring them here first gives the same result.
#pragma once
#ifdef __cplusplus
struct Matrix3D;
inline Matrix3D PitchMatrix3D(const float theta);
inline Matrix3D YawMatrix3D(const float theta);
inline Matrix3D RollMatrix3D(const float theta);
#endif
