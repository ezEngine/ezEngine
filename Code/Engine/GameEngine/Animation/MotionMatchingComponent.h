#pragma once

#include <GameEngine/Basics.h>
#include <RendererCore/AnimationSystem/AnimationGraph/AnimationClipSampler.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/Meshes/MeshComponentBase.h>

typedef ezTypedResourceHandle<class ezAnimationClipResource> ezAnimationClipResourceHandle;
typedef ezTypedResourceHandle<class ezSkeletonResource> ezSkeletonResourceHandle;

typedef ezComponentManagerSimple<class ezMotionMatchingComponent, ezComponentUpdateType::WhenSimulating> ezMotionMatchingComponentManager;

class EZ_GAMEENGINE_DLL ezMotionMatchingComponent : public ezMeshComponentBase
{
  EZ_DECLARE_COMPONENT_TYPE(ezMotionMatchingComponent, ezMeshComponentBase, ezMotionMatchingComponentManager);

public:
  ezMotionMatchingComponent();
  ~ezMotionMatchingComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent Interface
  //
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  //

  //////////////////////////////////////////////////////////////////////////
  // Properties
  //

  void Update();

  void SetAnimation(ezUInt32 uiIndex, const ezAnimationClipResourceHandle& hResource);
  ezAnimationClipResourceHandle GetAnimation(ezUInt32 uiIndex) const;

protected:
  ezUInt32 Animations_GetCount() const;
  const char* Animations_GetValue(ezUInt32 uiIndex) const;
  void Animations_SetValue(ezUInt32 uiIndex, const char* value);
  void Animations_Insert(ezUInt32 uiIndex, const char* value);
  void Animations_Remove(ezUInt32 uiIndex);

  void ConfigureInput();
  ezVec3 GetInputDirection() const;
  ezQuat GetInputRotation() const;

  ezAnimationPose m_AnimationPose;
  ezSkeletonResourceHandle m_hSkeleton;

  ezDynamicArray<ezAnimationClipResourceHandle> m_Animations;

  ezVec3 m_vLeftFootPos;
  ezVec3 m_vRightFootPos;

  struct MotionData
  {
    ezUInt16 m_uiAnimClipIndex;
    ezUInt16 m_uiKeyframeIndex;
    ezVec3 m_vLeftFootPosition;
    ezVec3 m_vLeftFootVelocity;
    ezVec3 m_vRightFootPosition;
    ezVec3 m_vRightFootVelocity;
    ezVec3 m_vRootVelocity;
  };

  struct TargetKeyframe
  {
    ezUInt16 m_uiAnimClip;
    ezUInt16 m_uiKeyframe;
  };

  TargetKeyframe m_Keyframe0;
  TargetKeyframe m_Keyframe1;
  float m_fKeyframeLerp = 0.0f;

  TargetKeyframe FindNextKeyframe(const TargetKeyframe& current, const ezVec3& vTargetDir) const;

  ezDynamicArray<MotionData> m_MotionData;

  static void PrecomputeMotion(ezDynamicArray<MotionData>& motionData, ezTempHashedString jointName1, ezTempHashedString jointName2,
                               const ezAnimationClipResourceDescriptor& animClip, ezUInt16 uiAnimClipIndex, const ezSkeleton& skeleton);

  ezUInt32 FindBestKeyframe(const TargetKeyframe& current, ezVec3 vLeftFootPosition, ezVec3 vRightFootPosition,
                            ezVec3 vTargetDir) const;
};

