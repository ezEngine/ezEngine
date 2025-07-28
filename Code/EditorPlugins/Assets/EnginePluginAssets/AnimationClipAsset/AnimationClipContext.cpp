#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimationClipAsset/AnimationClipContext.h>
#include <EnginePluginAssets/AnimationClipAsset/AnimationClipView.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Meshes/MeshResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipContext, 1, ezRTTIDefaultAllocator<ezAnimationClipContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Animation Clip"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimationClipContext::ezAnimationClipContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
}

void ezAnimationClipContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg0)
{
  if (auto pMsg = ezDynamicCast<const ezQuerySelectionBBoxMsgToEngine*>(pMsg0))
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (auto pMsg = ezDynamicCast<const ezSimpleDocumentConfigMsgToEngine*>(pMsg0))
  {
    if (pMsg->m_sWhatToDo == "CommonAssetUiState")
    {
      if (pMsg->m_sPayload == "Grid")
      {
        m_bDisplayGrid = pMsg->m_PayloadValue.ConvertTo<float>() > 0;
      }
    }
    else if (pMsg->m_sWhatToDo == "PreviewMesh" && m_sAnimatedMeshToUse != pMsg->m_sPayload)
    {
      m_sAnimatedMeshToUse = pMsg->m_sPayload;

      auto pWorld = m_pWorld;
      EZ_LOCK(pWorld->GetWriteMarker());

      ezAnimatedMeshComponent* pAnimMesh;
      if (pWorld->TryGetComponent(m_hAnimMeshComponent, pAnimMesh))
      {
        pAnimMesh->DeleteComponent();
        m_hAnimMeshComponent.Invalidate();
      }

      if (!m_sAnimatedMeshToUse.IsEmpty())
      {
        m_hAnimMeshComponent = ezAnimatedMeshComponent::CreateComponent(m_pGameObject, pAnimMesh);
        pAnimMesh->SetMeshFile(m_sAnimatedMeshToUse);
      }
    }
    else if (pMsg->m_sWhatToDo == "PreviewAnim" && m_sBaseAnimationClip != pMsg->m_sPayload)
    {
      m_sBaseAnimationClip = pMsg->m_sPayload;
    }
    else if (pMsg->m_sWhatToDo == "PlaybackPos")
    {
      SetPlaybackPosition(pMsg->m_PayloadValue.Get<double>());
    }
    else if (pMsg->m_sWhatToDo == "ExtractRootMotionFromFeet")
    {
      ExtractRootMotionFromFeet();
    }

    return;
  }

  if (auto pMsg = ezDynamicCast<const ezViewRedrawMsgToEngine*>(pMsg0))
  {
    auto pWorld = m_pWorld;
    EZ_LOCK(pWorld->GetWriteMarker());

    if (!m_sAnimatedMeshToUse.IsEmpty())
    {
      ezStringBuilder sAnimClipGuid;
      ezConversionUtils::ToString(GetDocumentGuid(), sAnimClipGuid);
      ezAnimationClipResourceHandle hAnimation = ezResourceManager::LoadResource<ezAnimationClipResource>(sAnimClipGuid);

      ezResourceLock<ezAnimationClipResource> pAnimation(hAnimation, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);
      if (pAnimation.GetAcquireResult() == ezResourceAcquireResult::Final)
      {
        ezSimpleDocumentConfigMsgToEditor msg;
        msg.m_DocumentGuid = pMsg->m_DocumentGuid;
        msg.m_sWhatToDo = "ClipDuration";
        msg.m_PayloadValue = pAnimation->GetDescriptor().GetDuration();
        SendProcessMessage(&msg);
      }
    }

    GenerateAndApplyPose();
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg0);
}

void ezAnimationClipContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetWriteMarker());

  ezGameObjectDesc obj;

  // Preview
  {
    obj.m_bDynamic = true;
    obj.m_sName.Assign("SkeletonPreview");
    pWorld->CreateObject(obj, m_pGameObject);
  }
}

ezEngineProcessViewContext* ezAnimationClipContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezAnimationClipViewContext, this);
}

void ezAnimationClipContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

bool ezAnimationClipContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  {
    EZ_LOCK(m_pWorld->GetWriteMarker());

    m_fNormalizedPlaybackPosition = 0.5f;
    GenerateAndApplyPose();

    m_pWorld->SetWorldSimulationEnabled(true);
    m_pWorld->Update();
    m_pWorld->SetWorldSimulationEnabled(false);
  }

  ezBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  ezAnimationClipViewContext* pMeshViewContext = static_cast<ezAnimationClipViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void ezAnimationClipContext::QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg)
{
  if (m_pGameObject == nullptr)
    return;

  ezBoundingBoxSphere bounds = ezBoundingBoxSphere::MakeInvalid();

  {
    EZ_LOCK(m_pWorld->GetWriteMarker());

    m_pGameObject->UpdateLocalBounds();
    m_pGameObject->UpdateGlobalTransformAndBounds();
    const auto& b = m_pGameObject->GetGlobalBounds();

    if (b.IsValid())
      bounds.ExpandToInclude(b);
  }

  const ezQuerySelectionBBoxMsgToEngine* msg = static_cast<const ezQuerySelectionBBoxMsgToEngine*>(pMsg);

  ezQuerySelectionBBoxResultMsgToEditor res;
  res.m_uiViewID = msg->m_uiViewID;
  res.m_iPurpose = msg->m_iPurpose;
  res.m_vCenter = bounds.m_vCenter;
  res.m_vHalfExtents = bounds.m_vBoxHalfExtents;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}

void ezAnimationClipContext::SetPlaybackPosition(double pos)
{
  m_fNormalizedPlaybackPosition = static_cast<float>(pos);
}

void ezAnimationClipContext::GenerateAndApplyPose()
{
  if (m_sAnimatedMeshToUse.IsEmpty() || m_pGameObject == nullptr)
    return;

  ezMeshResourceHandle hAnimMesh = ezResourceManager::LoadResource<ezMeshResource>(m_sAnimatedMeshToUse);
  ezResourceLock<ezMeshResource> pAnimMesh(hAnimMesh, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);
  if (pAnimMesh.GetAcquireResult() != ezResourceAcquireResult::Final || !pAnimMesh->m_hDefaultSkeleton.IsValid())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(pAnimMesh->m_hDefaultSkeleton, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  ezStringBuilder sAnimClipGuid;
  ezConversionUtils::ToString(GetDocumentGuid(), sAnimClipGuid);
  ezAnimationClipResourceHandle hAnimation = ezResourceManager::LoadResource<ezAnimationClipResource>(sAnimClipGuid);

  ezResourceLock<ezAnimationClipResource> pAnimation(hAnimation, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);
  if (pAnimation.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  ezAnimPoseGenerator poseGen;
  poseGen.Reset(pSkeleton.GetPointer(), m_pGameObject);

  bool bGraphSetup = false;

  if (!m_sBaseAnimationClip.IsEmpty())
  {
    // Additive mode: blend base animation with the additive clip on top.
    // The CombinePoses command automatically separates additive and non-additive
    // layers based on the m_bAdditive flag in each clip's descriptor.
    ezAnimationClipResourceHandle hBaseAnim = ezResourceManager::LoadResource<ezAnimationClipResource>(m_sBaseAnimationClip);
    ezResourceLock<ezAnimationClipResource> pBaseAnim(hBaseAnim, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);

    if (pBaseAnim.GetAcquireResult() == ezResourceAcquireResult::Final)
    {
      auto& cmdBase = poseGen.AllocCommandSampleTrack(0);
      cmdBase.m_hAnimationClip = hBaseAnim;
      cmdBase.m_fNormalizedSamplePos = 0.0f;
      cmdBase.m_fPreviousNormalizedSamplePos = 0.0f;
      cmdBase.m_EventSampling = ezAnimPoseEventTrackSampleMode::None;

      auto& cmdAdditive = poseGen.AllocCommandSampleTrack(1);
      cmdAdditive.m_hAnimationClip = hAnimation;
      cmdAdditive.m_fNormalizedSamplePos = m_fNormalizedPlaybackPosition;
      cmdAdditive.m_fPreviousNormalizedSamplePos = m_fNormalizedPlaybackPosition;
      cmdAdditive.m_EventSampling = ezAnimPoseEventTrackSampleMode::None;

      auto& cmdCombine = poseGen.AllocCommandCombinePoses();
      cmdCombine.m_Inputs.PushBack(cmdBase.GetCommandID());
      cmdCombine.m_InputWeights.PushBack(1.0f);
      cmdCombine.m_Inputs.PushBack(cmdAdditive.GetCommandID());
      cmdCombine.m_InputWeights.PushBack(1.0f);

      auto& cmdL2M = poseGen.AllocCommandLocalToModelPose();
      cmdL2M.m_pSendLocalPoseMsgTo = m_pGameObject;
      cmdL2M.m_Inputs.PushBack(cmdCombine.GetCommandID());
      poseGen.SetFinalCommand(cmdL2M.GetCommandID());

      bGraphSetup = true;
    }
  }

  if (!bGraphSetup)
  {
    // Non-additive mode (or base clip not yet loaded): sample the clip directly.
    auto& cmdSample = poseGen.AllocCommandSampleTrack(0);
    cmdSample.m_hAnimationClip = hAnimation;
    cmdSample.m_fNormalizedSamplePos = m_fNormalizedPlaybackPosition;
    cmdSample.m_fPreviousNormalizedSamplePos = m_fNormalizedPlaybackPosition;
    cmdSample.m_EventSampling = ezAnimPoseEventTrackSampleMode::None;

    auto& cmdL2M = poseGen.AllocCommandLocalToModelPose();
    cmdL2M.m_pSendLocalPoseMsgTo = m_pGameObject;
    cmdL2M.m_Inputs.PushBack(cmdSample.GetCommandID());
    poseGen.SetFinalCommand(cmdL2M.GetCommandID());
  }

  poseGen.UpdatePose(false);

  if (poseGen.ShouldSendPoseResultMsg())
  {
    ezMsgAnimationPoseUpdated poseMsg;
    poseMsg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
    poseMsg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
    poseMsg.m_ModelTransforms = poseGen.GetCurrentPose();
    m_pGameObject->SendMessageRecursive(poseMsg);
  }
}

void ezAnimationClipContext::ExtractRootMotionFromFeet()
{
  auto ReturnFailure = [this](ezStringView str)
  {
    ezSimpleDocumentConfigMsgToEditor msg;
    msg.m_sWhatToDo = "ReportError";
    msg.m_sPayload = str;
    SendProcessMessage(&msg);
  };

  if (m_sAnimatedMeshToUse.IsEmpty())
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nPreview mesh is not set.");
    return;
  }

  ezStringBuilder sAnimClipGuid;
  ezConversionUtils::ToString(GetDocumentGuid(), sAnimClipGuid);
  ezAnimationClipResourceHandle hAnimation = ezResourceManager::LoadResource<ezAnimationClipResource>(sAnimClipGuid);

  ezResourceLock<ezAnimationClipResource> pAnimation(hAnimation, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimation.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nCouldn't load animation.");
    return;
  }

  ezMeshResourceHandle hAnimMesh = ezResourceManager::LoadResource<ezMeshResource>(m_sAnimatedMeshToUse);
  ezResourceLock<ezMeshResource> pAnimMesh(hAnimMesh, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pAnimMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nCouldn't load preview mesh.");
    return;
  }

  if (!pAnimMesh->m_hDefaultSkeleton.IsValid())
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nPreview mesh has no skeleton.");
    return;
  }

  ezResourceLock<ezSkeletonResource> pSkeleton(pAnimMesh->m_hDefaultSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nSkeleton of preview mesh could not be loaded.");
    return;
  }

  ezAnimPoseGenerator pg;

  const auto& skel = pSkeleton->GetDescriptor().m_Skeleton;

  const ezUInt16 uiFoot1 = pSkeleton->GetDescriptor().m_uiLeftFootJoint;
  const ezUInt16 uiFoot2 = pSkeleton->GetDescriptor().m_uiRightFootJoint;

  if (uiFoot1 == ezInvalidJointIndex)
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nLeft foot joint is not correctly defined in skeleton asset.");
    return;
  }

  if (uiFoot2 == ezInvalidJointIndex)
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nRight foot joint is not correctly defined in skeleton asset.");
    return;
  }

  if (uiFoot1 == uiFoot2)
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nLeft and right foot joint must differ.");
    return;
  }

  ezUInt16 uiSharedParentJoint = ezInvalidJointIndex;

  // find shared parent bone
  {
    ezTempHybridArray<ezUInt16, 32> parents;

    auto* pJoint = &skel.GetJointByIndex(uiFoot1);

    // collect all parent joint indices
    while (pJoint->GetParentIndex() != ezInvalidJointIndex)
    {
      parents.PushBack(pJoint->GetParentIndex());
      pJoint = &skel.GetJointByIndex(pJoint->GetParentIndex());
    }

    pJoint = &skel.GetJointByIndex(uiFoot2);

    // collect all parent joint indices
    while (pJoint->GetParentIndex() != ezInvalidJointIndex)
    {
      if (parents.Contains(pJoint->GetParentIndex()))
      {
        uiSharedParentJoint = pJoint->GetParentIndex();
        break;
      }

      pJoint = &skel.GetJointByIndex(pJoint->GetParentIndex());
    }
  }

  if (uiSharedParentJoint == ezInvalidJointIndex)
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nCouldn't find shared parent bone of feet bones.");
    return;
  }

  // TODO: don't hard-code num samples ?
  const ezUInt32 uiNumSamples = 32;
  float fPrevPos = 0.0f;

  int iFootDown = -1;
  int iFootUp = -1;
  ezVec3 vLastHipDist(0);

  ezVec3 vMovement(0);
  int iSamplesTaken = 0;


  for (ezUInt32 uiSample = 0; uiSample < uiNumSamples; ++uiSample)
  {
    pg.Reset(pSkeleton.GetPointer(), nullptr);

    auto& cmd = pg.AllocCommandSampleTrack(0);
    cmd.m_EventSampling = ezAnimPoseEventTrackSampleMode::None;
    cmd.m_fPreviousNormalizedSamplePos = fPrevPos;
    cmd.m_fNormalizedSamplePos = (float)uiSample / (float)(uiNumSamples - 1);
    cmd.m_hAnimationClip = hAnimation;
    fPrevPos = cmd.m_fNormalizedSamplePos;

    auto& cmdMP = pg.AllocCommandLocalToModelPose();
    cmdMP.m_Inputs.PushBack(cmd.GetCommandID());

    pg.SetFinalCommand(cmdMP.GetCommandID());

    pg.UpdatePose(false);

    const ezVec3 p[3] =
      {
        pg.GetCurrentPose()[uiSharedParentJoint].GetTranslationVector(),
        pg.GetCurrentPose()[uiFoot1].GetTranslationVector(),
        pg.GetCurrentPose()[uiFoot2].GetTranslationVector()
        //
      };

    if (iFootDown == -1)
    {
      if (p[1].y < p[2].y)
      {
        iFootDown = 1;
        iFootUp = 2;
      }
      else
      {
        iFootDown = 2;
        iFootUp = 1;
      }

      vLastHipDist = p[0] - p[iFootDown];
    }
    else if (p[iFootDown].y > p[iFootUp].y)
    {
      ezMath::Swap(iFootDown, iFootUp);

      vLastHipDist = p[0] - p[iFootDown];
    }
    else
    {
      const ezVec3 vHipDist = p[0] - p[iFootDown];

      vMovement += vHipDist - vLastHipDist;
      iSamplesTaken++;
      vLastHipDist = vHipDist;
    }
  }

  if (iSamplesTaken == 0)
  {
    ReturnFailure("Failed to extract root motion from feet.\n\nNo valid animation samples found.");
    return;
  }

  ezVec3 avg = vMovement / (float)iSamplesTaken;
  avg *= (uiNumSamples - 1);                                           // calculate the average movement over the entire clip
  avg /= pAnimation->GetDescriptor().GetDuration().AsFloatInSeconds(); // scale it to the movement per second

  // transform the motion into the desired space
  avg = pSkeleton->GetDescriptor().m_RootTransform.GetAsMat4().TransformDirection(avg);

  const float len = avg.GetLengthAndNormalize();

  ezVec4 res;
  res.Set(avg.x, avg.y, avg.z, len);

  {
    ezSimpleDocumentConfigMsgToEditor msg;
    msg.m_sWhatToDo = "ExtractRootMotionFromFeet";
    msg.m_PayloadValue = res;

    SendProcessMessage(&msg);
  }
}
