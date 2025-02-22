#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshContext.h>
#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshView.h>

#include <JoltPlugin/Components/JoltVisColMeshComponent.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJoltCollisionMeshContext, 1, ezRTTIDefaultAllocator<ezJoltCollisionMeshContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Jolt_Colmesh_Triangle;Jolt_Colmesh_Convex"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltCollisionMeshContext::ezJoltCollisionMeshContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pMeshObject = nullptr;
}

void ezJoltCollisionMeshContext::HandleMessage(const ezEditorEngineDocumentMsg* pDocMsg)
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
  }

  ezEngineProcessDocumentContext::HandleMessage(pDocMsg);
}

void ezJoltCollisionMeshContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetWriteMarker());

  ezGameObjectDesc obj;
  ezJoltVisColMeshComponent* pMesh = nullptr;

  // Preview Mesh
  {
    obj.m_sName.Assign("MeshPreview");
    pWorld->CreateObject(obj, m_pMeshObject);

    const ezTag& tagCastShadows = ezTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pMeshObject->SetTag(tagCastShadows);

    ezJoltVisColMeshComponent::CreateComponent(m_pMeshObject, pMesh);
    ezStringBuilder sMeshGuid;
    ezConversionUtils::ToString(GetDocumentGuid(), sMeshGuid);
    m_hMesh = ezResourceManager::LoadResource<ezJoltMeshResource>(sMeshGuid);
    pMesh->SetMesh(m_hMesh);
  }
}

ezEngineProcessViewContext* ezJoltCollisionMeshContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezJoltCollisionMeshViewContext, this);
}

void ezJoltCollisionMeshContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

bool ezJoltCollisionMeshContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  ezBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  ezJoltCollisionMeshViewContext* pMeshViewContext = static_cast<ezJoltCollisionMeshViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void ezJoltCollisionMeshContext::QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg)
{
  if (m_pMeshObject == nullptr)
    return;

  ezBoundingBoxSphere bounds = ezBoundingBoxSphere::MakeInvalid();

  {
    EZ_LOCK(m_pWorld->GetWriteMarker());

    m_pMeshObject->UpdateLocalBounds();
    m_pMeshObject->UpdateGlobalTransformAndBounds();
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
