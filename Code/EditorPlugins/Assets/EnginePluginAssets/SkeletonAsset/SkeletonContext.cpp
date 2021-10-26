#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/SkeletonAsset/SkeletonContext.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonView.h>

#include <RendererCore/AnimationSystem/SkeletonComponent.h>

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
        m_bDisplayGrid = pMsg->m_fPayload > 0;
        return;
      }
    }
    else if (pMsg->m_sWhatToDo == "HighlightBones")
    {
      EZ_LOCK(m_pWorld->GetWriteMarker());

      ezSkeletonComponent* pSkeleton;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->SetBonesToHighlight(pMsg->m_sPayload);
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

  // Preview Mesh
  {
    obj.m_sName.Assign("SkeletonPreview");
    pWorld->CreateObject(obj, m_pGameObject);

    m_hSkeletonComponent = ezSkeletonComponent::CreateComponent(m_pGameObject, pVisSkeleton);
    ezStringBuilder sSkeletonGuid;
    ezConversionUtils::ToString(GetDocumentGuid(), sSkeletonGuid);
    m_hSkeleton = ezResourceManager::LoadResource<ezSkeletonResource>(sSkeletonGuid);
    pVisSkeleton->SetSkeleton(m_hSkeleton);
    pVisSkeleton->m_bVisualizeSkeleton = true;
    pVisSkeleton->m_bVisualizeColliders = true;
    pVisSkeleton->m_bVisualizeJoints = true;
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

  ezBoundingBoxSphere bounds;
  bounds.SetInvalid();

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
  res.m_vHalfExtents = bounds.m_vBoxHalfExtends;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}
