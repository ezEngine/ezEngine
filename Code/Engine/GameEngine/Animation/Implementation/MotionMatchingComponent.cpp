#include <PCH.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/MotionMatchingComponent.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezMotionMatchingComponent, 2, ezComponentMode::Dynamic);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Animated Mesh")),
    EZ_ARRAY_ACCESSOR_PROPERTY("Animations", Animations_GetCount, Animations_GetValue, Animations_SetValue, Animations_Insert, Animations_Remove)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezMotionMatchingComponent::ezMotionMatchingComponent() {}

ezMotionMatchingComponent::~ezMotionMatchingComponent() {}

void ezMotionMatchingComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s.WriteArray(m_Animations);
}

void ezMotionMatchingComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  if (uiVersion >= 2)
  {
    s.ReadArray(m_Animations);
  }
}

void ezMotionMatchingComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // make sure the skinning buffer is deleted
  EZ_ASSERT_DEBUG(m_hSkinningTransformsBuffer.IsInvalidated(), "The skinning buffer should not exist at this time");

  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::NoFallback);
    m_hSkeleton = pMesh->GetSkeleton();
  }

  if (m_hSkeleton.IsValid())
  {
    ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::NoFallback);

    const ezSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
    m_AnimationPose.Configure(skeleton);
    m_AnimationPose.ConvertFromLocalSpaceToObjectSpace(skeleton);
    m_AnimationPose.ConvertFromObjectSpaceToSkinningSpace(skeleton);

    // m_SkinningMatrices = m_AnimationPose.GetAllTransforms();

    // Create the buffer for the skinning matrices
    ezGALBufferCreationDescription BufferDesc;
    BufferDesc.m_uiStructSize = sizeof(ezMat4);
    BufferDesc.m_uiTotalSize = BufferDesc.m_uiStructSize * m_AnimationPose.GetTransformCount();
    BufferDesc.m_bUseAsStructuredBuffer = true;
    BufferDesc.m_bAllowShaderResourceView = true;
    BufferDesc.m_ResourceAccess.m_bImmutable = false;

    m_hSkinningTransformsBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(
        BufferDesc,
        ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(m_AnimationPose.GetAllTransforms().GetPtr()), BufferDesc.m_uiTotalSize));
  }

  // m_AnimationClipSampler.RestartAnimation();

  if (m_Animations.IsEmpty())
    return;

  m_Keyframe0.m_uiAnimClip = 0;
  m_Keyframe0.m_uiKeyframe = 0;
  m_Keyframe1.m_uiAnimClip = 0;
  m_Keyframe1.m_uiKeyframe = 1;

  for (ezUInt32 anim = 0; anim < m_Animations.GetCount(); ++anim)
  {
    ezResourceLock<ezAnimationClipResource> pClip(m_Animations[anim], ezResourceAcquireMode::NoFallback);
    ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::AllowFallback);

    PrecomputeMotion(m_MotionData, "Bip01_L_Foot", "Bip01_R_Foot", pClip->GetDescriptor(), anim, pSkeleton->GetDescriptor().m_Skeleton);
  }

  m_vLeftFootPos.SetZero();
  m_vRightFootPos.SetZero();

  ConfigureInput();
}
void ezMotionMatchingComponent::ConfigureInput()
{
  ezInputActionConfig iac;
  iac.m_bApplyTimeScaling = false;

  iac.m_sInputSlotTrigger[0] = ezInputSlot_Controller0_LeftStick_PosY;
  iac.m_sInputSlotTrigger[1] = ezInputSlot_KeyUp;
  ezInputManager::SetInputActionConfig("mm", "forward", iac, true);

  iac.m_sInputSlotTrigger[0] = ezInputSlot_Controller0_LeftStick_NegY;
  iac.m_sInputSlotTrigger[1] = ezInputSlot_KeyDown;
  ezInputManager::SetInputActionConfig("mm", "backward", iac, true);

  iac.m_sInputSlotTrigger[0] = ezInputSlot_Controller0_LeftStick_NegX;
  iac.m_sInputSlotTrigger[1].Clear();
  ezInputManager::SetInputActionConfig("mm", "left", iac, true);

  iac.m_sInputSlotTrigger[0] = ezInputSlot_Controller0_LeftStick_PosX;
  iac.m_sInputSlotTrigger[1].Clear();
  ezInputManager::SetInputActionConfig("mm", "right", iac, true);

  iac.m_bApplyTimeScaling = true;

  iac.m_sInputSlotTrigger[0] = ezInputSlot_Controller0_RightStick_PosX;
  iac.m_sInputSlotTrigger[1] = ezInputSlot_KeyRight;
  // iac.m_sInputSlotTrigger[1] = ezInputSlot_KeyRight;
  ezInputManager::SetInputActionConfig("mm", "turnright", iac, true);

  iac.m_sInputSlotTrigger[0] = ezInputSlot_Controller0_RightStick_NegX;
  iac.m_sInputSlotTrigger[1] = ezInputSlot_KeyLeft;
  // iac.m_sInputSlotTrigger[1] = ezInputSlot_KeyRight;
  ezInputManager::SetInputActionConfig("mm", "turnleft", iac, true);
}

ezVec3 ezMotionMatchingComponent::GetInputDirection() const
{
  float fw, bw, l, r;

  ezInputManager::GetInputActionState("mm", "forward", &fw);
  ezInputManager::GetInputActionState("mm", "backward", &bw);
  ezInputManager::GetInputActionState("mm", "left", &l);
  ezInputManager::GetInputActionState("mm", "right", &r);

  ezVec3 dir;
  dir.y = -(fw - bw);
  dir.x = r - l;
  dir.z = 0;

  // dir.NormalizeIfNotZero(ezVec3::ZeroVector());
  return dir * 3.0f;
}

ezQuat ezMotionMatchingComponent::GetInputRotation() const
{
  float tl, tr;

  ezInputManager::GetInputActionState("mm", "turnleft", &tl);
  ezInputManager::GetInputActionState("mm", "turnright", &tr);

  const ezAngle turn = ezAngle::Degree((tr - tl) * 90.0f);

  ezQuat q;
  q.SetFromAxisAndAngle(ezVec3(0, 0, 1), turn);
  return q;
}

void ezMotionMatchingComponent::Update()
{
  if (!m_hSkeleton.IsValid() || m_Animations.IsEmpty())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::AllowFallback);
  const ezSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  // ezTransform rootMotion;
  // rootMotion.SetIdentity();

  const float fKeyframeFraction = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds() * 24.0f; // assuming 24 FPS in the animations

  {
    const ezVec3 vTargetDir = GetInputDirection() / GetOwner()->GetGlobalScaling().x;

    ezStringBuilder tmp;
    tmp.Format("Gamepad: {0} / {1}", ezArgF(vTargetDir.x, 1), ezArgF(vTargetDir.y, 1));
    ezDebugRenderer::Draw2DText(GetWorld(), tmp, ezVec2I32(10, 10), ezColor::White);

    m_fKeyframeLerp += fKeyframeFraction;
    while (m_fKeyframeLerp > 1.0f)
    {

      m_Keyframe0 = m_Keyframe1;
      m_Keyframe1 = FindNextKeyframe(m_Keyframe1, vTargetDir);

      // ezLog::Info("Old KF: {0} | {1} - {2}", m_Keyframe0.m_uiAnimClip, m_Keyframe0.m_uiKeyframe, m_fKeyframeLerp);
      m_fKeyframeLerp -= 1.0f;
      // ezLog::Info("New KF: {0} | {1} - {2}", m_Keyframe1.m_uiAnimClip, m_Keyframe1.m_uiKeyframe, m_fKeyframeLerp);
    }
  }

  m_AnimationPose.SetToBindPoseInLocalSpace(skeleton);

  {
    ezResourceLock<ezAnimationClipResource> pAnimClip0(m_Animations[m_Keyframe0.m_uiAnimClip], ezResourceAcquireMode::NoFallback);
    ezResourceLock<ezAnimationClipResource> pAnimClip1(m_Animations[m_Keyframe1.m_uiAnimClip], ezResourceAcquireMode::NoFallback);

    const auto& animDesc0 = pAnimClip0->GetDescriptor();
    const auto& animDesc1 = pAnimClip1->GetDescriptor();

    const auto& animatedJoints0 = animDesc0.GetAllJointIndices();

    for (ezUInt32 b = 0; b < animatedJoints0.GetCount(); ++b)
    {
      const ezHashedString sJointName = animatedJoints0.GetKey(b);
      const ezUInt32 uiAnimJointIdx0 = animatedJoints0.GetValue(b);
      const ezUInt32 uiAnimJointIdx1 = animDesc1.FindJointIndexByName(sJointName);

      const ezUInt16 uiSkeletonJointIdx = skeleton.FindJointByName(sJointName);
      if (uiSkeletonJointIdx != ezInvalidJointIndex)
      {
        ezArrayPtr<const ezTransform> pTransforms0 = animDesc0.GetJointKeyframes(uiAnimJointIdx0);
        ezArrayPtr<const ezTransform> pTransforms1 = animDesc1.GetJointKeyframes(uiAnimJointIdx1);

        const ezTransform jointTransform1 = pTransforms0[m_Keyframe0.m_uiKeyframe];
        const ezTransform jointTransform2 = pTransforms1[m_Keyframe1.m_uiKeyframe];

        ezTransform res;
        res.m_vPosition = ezMath::Lerp(jointTransform1.m_vPosition, jointTransform2.m_vPosition, m_fKeyframeLerp);
        res.m_qRotation.SetSlerp(jointTransform1.m_qRotation, jointTransform2.m_qRotation, m_fKeyframeLerp);
        res.m_vScale = ezMath::Lerp(jointTransform1.m_vScale, jointTransform2.m_vScale, m_fKeyframeLerp);

        m_AnimationPose.SetTransform(uiSkeletonJointIdx, res.GetAsMat4());
      }
    }

    // root motion
    {
      auto* pOwner = GetOwner();

      ezVec3 vRootMotion0, vRootMotion1;
      vRootMotion0.SetZero();
      vRootMotion1.SetZero();

      if (animDesc0.HasRootMotion())
        vRootMotion0 = animDesc0.GetJointKeyframes(animDesc0.GetRootMotionJoint())[m_Keyframe0.m_uiKeyframe].m_vPosition;
      if (animDesc1.HasRootMotion())
        vRootMotion1 = animDesc1.GetJointKeyframes(animDesc1.GetRootMotionJoint())[m_Keyframe1.m_uiKeyframe].m_vPosition;

      const ezVec3 vRootMotion =
          ezMath::Lerp(vRootMotion0, vRootMotion1, m_fKeyframeLerp) * fKeyframeFraction * pOwner->GetGlobalScaling().x;

      const ezQuat qRotate = GetInputRotation();

      const ezQuat qOldRot = pOwner->GetLocalRotation();
      const ezVec3 vNewPos = qOldRot * vRootMotion + pOwner->GetLocalPosition();
      const ezQuat qNewRot = qRotate * qOldRot;

      pOwner->SetLocalPosition(vNewPos);
      pOwner->SetLocalRotation(qNewRot);
    }
  }

  m_AnimationPose.ConvertFromLocalSpaceToObjectSpace(skeleton);

  const ezUInt16 uiLeftFootJoint = skeleton.FindJointByName("Bip01_L_Foot");
  const ezUInt16 uiRightFootJoint = skeleton.FindJointByName("Bip01_R_Foot");
  if (uiLeftFootJoint != ezInvalidJointIndex && uiRightFootJoint != ezInvalidJointIndex)
  {
    ezTransform tLeft, tRight;
    ezBoundingSphere sphere(ezVec3::ZeroVector(), 0.5f);

    tLeft.SetFromMat4(m_AnimationPose.GetTransform(uiLeftFootJoint));
    tRight.SetFromMat4(m_AnimationPose.GetTransform(uiRightFootJoint));

    m_AnimationPose.VisualizePose(GetWorld(), skeleton, GetOwner()->GetGlobalTransform(), 1.0f / 6.0f, uiLeftFootJoint);
    m_AnimationPose.VisualizePose(GetWorld(), skeleton, GetOwner()->GetGlobalTransform(), 1.0f / 6.0f, uiRightFootJoint);

    // const float fScaleToPerSec = (float)(1.0 / GetWorld()->GetClock().GetTimeDiff().GetSeconds());

    // const ezVec3 vLeftFootVel = (tLeft.m_vPosition - m_vLeftFootPos) * fScaleToPerSec;
    // const ezVec3 vRightFootVel = (tRight.m_vPosition - m_vRightFootPos) * fScaleToPerSec;

    m_vLeftFootPos = tLeft.m_vPosition;
    m_vRightFootPos = tRight.m_vPosition;
  }

  m_AnimationPose.ConvertFromObjectSpaceToSkinningSpace(skeleton);

  ezArrayPtr<ezMat4> pRenderMatrices = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezMat4, m_AnimationPose.GetTransformCount());
  ezMemoryUtils::Copy(pRenderMatrices.GetPtr(), m_AnimationPose.GetAllTransforms().GetPtr(), m_AnimationPose.GetTransformCount());

  m_SkinningMatrices = pRenderMatrices;
}

void ezMotionMatchingComponent::SetAnimation(ezUInt32 uiIndex, const ezAnimationClipResourceHandle& hResource)
{
  m_Animations.EnsureCount(uiIndex + 1);

  m_Animations[uiIndex] = hResource;
}

ezAnimationClipResourceHandle ezMotionMatchingComponent::GetAnimation(ezUInt32 uiIndex) const
{
  if (uiIndex >= m_Animations.GetCount())
    return ezAnimationClipResourceHandle();

  return m_Animations[uiIndex];
}

ezUInt32 ezMotionMatchingComponent::Animations_GetCount() const
{
  return m_Animations.GetCount();
}

const char* ezMotionMatchingComponent::Animations_GetValue(ezUInt32 uiIndex) const
{
  const auto& hMat = GetAnimation(uiIndex);

  if (!hMat.IsValid())
    return "";

  return hMat.GetResourceID();
}

void ezMotionMatchingComponent::Animations_SetValue(ezUInt32 uiIndex, const char* value)
{
  if (ezStringUtils::IsNullOrEmpty(value))
    SetAnimation(uiIndex, ezAnimationClipResourceHandle());
  else
  {
    auto hMat = ezResourceManager::LoadResource<ezAnimationClipResource>(value);
    SetAnimation(uiIndex, hMat);
  }
}

void ezMotionMatchingComponent::Animations_Insert(ezUInt32 uiIndex, const char* value)
{
  ezAnimationClipResourceHandle hMat;

  if (!ezStringUtils::IsNullOrEmpty(value))
    hMat = ezResourceManager::LoadResource<ezAnimationClipResource>(value);

  m_Animations.Insert(hMat, uiIndex);
}

void ezMotionMatchingComponent::Animations_Remove(ezUInt32 uiIndex)
{
  m_Animations.RemoveAtAndCopy(uiIndex);
}

ezMotionMatchingComponent::TargetKeyframe ezMotionMatchingComponent::FindNextKeyframe(const TargetKeyframe& current,
                                                                                      const ezVec3& vTargetDir) const
{
  TargetKeyframe kf;
  kf.m_uiAnimClip = current.m_uiAnimClip;
  kf.m_uiKeyframe = current.m_uiKeyframe + 1;

  {
    // ezResourceLock<ezAnimationClipResource> pAnimClipCur(m_Animations[current.m_uiAnimClip], ezResourceAcquireMode::NoFallback);
    // const auto& animClip = pAnimClipCur->GetDescriptor();

    // const ezUInt32 uiLeftFootJoint = animClip.FindJointIndexByName("Bip01_L_Foot");
    // const ezUInt32 uiRightFootJoint = animClip.FindJointIndexByName("Bip01_R_Foot");

    const ezVec3 vLeftFootPos = m_vLeftFootPos;   // animClip.GetJointKeyframes(uiLeftFootJoint)[current.m_uiKeyframe].m_vPosition;
    const ezVec3 vRightFootPos = m_vRightFootPos; // animClip.GetJointKeyframes(uiRightFootJoint)[current.m_uiKeyframe].m_vPosition;

    const ezUInt32 uiBestMM = FindBestKeyframe(current, vLeftFootPos, vRightFootPos, vTargetDir);

    TargetKeyframe nkf;
    nkf.m_uiAnimClip = m_MotionData[uiBestMM].m_uiAnimClipIndex;
    nkf.m_uiKeyframe = m_MotionData[uiBestMM].m_uiKeyframeIndex;

    if ((nkf.m_uiAnimClip != kf.m_uiAnimClip) || (nkf.m_uiKeyframe != kf.m_uiKeyframe && nkf.m_uiKeyframe != current.m_uiKeyframe))
    {
      kf = nkf;
    }
  }

  ezResourceLock<ezAnimationClipResource> pAnimClip(m_Animations[kf.m_uiAnimClip], ezResourceAcquireMode::NoFallback);

  if (kf.m_uiKeyframe >= pAnimClip->GetDescriptor().GetNumFrames())
  {
    // loop
    kf.m_uiKeyframe = 0;
  }

  return kf;
}

void ezMotionMatchingComponent::PrecomputeMotion(ezDynamicArray<MotionData>& motionData, ezTempHashedString jointName1,
                                                 ezTempHashedString jointName2, const ezAnimationClipResourceDescriptor& animClip,
                                                 ezUInt16 uiAnimClipIndex, const ezSkeleton& skeleton)
{
  const ezUInt16 uiRootJoint = animClip.HasRootMotion() ? animClip.GetRootMotionJoint() : 0xFFFFFFFFu;
  // const ezUInt16 uiJoint1IndexInAnim = animClip.FindJointIndexByName(jointName1);
  // const ezUInt16 uiJoint2IndexInAnim = animClip.FindJointIndexByName(jointName2);

  const ezUInt16 uiJoint1IndexInSkeleton = skeleton.FindJointByName(jointName1);
  const ezUInt16 uiJoint2IndexInSkeleton = skeleton.FindJointByName(jointName2);
  if (uiJoint1IndexInSkeleton == ezInvalidJointIndex || uiJoint2IndexInSkeleton == ezInvalidJointIndex)
    return;

  const auto& jointNamesToIndices = animClip.GetAllJointIndices();

  const ezUInt32 uiFirstMotionDataIdx = motionData.GetCount();
  motionData.Reserve(uiFirstMotionDataIdx + animClip.GetNumFrames());

  const float fRootMotionToVelocity = animClip.GetFramesPerSecond();

  ezAnimationPose pose;
  pose.Configure(skeleton);

  for (ezUInt16 uiFrameIdx = 0; uiFrameIdx < animClip.GetNumFrames(); ++uiFrameIdx)
  {
    pose.SetToBindPoseInLocalSpace(skeleton);

    for (ezUInt32 b = 0; b < jointNamesToIndices.GetCount(); ++b)
    {
      const ezUInt16 uiJointIndexInPose = skeleton.FindJointByName(jointNamesToIndices.GetKey(b));
      if (uiJointIndexInPose != ezInvalidJointIndex)
      {
        const ezTransform jointTransform = animClip.GetJointKeyframes(jointNamesToIndices.GetValue(b))[uiFrameIdx];

        pose.SetTransform(uiJointIndexInPose, jointTransform.GetAsMat4());
      }
    }

    pose.ConvertFromLocalSpaceToObjectSpace(skeleton);

    MotionData& md = motionData.ExpandAndGetRef();
    md.m_vLeftFootPosition = pose.GetTransform(uiJoint1IndexInSkeleton).GetTranslationVector();
    md.m_vRightFootPosition = pose.GetTransform(uiJoint2IndexInSkeleton).GetTranslationVector();
    md.m_uiAnimClipIndex = uiAnimClipIndex;
    md.m_uiKeyframeIndex = uiFrameIdx;
    md.m_vLeftFootVelocity.SetZero();
    md.m_vRightFootVelocity.SetZero();
    md.m_vRootVelocity = animClip.HasRootMotion() ? fRootMotionToVelocity * animClip.GetJointKeyframes(uiRootJoint)[uiFrameIdx].m_vPosition
                                                  : ezVec3::ZeroVector();
  }

  // now compute the velocity
  {
    const float fScaleToVelPerSec = animClip.GetFramesPerSecond();

    ezUInt32 uiPrevMdIdx = motionData.GetCount() - 1;

    for (ezUInt32 uiMotionDataIdx = uiFirstMotionDataIdx; uiMotionDataIdx < motionData.GetCount(); ++uiMotionDataIdx)
    {
      {
        ezVec3 vel = motionData[uiMotionDataIdx].m_vLeftFootPosition - motionData[uiPrevMdIdx].m_vLeftFootPosition;
        motionData[uiMotionDataIdx].m_vLeftFootVelocity = vel * fScaleToVelPerSec;
      }
      {
        ezVec3 vel = motionData[uiMotionDataIdx].m_vRightFootPosition - motionData[uiPrevMdIdx].m_vRightFootPosition;
        motionData[uiMotionDataIdx].m_vRightFootVelocity = vel * fScaleToVelPerSec;
      }

      uiPrevMdIdx = uiMotionDataIdx;
    }
  }
}

ezUInt32 ezMotionMatchingComponent::FindBestKeyframe(const TargetKeyframe& current, ezVec3 vLeftFootPosition, ezVec3 vRightFootPosition,
                                                     ezVec3 vTargetDir) const
{
  float fClosest = 1000000000.0f;
  ezUInt32 uiClosest = 0xFFFFFFFFu;

  const float fDirWeight = 3.0f;

  for (ezUInt32 i = 0; i < m_MotionData.GetCount(); ++i)
  {
    const auto& md = m_MotionData[i];

    float penaltyMul = 1.1f;
    float penaltyAdd = 100;

    if (md.m_uiAnimClipIndex == current.m_uiAnimClip)
    {
      // do NOT allow to transition backwards to a keyframe within a certain range
      if (md.m_uiKeyframeIndex < current.m_uiKeyframe && md.m_uiKeyframeIndex + 10 > current.m_uiKeyframe)
        continue;

      penaltyMul = 1.0f;

      if (md.m_uiKeyframeIndex == current.m_uiKeyframe)
      {
        penaltyAdd = 0;
        penaltyMul = 0.9f;
      }
    }

    const float dirDist = ezMath::Pow((md.m_vRootVelocity - vTargetDir).GetLength(), fDirWeight);
    const float leftFootDist = (md.m_vLeftFootPosition - vLeftFootPosition).GetLengthSquared();
    const float rightFootDist = (md.m_vRightFootPosition - vRightFootPosition).GetLengthSquared();

    const float fScore = dirDist + (leftFootDist + rightFootDist) * penaltyMul + penaltyAdd;

    if (fScore < fClosest)
    {
      fClosest = fScore;
      uiClosest = i;
    }
  }

  return uiClosest;
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_MotionMatchingComponent);

