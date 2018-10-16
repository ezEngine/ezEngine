#include <PCH.h>

#include <EnginePluginKraut/KrautTreeAsset/KrautTreeContext.h>
#include <EnginePluginKraut/KrautTreeAsset/KrautTreeView.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <GameEngine/Components/InputComponent.h>
#include <GameEngine/Components/RotorComponent.h>
#include <GameEngine/Components/SliderComponent.h>
#include <GameEngine/Components/SpawnComponent.h>
#include <GameEngine/Components/TimedDeathComponent.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <SharedPluginAssets/Common/Messages.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeContext, 1, ezRTTIDefaultAllocator<ezKrautTreeContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Kraut Tree Asset"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezKrautTreeContext::ezKrautTreeContext()
{
  m_pMainObject = nullptr;
}

void ezKrautTreeContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezQuerySelectionBBoxMsgToEngine>())
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

void ezKrautTreeContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetWriteMarker());

  ezGameObjectDesc obj;
  ezKrautTreeComponent* pTree;

  // Preview Mesh
  {
    obj.m_sName.Assign("KrautTreePreview");
    pWorld->CreateObject(obj, m_pMainObject);

    const ezTag& tagCastShadows = ezTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pMainObject->GetTags().Set(tagCastShadows);

     ezKrautTreeComponent::CreateComponent(m_pMainObject, pTree);
     ezStringBuilder sMeshGuid;
     ezConversionUtils::ToString(GetDocumentGuid(), sMeshGuid);
     m_hMainResource = ezResourceManager::LoadResource<ezKrautTreeResource>(sMeshGuid);
     pTree->SetKrautTree(m_hMainResource);
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

ezEngineProcessViewContext* ezKrautTreeContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezKrautTreeViewContext, this);
}

void ezKrautTreeContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

bool ezKrautTreeContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  ezBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  ezKrautTreeViewContext* pMeshViewContext = static_cast<ezKrautTreeViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void ezKrautTreeContext::QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg)
{
  if (m_pMainObject == nullptr)
    return;

  ezBoundingBoxSphere bounds;
  bounds.SetInvalid();

  {
    EZ_LOCK(m_pWorld->GetReadMarker());

    m_pMainObject->UpdateLocalBounds();
    m_pMainObject->UpdateGlobalTransformAndBounds();
    const auto& b = m_pMainObject->GetGlobalBounds();

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
