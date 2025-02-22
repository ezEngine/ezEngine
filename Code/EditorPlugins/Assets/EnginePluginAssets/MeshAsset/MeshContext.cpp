#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <EnginePluginAssets/MeshAsset/MeshView.h>

#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshContext, 1, ezRTTIDefaultAllocator<ezMeshContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Mesh"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezMeshContext::ezMeshContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pMeshObject = nullptr;
}

void ezMeshContext::HandleMessage(const ezEditorEngineDocumentMsg* pDocMsg)
{
  if (auto* pMsg = ezDynamicCast<const ezEditorEngineSetMaterialsMsg*>(pDocMsg))
  {
    ezMeshComponent* pMesh;
    if (m_pMeshObject && m_pMeshObject->TryGetComponentOfBaseType(pMesh))
    {
      for (ezUInt32 i = 0; i < pMsg->m_Materials.GetCount(); ++i)
      {
        ezMaterialResourceHandle hMat;

        if (!pMsg->m_Materials[i].IsEmpty())
        {
          hMat = ezResourceManager::LoadResource<ezMaterialResource>(pMsg->m_Materials[i]);
        }

        pMesh->SetMaterial(i, hMat);
      }
    }

    return;
  }

  if (auto* pMsg = ezDynamicCast<const ezQuerySelectionBBoxMsgToEngine*>(pDocMsg))
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
  }

  ezEngineProcessDocumentContext::HandleMessage(pDocMsg);
}

void ezMeshContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetWriteMarker());

  ezGameObjectDesc obj;
  ezMeshComponent* pMesh;

  // Preview Mesh
  {
    obj.m_sName.Assign("MeshPreview");
    pWorld->CreateObject(obj, m_pMeshObject);

    const ezTag& tagCastShadows = ezTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pMeshObject->SetTag(tagCastShadows);

    ezMeshComponent::CreateComponent(m_pMeshObject, pMesh);
    ezStringBuilder sMeshGuid;
    ezConversionUtils::ToString(GetDocumentGuid(), sMeshGuid);
    m_hMesh = ezResourceManager::LoadResource<ezMeshResource>(sMeshGuid);
    pMesh->SetMesh(m_hMesh);

    {
      ezResourceLock<ezMeshResource> pMeshRes(m_hMesh, ezResourceAcquireMode::PointerOnly);
      pMeshRes->m_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezMeshContext::OnResourceEvent, this), m_MeshResourceEventSubscriber);
    }
  }
}

ezEngineProcessViewContext* ezMeshContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezMeshViewContext, this);
}

void ezMeshContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

bool ezMeshContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  if (m_bBoundsDirty)
  {
    EZ_LOCK(m_pWorld->GetWriteMarker());

    m_pMeshObject->UpdateLocalBounds();
    m_pMeshObject->UpdateGlobalTransformAndBounds();
    m_bBoundsDirty = false;
  }
  ezBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  ezMeshViewContext* pMeshViewContext = static_cast<ezMeshViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void ezMeshContext::QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg)
{
  if (m_pMeshObject == nullptr)
    return;

  ezBoundingBoxSphere bounds = ezBoundingBoxSphere::MakeInvalid();

  {
    EZ_LOCK(m_pWorld->GetWriteMarker());

    m_pMeshObject->UpdateLocalBounds();
    m_pMeshObject->UpdateGlobalTransformAndBounds();
    m_bBoundsDirty = false;
    const auto& b = m_pMeshObject->GetGlobalBounds();

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

void ezMeshContext::OnResourceEvent(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUpdated)
  {
    m_bBoundsDirty = true;
  }
}
