#include <EnginePluginPhysXPCH.h>

#include <EnginePluginPhysX/CollisionMeshAsset/CollisionMeshContext.h>
#include <EnginePluginPhysX/CollisionMeshAsset/CollisionMeshView.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <GameEngine/Animation/RotorComponent.h>
#include <GameEngine/Animation/SliderComponent.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Gameplay/InputComponent.h>
#include <GameEngine/Gameplay/TimedDeathComponent.h>
#include <GameEngine/Prefabs/SpawnComponent.h>
#include <PhysXPlugin/Components/PxVisColMeshComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollisionMeshContext, 1, ezRTTIDefaultAllocator<ezCollisionMeshContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Collision Mesh Asset;Collision Mesh Asset (Convex)"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCollisionMeshContext::ezCollisionMeshContext()
{
  m_pMeshObject = nullptr;
}

void ezCollisionMeshContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezQuerySelectionBBoxMsgToEngine>())
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

void ezCollisionMeshContext::OnInitialize()
{
  auto pWorld = m_pWorld.Borrow();
  EZ_LOCK(pWorld->GetWriteMarker());

  ezGameObjectDesc obj;
  ezPxVisColMeshComponent* pMesh;

  // Preview Mesh
  {
    obj.m_sName.Assign("MeshPreview");
    pWorld->CreateObject(obj, m_pMeshObject);

    const ezTag& tagCastShadows = ezTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pMeshObject->GetTags().Set(tagCastShadows);

    ezPxVisColMeshComponent::CreateComponent(m_pMeshObject, pMesh);
    ezStringBuilder sMeshGuid;
    ezConversionUtils::ToString(GetDocumentGuid(), sMeshGuid);
    m_hMesh = ezResourceManager::LoadResource<ezPxMeshResource>(sMeshGuid);
    pMesh->SetMesh(m_hMesh);
  }

  // Lights
  {
    obj.m_sName.Assign("DirLight");
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0.0f, 1.0f, 0.0f), ezAngle::Degree(120.0f));

    ezGameObject* pObj;
    pWorld->CreateObject(obj, pObj);

    ezDirectionalLightComponent* pDirLight;
    ezDirectionalLightComponent::CreateComponent(pObj, pDirLight);
    pDirLight->SetCastShadows(true);

    ezAmbientLightComponent* pAmbLight;
    ezAmbientLightComponent::CreateComponent(pObj, pAmbLight);
    pAmbLight->SetIntensity(5.0f);
  }
}

ezEngineProcessViewContext* ezCollisionMeshContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezCollisionMeshViewContext, this);
}

void ezCollisionMeshContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

bool ezCollisionMeshContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  ezBoundingBoxSphere bounds = GetWorldBounds(m_pWorld.Borrow());

  ezCollisionMeshViewContext* pMeshViewContext = static_cast<ezCollisionMeshViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void ezCollisionMeshContext::QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg)
{
  if (m_pMeshObject == nullptr)
    return;

  ezBoundingBoxSphere bounds;
  bounds.SetInvalid();

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
  res.m_vHalfExtents = bounds.m_vBoxHalfExtends;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}
