#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/Basics.h>

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
  ezUInt32 AddJoint(const char* szName, const ezTransform& LocalTransform, ezUInt32 uiParentIndex = 0xFFFFFFFFu);

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
    ezUInt32 m_uiParentIndex = 0xFFFFFFFFu;
    ezHashedString m_sName;
  };

  ezDeque<BuilderJoint> m_Joints;
};
