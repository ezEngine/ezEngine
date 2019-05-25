#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Math/Vec3.h>

struct EZ_CORE_DLL ezAmbientCubeBasis
{
  enum
  {
    PosX = 0,
    NegX,
    PosY,
    NegY,
    PosZ,
    NegZ,

    NumDirs = 6
  };

  static ezVec3 s_Dirs[NumDirs];
};

template <typename T>
struct ezAmbientCube
{
  EZ_DECLARE_POD_TYPE();

  ezAmbientCube();

  template <typename U>
  ezAmbientCube(const ezAmbientCube<U>& other);

  template <typename U>
  void operator=(const ezAmbientCube<U>& other);

  bool operator==(const ezAmbientCube& other) const;
  bool operator!=(const ezAmbientCube& other) const;

  T Evaluate(const ezVec3& vNormal);

  T m_Values[ezAmbientCubeBasis::NumDirs];
};

#include <Core/Graphics/Implementation/AmbientCubeBasis_inl.h>
