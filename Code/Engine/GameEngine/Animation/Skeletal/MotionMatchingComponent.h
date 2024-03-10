// #pragma once
//
// #include <GameEngine/GameEngineDLL.h>
// #include <RendererCore/AnimationSystem/AnimationGraph/AnimationClipSampler.h>
// #include <RendererCore/AnimationSystem/AnimationPose.h>
// #include <RendererCore/Meshes/SkinnedMeshComponent.h>
//
// using ezAnimationClipResourceHandle = ezTypedResourceHandle<class ezAnimationClipResource>;
// using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;
//
// using ezMotionMatchingComponentManager = ezComponentManagerSimple<class ezMotionMatchingComponent, ezComponentUpdateType::WhenSimulating> ;
//
// class EZ_GAMEENGINE_DLL ezMotionMatchingComponent : public ezSkinnedMeshComponent
//{
//   EZ_DECLARE_COMPONENT_TYPE(ezMotionMatchingComponent, ezSkinnedMeshComponent, ezMotionMatchingComponentManager);
//
//   //////////////////////////////////////////////////////////////////////////
//   // ezComponent
//
// public:
//   virtual void SerializeComponent(ezWorldWriter& stream) const override;
//   virtual void DeserializeComponent(ezWorldReader& stream) override;
//
// protected:
//   virtual void OnSimulationStarted() override;
//
//
//   //////////////////////////////////////////////////////////////////////////
//   // ezMotionMatchingComponent
//
// public:
//   ezMotionMatchingComponent();
//   ~ezMotionMatchingComponent();
//
//   void SetAnimation(ezUInt32 uiIndex, const ezAnimationClipResourceHandle& hResource);
//   ezAnimationClipResourceHandle GetAnimation(ezUInt32 uiIndex) const;
//
// protected:
//   void Update();
//
//   ezUInt32 Animations_GetCount() const;                          // [ property ]
//   const char* Animations_GetValue(ezUInt32 uiIndex) const;       // [ property ]
//   void Animations_SetValue(ezUInt32 uiIndex, const char* value); // [ property ]
//   void Animations_Insert(ezUInt32 uiIndex, const char* value);   // [ property ]
//   void Animations_Remove(ezUInt32 uiIndex);                      // [ property ]
//
//   void ConfigureInput();
//   ezVec3 GetInputDirection() const;
//   ezQuat GetInputRotation() const;
//
//   ezAnimationPose m_AnimationPose;
//   ezSkeletonResourceHandle m_hSkeleton;
//
//   ezDynamicArray<ezAnimationClipResourceHandle> m_Animations;
//
//   ezVec3 m_vLeftFootPos;
//   ezVec3 m_vRightFootPos;
//
//   struct MotionData
//   {
//     ezUInt16 m_uiAnimClipIndex;
//     ezUInt16 m_uiKeyframeIndex;
//     ezVec3 m_vLeftFootPosition;
//     ezVec3 m_vLeftFootVelocity;
//     ezVec3 m_vRightFootPosition;
//     ezVec3 m_vRightFootVelocity;
//     ezVec3 m_vRootVelocity;
//   };
//
//   struct TargetKeyframe
//   {
//     ezUInt16 m_uiAnimClip;
//     ezUInt16 m_uiKeyframe;
//   };
//
//   TargetKeyframe m_Keyframe0;
//   TargetKeyframe m_Keyframe1;
//   float m_fKeyframeLerp = 0.0f;
//
//   TargetKeyframe FindNextKeyframe(const TargetKeyframe& current, const ezVec3& vTargetDir) const;
//
//   ezDynamicArray<MotionData> m_MotionData;
//
//   static void PrecomputeMotion(ezDynamicArray<MotionData>& motionData, ezTempHashedString jointName1, ezTempHashedString jointName2,
//     const ezAnimationClipResourceDescriptor& animClip, ezUInt16 uiAnimClipIndex, const ezSkeleton& skeleton);
//
//   ezUInt32 FindBestKeyframe(const TargetKeyframe& current, ezVec3 vLeftFootPosition, ezVec3 vRightFootPosition, ezVec3 vTargetDir) const;
// };
