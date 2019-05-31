#include <EnginePluginKrautPCH.h>

#include <EnginePluginKraut/KrautTreeAsset/KrautTreeContext.h>
#include <EnginePluginKraut/KrautTreeAsset/KrautTreeView.h>

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
#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/RenderContext/RenderContext.h>

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
  auto pWorld = m_pWorld.Borrow();
  EZ_LOCK(pWorld->GetWriteMarker());


  ezKrautTreeComponent* pTree;

  // Preview Mesh
  {
    ezGameObjectDesc obj;
    obj.m_sName.Assign("KrautTreePreview");
    // TODO: making the object dynamic is a workaround!
    // without it, shadows keep disappearing when switching between tree documents
    // triggering resource reload will also trigger ezKrautTreeComponent::OnExtractRenderData,
    // which fixes the shadows for a while, but not caching the render-data (ezRenderData::Caching::IfStatic)
    // 'solves' the issue in the preview
    obj.m_bDynamic = true;
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
    ezGameObjectDesc obj;
    obj.m_sName.Assign("DirLight");
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0.0f, 1.0f, 0.0f), ezAngle::Degree(120.0f));
    obj.m_LocalPosition = obj.m_LocalRotation * ezVec3(-5, 0, 0);

    ezGameObject* pObj;
    pWorld->CreateObject(obj, pObj);

    ezDirectionalLightComponent* pDirLight;
    ezDirectionalLightComponent::CreateComponent(pObj, pDirLight);
    pDirLight->SetCastShadows(true);

    ezAmbientLightComponent* pAmbLight;
    ezAmbientLightComponent::CreateComponent(pObj, pAmbLight);
    pAmbLight->SetIntensity(5.0f);
  }

  // Sky
  {
    ezGameObjectDesc obj;
    obj.m_sName.Assign("Sky");

    ezGameObject* pObj;
    pWorld->CreateObject(obj, pObj);

    ezSkyBoxComponent* pSkybox = nullptr;
    ezSkyBoxComponent::CreateComponent(pObj, pSkybox);

    pSkybox->SetExposureBias(1.0f);
    pSkybox->SetCubeMapFile("{ 0b202e08-a64f-465d-b38e-15b81d161822 }");
  }

  // ground
  {
    const char* szMeshName = "KrautPreviewGroundMesh";
    ezMeshResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshName);

    if (!hMesh.IsValid())
    {
      const char* szMeshBufferName = "KrautPreviewGroundMeshBuffer";

      ezMeshBufferResourceHandle hMeshBuffer;
      {
        // Build geometry
        ezMat4 t;
        t.SetTranslationMatrix(ezVec3(0, 0, -0.05f));

        ezGeometry geom;
        geom.AddCylinder(8.0f, 7.9f, 0.05f, 0.05f, true, true, 32, ezColor::White, t);
        geom.TriangulatePolygons();
        geom.ComputeTangents();

        ezMeshBufferResourceDescriptor desc;
        desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
        desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat);
        desc.AddStream(ezGALVertexAttributeSemantic::TexCoord1, ezGALResourceFormat::XYFloat);
        desc.AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
        desc.AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);
        desc.AddStream(ezGALVertexAttributeSemantic::Color, ezGALResourceFormat::RGBAUByteNormalized);
        desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

        hMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }
      {
        ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer, ezResourceAcquireMode::AllowFallback);

        ezMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Pattern.ezMaterialAsset
        md.ComputeBounds();

        hMesh = ezResourceManager::CreateResource<ezMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }

    // Ground Mesh Component
    {
      ezGameObjectDesc obj;
      obj.m_sName.Assign("KrautGround");

      ezGameObject* pObj;
      pWorld->CreateObject(obj, pObj);

      ezMeshComponent* pMesh;
      ezMeshComponent::CreateComponent(pObj, pMesh);
      pMesh->SetMesh(hMesh);
    }
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
  m_pMainObject->UpdateLocalBounds();
  ezBoundingBoxSphere bounds = m_pMainObject->GetGlobalBounds();

  // undo the artificial bounds scale to get a tight bbox for better thumbnails
  const float fAdditionalZoom = 1.5f;
  bounds.m_fSphereRadius /= ezKrautTreeComponent::s_iLocalBoundsScale * fAdditionalZoom;
  bounds.m_vBoxHalfExtends /= ezKrautTreeComponent::s_iLocalBoundsScale * fAdditionalZoom;

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
    EZ_LOCK(m_pWorld->GetWriteMarker());

    m_pMainObject->UpdateLocalBounds();
    m_pMainObject->UpdateGlobalTransformAndBounds();
    auto b = m_pMainObject->GetGlobalBounds();

    if (b.IsValid())
    {
      b.m_fSphereRadius /= ezKrautTreeComponent::s_iLocalBoundsScale;
      b.m_vBoxHalfExtends /= (float)ezKrautTreeComponent::s_iLocalBoundsScale;

      bounds.ExpandToInclude(b);
    }
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
