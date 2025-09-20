#pragma once

#include <Foundation/Math/Vec3.h>

#include <Core/World/Declarations.h>
#include <Foundation/Types/RefCounted.h>

/// \brief Defines a coordinate system through three orthogonal direction vectors.
///
/// Represents a 3D coordinate system using forward, right, and up direction vectors.
/// Used for transforming between different coordinate conventions in the engine.
struct EZ_CORE_DLL ezCoordinateSystem
{
  EZ_DECLARE_POD_TYPE();

  ezVec3 m_vForwardDir;
  ezVec3 m_vRightDir;
  ezVec3 m_vUpDir;
};

/// \brief Abstract base class for providing coordinate systems at specific world positions.
///
/// Allows defining position-dependent coordinate systems within a world. Derived classes
/// implement GetCoordinateSystem to provide custom coordinate transformations based on
/// world position, enabling features like gravity-aligned coordinate systems or curved spaces.
class EZ_CORE_DLL ezCoordinateSystemProvider : public ezRefCounted
{
public:
  ezCoordinateSystemProvider(const ezWorld* pOwnerWorld)
    : m_pOwnerWorld(pOwnerWorld)
  {
  }

  virtual ~ezCoordinateSystemProvider() = default;

  /// \brief Returns the coordinate system at the given global position.
  virtual void GetCoordinateSystem(const ezVec3& vGlobalPosition, ezCoordinateSystem& out_coordinateSystem) const = 0;

protected:
  friend class ezWorld;

  const ezWorld* m_pOwnerWorld;
};

/// \brief Helper class to convert between two ezCoordinateSystem spaces.
///
/// All functions will do an identity transform until SetConversion is called to set up
/// the conversion. Afterwards the convert functions can be used to convert between
/// the two systems in both directions.
/// Currently, only uniformly scaled orthogonal coordinate systems are supported.
/// They can however be right handed or left handed.
class EZ_CORE_DLL ezCoordinateSystemConversion
{
public:
  /// \brief Creates a new conversion that until set up, does identity conversions.
  ezCoordinateSystemConversion(); // [tested]

  /// \brief Set up the source and target coordinate systems.
  void SetConversion(const ezCoordinateSystem& source, const ezCoordinateSystem& target); // [tested]
  /// \brief Returns the equivalent point in the target coordinate system.
  ezVec3 ConvertSourcePosition(const ezVec3& vPos) const; // [tested]
  /// \brief Returns the equivalent rotation in the target coordinate system.
  ezQuat ConvertSourceRotation(const ezQuat& qOrientation) const; // [tested]
  /// \brief Returns the equivalent length in the target coordinate system.
  float ConvertSourceLength(float fLength) const; // [tested]

  /// \brief Returns the equivalent point in the source coordinate system.
  ezVec3 ConvertTargetPosition(const ezVec3& vPos) const; // [tested]
  /// \brief Returns the equivalent rotation in the source coordinate system.
  ezQuat ConvertTargetRotation(const ezQuat& qOrientation) const; // [tested]
  /// \brief Returns the equivalent length in the source coordinate system.
  float ConvertTargetLength(float fLength) const; // [tested]

private:
  ezMat3 m_mSourceToTarget;
  ezMat3 m_mTargetToSource;
  float m_fWindingSwap = 1.0f;
  float m_fSourceToTargetScale = 1.0f;
  float m_fTargetToSourceScale = 1.0f;
};
