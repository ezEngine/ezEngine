#include <PCH.h>
#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <EnginePluginAssets/MeshAsset/MeshView.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <GameEngine/Components/RotorComponent.h>
#include <GameEngine/Components/SliderComponent.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Core/Graphics/Geometry.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Components/SpawnComponent.h>
#include <GameEngine/Components/TimedDeathComponent.h>
#include <GameEngine/Components/InputComponent.h>
#include <EditorFramework/Gizmos/GizmoRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <SharedPluginAssets/Common/Messages.h>
#include <RendererCore/Lights/AmbientLightComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshContext, 1, ezRTTIDefaultAllocator<ezMeshContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Mesh Asset"),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE


ezMeshContext::ezMeshContext()
{
  m_pMeshObject = nullptr;
}

void ezMeshContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezQuerySelectionBBoxMsgToEngine>())
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
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

    ezMeshComponent::CreateComponent(m_pMeshObject, pMesh);
    ezStringBuilder sMeshGuid;
    ezConversionUtils::ToString(GetDocumentGuid(), sMeshGuid);
    ezMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezMeshResource>(sMeshGuid);
    pMesh->SetMesh(hMesh);
  }

  // Lights
  {
    obj.m_sName.Assign("DirLight");
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0.0f, 1.0f, 0.0f), ezAngle::Degree(120.0f));

    ezGameObject* pObj;
    pWorld->CreateObject(obj, pObj);

    ezDirectionalLightComponent* pDirLight;
    ezDirectionalLightComponent::CreateComponent(pObj, pDirLight);

    ezAmbientLightComponent* pAmbLight;
    ezAmbientLightComponent::CreateComponent(pObj, pAmbLight);
  }
}

void ezMeshContext::OnDeinitialize()
{
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
  ezBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  ezMeshViewContext* pMeshViewContext = static_cast<ezMeshViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void ezMeshContext::QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg)
{
  if (m_pMeshObject == nullptr)
    return;

  ezBoundingBoxSphere bounds;
  bounds.SetInvalid();

  {
    EZ_LOCK(m_pWorld->GetReadMarker());

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
  res.m_vHalfExtents = bounds.m_vBoxHalfExtends;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}
