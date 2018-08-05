#pragma once

#include <Foundation/Math/Mat3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Basics.h>

class ezStreamWriter;
class ezStreamReader;
class ezAnimationPose;
class ezSkeletonBuilder;
class ezSkeleton;

// TODOs:
// - decide if bone name should be hashed string

/// \brief The skeleton class encapsulates the information about the bone structure for a model.
class EZ_RENDERERCORE_DLL ezSkeleton : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkeleton, ezReflectedClass);

public:
  /// \brief Describes a single bone.
  /// The transforms of the bones are in their local space and thus need to be correctly multiplied with their parent transforms to get the
  /// final transform.
  class EZ_RENDERERCORE_DLL Bone
  {
  public:
    inline const ezMat4& GetBoneTransform() const;
    inline const ezMat4& GetInverseBindPoseTransform() const;
    inline ezUInt32 GetParentIndex() const;
    inline bool IsRootBone() const;
    inline const ezHashedString& GetName() const;

  protected:
    friend ezSkeletonBuilder;
    friend ezSkeleton;

    ezMat4 m_BoneTransform;
    ezMat4 m_InverseBindPoseTransform;
    ezUInt32 m_uiParentIndex; // 0xFFFFFFFFu if no parent

    ezHashedString m_sName;
  };

  /// \brief The available skinning modes, says if a skeleton uses single or four bone skinning
  enum class Mode
  {
    SingleBone,
    FourBones
  };

  ezSkeleton();

  ezSkeleton(const ezSkeleton& Other);

  ~ezSkeleton();

  void operator=(const ezSkeleton& Other);

  /// \brief Returns the number of bones in the skeleton.
  ezUInt32 GetBoneCount() const;

  /// \brief Returns the bone given the bone index.
  inline const Bone& GetBone(ezUInt32 uiBoneIndex) const;

  /// \brief Allows to find a specific bone in the skeleton by name, returns true if a bone is found, false otherwise.
  bool FindBoneByName(const ezTempHashedString& sBoneName, ezUInt32& out_uiBoneIndex) const;

  /// \brief Creates a new pose object with storage for all bone transforms of the skeleton.
  ezUniquePtr<ezAnimationPose> CreatePose() const;

  /// \brief Sets the animation pose to the bind pose of this skeleton.
  void SetAnimationPoseToBindPose(ezAnimationPose* pPose) const;

  /// \brief Converts the given animation pose from a collection of local transforms to a usable object space matrix collection.
  void CalculateObjectSpaceAnimationPoseMatrices(ezAnimationPose* pPose) const;

  /// \brief Checks if two skeletons are compatible (same bone count and hierarchy)
  bool IsCompatibleWith(const ezSkeleton* pOtherSkeleton) const;

  /// \brief Saves the skeleton in a given stream.
  void Save(ezStreamWriter& stream) const;

  /// \brief Loads the skeleton from the given stream.
  ezResult Load(ezStreamReader& stream);

  /// \brief Applies a global transform to the skeleton (used by the importer to correct scale and up-axis)
  void ApplyGlobalTransform(const ezMat3& transform);

  /// \brief Returns the bind space pose this skeleton represents. All pose matrices are still relative.
  const ezAnimationPose* GetBindSpacePose() const;

  /// \brief Returns the bind space pose this skeleton represents. All pose matrices are the final matrices necessary for skinning.
  const ezAnimationPose* GetBindSpacePoseInObjectSpace() const;

  /// \brief Returns the skinning mode of the skeleton
  Mode GetSkinningMode() const;

protected:
  friend ezSkeletonBuilder;

  ezDynamicArray<Bone> m_Bones;

  /// These poses are generated on demand
  mutable ezUniquePtr<ezAnimationPose> m_pBindSpacePose;
  mutable ezUniquePtr<ezAnimationPose> m_pObjectSpaceBindPose;

  Mode m_eSkinningMode;

  mutable bool m_bAnyPoseCreated;
};

#include <RendererCore/AnimationSystem/Implementation/Skeleton_inl.h>
