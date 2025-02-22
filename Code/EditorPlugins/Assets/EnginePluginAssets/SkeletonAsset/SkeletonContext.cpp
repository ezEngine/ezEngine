#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/SkeletonAsset/SkeletonContext.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonView.h>

#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <RendererCore/AnimationSystem/SkeletonComponent.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkeletonContext, 1, ezRTTIDefaultAllocator<ezSkeletonContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Skeleton"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSkeletonContext::ezSkeletonContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
}

void ezSkeletonContext::HandleMessage(const ezEditorEngineDocumentMsg* pDocMsg)
{
  if (auto pMsg = ezDynamicCast<const ezQuerySelectionBBoxMsgToEngine*>(pDocMsg))
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (auto pMsg = ezDynamicCast<const ezSimpleDocumentConfigMsgToEngine*>(pDocMsg))
  {
    if (pMsg->m_sWhatToDo == "CommonAssetUiState")
    {
      if (pMsg->m_sPayload == "Grid")
      {
        m_bDisplayGrid = pMsg->m_PayloadValue.ConvertTo<float>() > 0;
        return;
      }
    }
    else if (pMsg->m_sWhatToDo == "HighlightBones")
    {
      EZ_LOCK(m_pWorld->GetWriteMarker());

      ezSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->SetBonesToHighlight(pMsg->m_sPayload);
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderBones")
    {
      EZ_LOCK(m_pWorld->GetWriteMarker());

      ezSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeBones = pMsg->m_PayloadValue.Get<bool>();
      }

      // resend the pose every frame (this config message is send every frame)
      // this ensures that changing any of the visualization states in the skeleton component displays correctly
      // a bit hacky and should be cleaned up, but this way the skeleton component doesn't need to keep a copy of the last pose (maybe it should)
      ezSkeletonPoseComponent* pPoseSkeleton;
      if (m_pWorld->TryGetComponent(m_hPoseComponent, pPoseSkeleton))
      {
        pPoseSkeleton->ResendPose();
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderColliders")
    {
      EZ_LOCK(m_pWorld->GetWriteMarker());

      ezSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeColliders = pMsg->m_PayloadValue.Get<bool>();
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderJoints")
    {
      EZ_LOCK(m_pWorld->GetWriteMarker());

      ezSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeJoints = pMsg->m_PayloadValue.Get<bool>();
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderSwingLimits")
    {
      EZ_LOCK(m_pWorld->GetWriteMarker());

      ezSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeSwingLimits = pMsg->m_PayloadValue.Get<bool>();
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderTwistLimits")
    {
      EZ_LOCK(m_pWorld->GetWriteMarker());

      ezSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeTwistLimits = pMsg->m_PayloadValue.Get<bool>();
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
        m_hAnimMeshComponent.Invalidate();
        pAnimMesh->DeleteComponent();
      }

      if (!m_sAnimatedMeshToUse.IsEmpty())
      {
        ezMaterialResourceHandle hMat = ezResourceManager::LoadResource<ezMaterialResource>("Editor/Materials/SkeletonPreviewMesh.ezMaterial");

        m_hAnimMeshComponent = ezAnimatedMeshComponent::CreateComponent(m_pGameObject, pAnimMesh);
        pAnimMesh->SetMeshFile(m_sAnimatedMeshToUse);

        for (int i = 0; i < 10; ++i)
        {
          pAnimMesh->SetMaterial(i, hMat);
        }
      }
    }
  }

  ezEngineProcessDocumentContext::HandleMessage(pDocMsg);
}

void ezSkeletonContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetWriteMarker());

  ezGameObjectDesc obj;
  ezSkeletonComponent* pVisSkeleton;
  ezSkeletonPoseComponent* pPoseSkeleton;

  // Preview Mesh
  {
    obj.m_sName.Assign("SkeletonPreview");
    obj.m_bDynamic = true;
    pWorld->CreateObject(obj, m_pGameObject);

    m_hSkeletonComponent = ezSkeletonComponent::CreateComponent(m_pGameObject, pVisSkeleton);
    ezStringBuilder sSkeletonGuid;
    ezConversionUtils::ToString(GetDocumentGuid(), sSkeletonGuid);
    m_hSkeleton = ezResourceManager::LoadResource<ezSkeletonResource>(sSkeletonGuid);
    pVisSkeleton->SetSkeleton(m_hSkeleton);
    pVisSkeleton->m_bVisualizeColliders = true;

    m_hPoseComponent = ezSkeletonPoseComponent::CreateComponent(m_pGameObject, pPoseSkeleton);
    pPoseSkeleton->SetSkeleton(m_hSkeleton);
    pPoseSkeleton->SetPoseMode(ezSkeletonPoseMode::RestPose);
  }
}

ezEngineProcessViewContext* ezSkeletonContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezSkeletonViewContext, this);
}

void ezSkeletonContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

bool ezSkeletonContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  ezBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  ezSkeletonViewContext* pMeshViewContext = static_cast<ezSkeletonViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void ezSkeletonContext::QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg)
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
