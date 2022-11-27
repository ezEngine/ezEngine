#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/RendererCoreDLL.h>

/// \brief The skeleton builder class provides the means to build skeleton instances from scratch.
/// This class is not necessary to use skeletons, usually they should be deserialized from data created by the tools.
class EZ_RENDERERCORE_DLL ezSkeletonBuilder
{

public:
  ezSkeletonBuilder();
  ~ezSkeletonBuilder();

  /// \brief Adds a joint to the skeleton
  /// Since the only way to add a joint with a parent is through this method the order of joints in the array is guaranteed
  /// so that child joints always come after their parent joints
  ezUInt16 AddJoint(const char* szName, const ezTransform& localBindPose, ezUInt16 uiParentIndex = ezInvalidJointIndex);

  void SetJointLimit(ezUInt16 uiJointIndex, const ezQuat& localOrientation, bool bLimitSwing, ezAngle halfSwingLimitY, ezAngle halfSwingLimitZ, bool bLimitTwist, ezAngle twistLimitHalfAngle, ezAngle twistLimitCenterAngle);

  /// \brief Creates a skeleton from the accumulated data.
  void BuildSkeleton(ezSkeleton& skeleton) const;

  /// \brief Returns true if there any joints have been added to the skeleton builder
  bool HasJoints() const;

protected:
  struct BuilderJoint
  {
    ezTransform m_BindPoseLocal;
    ezTransform m_BindPoseGlobal; // this one is temporary and not stored in the final ezSkeleton
    ezTransform m_InverseBindPoseGlobal;
    ezUInt16 m_uiParentIndex = ezInvalidJointIndex;
    ezHashedString m_sName;
    ezQuat m_qLocalJointOrientation = ezQuat::IdentityQuaternion();
    ezAngle m_HalfSwingLimitZ;
    ezAngle m_HalfSwingLimitY;
    ezAngle m_TwistLimitHalfAngle;
    ezAngle m_TwistLimitCenterAngle;
    bool m_bLimitTwist = false;
    bool m_bLimitSwing = false;
  };

  ezDeque<BuilderJoint> m_Joints;
};
