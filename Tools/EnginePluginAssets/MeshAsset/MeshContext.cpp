#include <PCH.h>
#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <EnginePluginAssets/MeshAsset/MeshView.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <GameUtils/Components/RotorComponent.h>
#include <GameUtils/Components/SliderComponent.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <CoreUtils/Geometry/GeomUtils.h>
#include <GameFoundation/GameApplication/GameApplication.h>
#include <GameUtils/Components/SpawnComponent.h>
#include <GameUtils/Components/TimedDeathComponent.h>
#include <GameUtils/Components/InputComponent.h>
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
}

void ezMeshContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

void ezMeshContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetWriteMarker());

  ezMeshComponentManager* pMeshCompMan = pWorld->GetOrCreateComponentManager<ezMeshComponentManager>();

  ezGameObjectDesc obj;
  ezGameObject* pObj;
  ezMeshComponent* pMesh;

  // Preview Mesh
  {
    obj.m_sName.Assign("MeshPreview");
    pWorld->CreateObject(obj, pObj);

    pMeshCompMan->CreateComponent(pMesh);
    ezString sMeshGuid = ezConversionUtils::ToString(GetDocumentGuid());
    ezMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezMeshResource>(sMeshGuid);
    pMesh->SetMesh(hMesh);

    pObj->AttachComponent(pMesh);
  }

  // Lights
  {
    obj.m_sName.Assign("DirLight");
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0.0f, 1.0f, 0.0f), ezAngle::Degree(60.0f));

    pWorld->CreateObject(obj, pObj);

    ezDirectionalLightComponent* pDirLight;
    ezDirectionalLightComponent::CreateComponent(pWorld, pDirLight);
    pObj->AttachComponent(pDirLight);

    ezAmbientLightComponent* pAmbLight;
    ezAmbientLightComponent::CreateComponent(GetWorld(), pAmbLight);
    pObj->AttachComponent(pAmbLight);
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
