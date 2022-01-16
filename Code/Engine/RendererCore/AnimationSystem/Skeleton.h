#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Math/Mat3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/Declarations.h>

class ezStreamWriter;
class ezStreamReader;
class ezSkeletonBuilder;
class ezSkeleton;

namespace ozz::animation
{
  class Skeleton;
}

/// \brief Describes a single joint.
/// The transforms of the joints are in their local space and thus need to be correctly multiplied with their parent transforms to get the
/// final transform.
class EZ_RENDERERCORE_DLL ezSkeletonJoint
{
public:
  const ezTransform& GetBindPoseLocalTransform() const { return m_BindPoseLocal; }

  /// \brief Returns ezInvalidJointIndex if no parent
  ezUInt16 GetParentIndex() const { return m_uiParentIndex; }

  bool IsRootJoint() const { return m_uiParentIndex == ezInvalidJointIndex; }
  const ezHashedString& GetName() const { return m_sName; }

  ezAngle GetHalfSwingLimitY() const { return m_HalfSwingLimitY; }
  ezAngle GetHalfSwingLimitZ() const { return m_HalfSwingLimitZ; }
  ezAngle GetTwistLimitLow() const { return m_TwistLimitLow; }
  ezAngle GetTwistLimitHigh() const { return m_TwistLimitHigh; }

  ezQuat GetLocalOrientation() const { return m_qLocalJointOrientation; }

protected:
  friend ezSkeleton;
  friend ezSkeletonBuilder;

  ezTransform m_BindPoseLocal;
  ezUInt16 m_uiParentIndex = ezInvalidJointIndex;
  ezHashedString m_sName;

  ezQuat m_qLocalJointOrientation = ezQuat::IdentityQuaternion();
  ezAngle m_HalfSwingLimitY;
  ezAngle m_HalfSwingLimitZ;
  ezAngle m_TwistLimitLow;
  ezAngle m_TwistLimitHigh;
};

/// \brief The skeleton class encapsulates the information about the joint structure for a model.
class EZ_RENDERERCORE_DLL ezSkeleton
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezSkeleton);

public:
  ezSkeleton();
  ezSkeleton(ezSkeleton&& rhs);
  ~ezSkeleton();

  void operator=(ezSkeleton&& rhs);

  /// \brief Returns the number of joints in the skeleton.
  ezUInt16 GetJointCount() const { return m_Joints.GetCount(); }

  /// \brief Returns the nth joint.
  const ezSkeletonJoint& GetJointByIndex(ezUInt16 uiIndex) const { return m_Joints[uiIndex]; }

  /// \brief Allows to find a specific joint in the skeleton by name. Returns ezInvalidJointIndex if not found
  ezUInt16 FindJointByName(const ezTempHashedString& sName) const;

  /// \brief Checks if two skeletons are compatible (same joint count and hierarchy)
  //bool IsCompatibleWith(const ezSkeleton& other) const;

  /// \brief Saves the skeleton in a given stream.
  void Save(ezStreamWriter& stream) const;

  /// \brief Loads the skeleton from the given stream.
  void Load(ezStreamReader& stream);

  bool IsJointDescendantOf(ezUInt16 uiJoint, ezUInt16 uiExpectedParent) const;

  const ozz::animation::Skeleton& GetOzzSkeleton() const;

  ezUInt64 GetHeapMemoryUsage() const;

  /// \brief The direction in which the bones shall point for visualization
  ezEnum<ezBasisAxis> m_BoneDirection;

protected:
  friend ezSkeletonBuilder;

  ezDynamicArray<ezSkeletonJoint> m_Joints;
  mutable ezUniquePtr<ozz::animation::Skeleton> m_pOzzSkeleton;
};
