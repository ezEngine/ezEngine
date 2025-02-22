#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimationClipAsset/AnimationClipContext.h>
#include <EnginePluginAssets/AnimationClipAsset/AnimationClipView.h>

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
