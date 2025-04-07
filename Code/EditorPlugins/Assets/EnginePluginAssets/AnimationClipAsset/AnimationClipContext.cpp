#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimationClipAsset/AnimationClipContext.h>
#include <EnginePluginAssets/AnimationClipAsset/AnimationClipView.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GameEngine/Animation/Skeletal/SimpleAnimationComponent.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

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

      ezStringBuilder sAnimClipGuid;
      ezConversionUtils::ToString(GetDocumentGuid(), sAnimClipGuid);

      ezAnimatedMeshComponent* pAnimMesh;
      if (pWorld->TryGetComponent(m_hAnimMeshComponent, pAnimMesh))
      {
        pAnimMesh->DeleteComponent();
        m_hAnimMeshComponent.Invalidate();
      }

      ezSimpleAnimationComponent* pAnimController;
      if (pWorld->TryGetComponent(m_hAnimControllerComponent, pAnimController))
      {
        pAnimController->DeleteComponent();
        m_hAnimControllerComponent.Invalidate();
      }

      if (!m_sAnimatedMeshToUse.IsEmpty())
      {
        m_hAnimMeshComponent = ezAnimatedMeshComponent::CreateComponent(m_pGameObject, pAnimMesh);
        m_hAnimControllerComponent = ezSimpleAnimationComponent::CreateComponent(m_pGameObject, pAnimController);

        pAnimMesh->SetMeshFile(m_sAnimatedMeshToUse);
        pAnimController->SetAnimationClipFile(sAnimClipGuid);
      }
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

    ezSimpleAnimationComponent* pAnimController;
    if (pWorld->TryGetComponent(m_hAnimControllerComponent, pAnimController))
    {
      if (pAnimController->m_hAnimationClip.IsValid())
      {
        ezResourceLock<ezAnimationClipResource> pResource(pAnimController->m_hAnimationClip, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);

        if (pResource.GetAcquireResult() == ezResourceAcquireResult::Final)
        {
          ezSimpleDocumentConfigMsgToEditor msg;
          msg.m_DocumentGuid = pMsg->m_DocumentGuid;
          msg.m_sWhatToDo = "ClipDuration";
          msg.m_PayloadValue = pResource->GetDescriptor().GetDuration();

          SendProcessMessage(&msg);
        }
      }
    }
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
  ezBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  if (!m_hAnimControllerComponent.IsInvalidated())
  {
    EZ_LOCK(m_pWorld->GetWriteMarker());

    ezSimpleAnimationComponent* pAnimController;
    if (m_pWorld->TryGetComponent(m_hAnimControllerComponent, pAnimController))
    {
      pAnimController->SetNormalizedPlaybackPosition(0.5f);
      pAnimController->m_fSpeed = 0.0f;

      m_pWorld->SetWorldSimulationEnabled(true);
      m_pWorld->Update();
      m_pWorld->SetWorldSimulationEnabled(false);
    }
  }

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
  EZ_LOCK(m_pWorld->GetWriteMarker());

  ezSimpleAnimationComponent* pAnimController;
  if (m_pWorld->TryGetComponent(m_hAnimControllerComponent, pAnimController))
  {
    pAnimController->SetNormalizedPlaybackPosition(static_cast<float>(pos));
    pAnimController->m_fSpeed = 0.0f;
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
    ezHybridArray<ezUInt16, 32> parents;

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
