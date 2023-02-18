#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/IO/Stream.h>
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

  void AddSample(const ezVec3& vDir, const T& value);

  T Evaluate(const ezVec3& vNormal) const;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

  T m_Values[ezAmbientCubeBasis::NumDirs];
};

#include <Core/Graphics/Implementation/AmbientCubeBasis_inl.h>
