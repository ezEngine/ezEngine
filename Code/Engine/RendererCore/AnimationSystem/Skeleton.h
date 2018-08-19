#pragma once

#include <RendererCore/Basics.h>

#include <Foundation/Math/Mat3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/Declarations.h>

class ezStreamWriter;
class ezStreamReader;
class ezAnimationPose;
class ezSkeletonBuilder;
class ezSkeleton;

/// \brief Describes a single joint.
/// The transforms of the joints are in their local space and thus need to be correctly multiplied with their parent transforms to get the
/// final transform.
class EZ_RENDERERCORE_DLL ezSkeletonJoint
{
public:
  const ezTransform& GetBindPoseLocalTransform() const { return m_BindPoseLocal; }
  const ezTransform& GetInverseBindPoseGlobalTransform() const { return m_InverseBindPoseGlobal; }

  /// \brief Returns ezInvalidJointIndex if no parent
  ezUInt16 GetParentIndex() const { return m_uiParentIndex; }

  bool IsRootJoint() const { return m_uiParentIndex == ezInvalidJointIndex; }
  const ezHashedString& GetName() const { return m_sName; }

protected:
  friend ezSkeleton;
  friend ezSkeletonBuilder;

  ezTransform m_BindPoseLocal;
  ezTransform m_InverseBindPoseGlobal;
  ezUInt16 m_uiParentIndex = ezInvalidJointIndex;
  ezHashedString m_sName;
};

/// \brief The skeleton class encapsulates the information about the joint structure for a model.
class EZ_RENDERERCORE_DLL ezSkeleton
{
public:
  ezSkeleton();
  ~ezSkeleton();

  /// \brief Returns the number of joints in the skeleton.
  ezUInt16 GetJointCount() const { return m_Joints.GetCount(); }

  /// \brief Returns the nth joint.
  const ezSkeletonJoint& GetJointByIndex(ezUInt16 uiIndex) const { return m_Joints[uiIndex]; }

  /// \brief Allows to find a specific joint in the skeleton by name. Returns ezInvalidJointIndex if not found
  ezUInt16 FindJointByName(const ezTempHashedString& sName) const;

  /// \brief Checks if two skeletons are compatible (same joint count and hierarchy)
  bool IsCompatibleWith(const ezSkeleton& other) const;

  /// \brief Saves the skeleton in a given stream.
  void Save(ezStreamWriter& stream) const;

  /// \brief Loads the skeleton from the given stream.
  void Load(ezStreamReader& stream);

  bool IsJointDescendantOf(ezUInt16 uiJoint, ezUInt16 uiExpectedParent) const;

  /// \brief Applies a global transform to the skeleton (used by the importer to correct scale and up-axis)
  // void ApplyGlobalTransform(const ezMat3& transform);

protected:
  friend ezSkeletonBuilder;

  ezDynamicArray<ezSkeletonJoint> m_Joints;
};
