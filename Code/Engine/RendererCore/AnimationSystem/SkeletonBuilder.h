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

  /// \brief Adds a bone to the skeleton
  /// Since the only way to add a bone with a parent is through this method the order of bones in the array is guaranteed
  /// so that child bones always come after their parent bones
  ezUInt32 AddBone(const char* szName, const ezMat4& LocalTransform, ezUInt32 uiParentIndex = 0xFFFFFFFFu);

  /// \brief Sets the skinning mode the skeleton uses
  void SetSkinningMode(ezSkeleton::Mode eSkinningMode);

  /// \brief Creates a skeleton from the accumulated data.
  ezUniquePtr<ezSkeleton> CreateSkeletonInstance() const;

  /// \brief Returns true if there any bones have been added to the skeleton builder
  bool HasBones() const;

protected:
  ezDynamicArray<ezSkeleton::Bone> m_Bones;

  ezSkeleton::Mode m_eSkinningMode;
};
