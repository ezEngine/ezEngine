#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Math/Vec3.h>

/// \brief Defines the basis directions for ambient cube sampling.
///
/// Provides the six cardinal directions (positive/negative X, Y, Z) used for
/// ambient lighting calculations and directional sampling.
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

/// \brief Template class for storing ambient lighting data in a cube format.
///
/// Stores lighting values for six directions (the cardinal axes) to approximate
/// ambient lighting. Values can be added via directional samples and evaluated
/// for any normal direction using trilinear interpolation.
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
