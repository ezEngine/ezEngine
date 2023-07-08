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
  ezUInt16 AddJoint(ezStringView sName, const ezTransform& localRestPose, ezUInt16 uiParentIndex = ezInvalidJointIndex);

  void SetJointLimit(ezUInt16 uiJointIndex, const ezQuat& qLocalOrientation, ezSkeletonJointType::Enum jointType, ezAngle halfSwingLimitY, ezAngle halfSwingLimitZ, ezAngle twistLimitHalfAngle, ezAngle twistLimitCenterAngle, float fStiffness);

  void SetJointSurface(ezUInt16 uiJointIndex, ezStringView sSurface);
  void SetJointCollisionLayer(ezUInt16 uiJointIndex, ezUInt8 uiCollsionLayer);

  /// \brief Creates a skeleton from the accumulated data.
  void BuildSkeleton(ezSkeleton& ref_skeleton) const;

  /// \brief Returns true if there any joints have been added to the skeleton builder
  bool HasJoints() const;

protected:
  struct BuilderJoint
  {
    ezTransform m_RestPoseLocal;
    ezTransform m_RestPoseGlobal; // this one is temporary and not stored in the final ezSkeleton
    ezTransform m_InverseRestPoseGlobal;
    ezUInt16 m_uiParentIndex = ezInvalidJointIndex;
    ezHashedString m_sName;
    ezEnum<ezSkeletonJointType> m_JointType;
    ezQuat m_qLocalJointOrientation = ezQuat::IdentityQuaternion();
    ezAngle m_HalfSwingLimitZ;
    ezAngle m_HalfSwingLimitY;
    ezAngle m_TwistLimitHalfAngle;
    ezAngle m_TwistLimitCenterAngle;
    float m_fStiffness = 0.0f;

    ezString m_sSurface;
    ezUInt8 m_uiCollisionLayer = 0;
  };

  ezDeque<BuilderJoint> m_Joints;
};
