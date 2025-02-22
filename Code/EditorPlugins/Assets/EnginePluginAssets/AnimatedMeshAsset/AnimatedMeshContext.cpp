#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshContext.h>
#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshView.h>

#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimatedMeshContext, 1, ezRTTIDefaultAllocator<ezAnimatedMeshContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Animated Mesh"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimatedMeshContext::ezAnimatedMeshContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pAnimatedMeshObject = nullptr;
}

void ezAnimatedMeshContext::HandleMessage(const ezEditorEngineDocumentMsg* pDocMsg)
{
  if (auto* pMsg = ezDynamicCast<const ezEditorEngineSetMaterialsMsg*>(pDocMsg))
  {
    ezAnimatedMeshComponent* pAnimatedMesh;
    if (m_pAnimatedMeshObject && m_pAnimatedMeshObject->TryGetComponentOfBaseType(pAnimatedMesh))
    {
      for (ezUInt32 i = 0; i < pMsg->m_Materials.GetCount(); ++i)
      {
        ezMaterialResourceHandle hMat;

        if (!pMsg->m_Materials[i].IsEmpty())
        {
          hMat = ezResourceManager::LoadResource<ezMaterialResource>(pMsg->m_Materials[i]);
        }

        pAnimatedMesh->SetMaterial(i, hMat);
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

void ezAnimatedMeshContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetWriteMarker());

  ezAnimatedMeshComponent* pAnimatedMesh;

  // Preview AnimatedMesh
  {
    ezGameObjectDesc obj;
    obj.m_bDynamic = true;
    obj.m_sName.Assign("AnimatedMeshPreview");
    pWorld->CreateObject(obj, m_pAnimatedMeshObject);

    const ezTag& tagCastShadows = ezTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pAnimatedMeshObject->SetTag(tagCastShadows);

    ezAnimatedMeshComponent::CreateComponent(m_pAnimatedMeshObject, pAnimatedMesh);
    ezStringBuilder sAnimatedMeshGuid;
    ezConversionUtils::ToString(GetDocumentGuid(), sAnimatedMeshGuid);
    m_hAnimatedMesh = ezResourceManager::LoadResource<ezMeshResource>(sAnimatedMeshGuid);
    pAnimatedMesh->SetMesh(m_hAnimatedMesh);
  }
}

ezEngineProcessViewContext* ezAnimatedMeshContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezAnimatedMeshViewContext, this);
}

void ezAnimatedMeshContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

bool ezAnimatedMeshContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  ezBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  ezAnimatedMeshViewContext* pAnimatedMeshViewContext = static_cast<ezAnimatedMeshViewContext*>(pThumbnailViewContext);
  return pAnimatedMeshViewContext->UpdateThumbnailCamera(bounds);
}


void ezAnimatedMeshContext::QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg)
{
  if (m_pAnimatedMeshObject == nullptr)
    return;

  ezBoundingBoxSphere bounds = ezBoundingBoxSphere::MakeInvalid();

  {
    EZ_LOCK(m_pWorld->GetWriteMarker());

    m_pAnimatedMeshObject->UpdateLocalBounds();
    m_pAnimatedMeshObject->UpdateGlobalTransformAndBounds();
    const auto& b = m_pAnimatedMeshObject->GetGlobalBounds();

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
