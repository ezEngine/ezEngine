#include <PCH.h>
#include <EnginePluginAssets/MaterialAsset/MaterialContext.h>
#include <EnginePluginAssets/MaterialAsset/MaterialView.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <GameEngine/Components/RotorComponent.h>
#include <GameEngine/Components/SliderComponent.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Core/Graphics/Geometry.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Components/SpawnComponent.h>
#include <GameEngine/Components/TimedDeathComponent.h>
#include <GameEngine/Components/InputComponent.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <SharedPluginAssets/Common/Messages.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <SharedPluginAssets/Common/Messages.h>
#include <RendererCore/Lights/AmbientLightComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialContext, 1, ezRTTIDefaultAllocator<ezMaterialContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Material Asset"),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE


ezMaterialContext::ezMaterialContext()
{
}

void ezMaterialContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineResourceUpdateMsg>())
  {
    const ezEditorEngineResourceUpdateMsg* pMsg2 = static_cast<const ezEditorEngineResourceUpdateMsg*>(pMsg);

    if (pMsg2->m_sResourceType == "Material")
    {
      ezUniquePtr<ezResourceLoaderFromMemory> loader(EZ_DEFAULT_NEW(ezResourceLoaderFromMemory));
      loader->m_ModificationTimestamp = ezTimestamp::CurrentTimestamp();
      loader->m_sResourceDescription = "MaterialImmediateEditorUpdate";
      ezMemoryStreamWriter memoryWriter(&loader->m_CustomData);
      memoryWriter.WriteBytes(pMsg2->m_Data.GetData(), pMsg2->m_Data.GetCount());

      ezResourceManager::UpdateResourceWithCustomLoader(m_hMaterial, std::move(loader));

      // force loading of the resource
      ezResourceLock<ezMaterialResource> pResource(m_hMaterial, ezResourceAcquireMode::NoFallback);
    }
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineRestoreResourceMsg>())
  {
    ezResourceManager::RestoreResource(m_hMaterial);
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

void ezMaterialContext::OnInitialize()
{

  const char* szMeshName = "DefaultMaterialPreviewMesh";
  ezMeshResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshName);

  if (!hMesh.IsValid())
  {
    const char* szMeshBufferName = "DefaultMaterialPreviewMeshBuffer";

    ezMeshBufferResourceHandle hMeshBuffer;
    {
      // Build geometry
      ezGeometry geom;

      geom.AddSphere(1, 64, 64, ezColor::Red);
      geom.ComputeTangents();

      ezMeshBufferResourceDescriptor desc;
      desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::Color, ezGALResourceFormat::RGBAUByteNormalized);
      desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

      hMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>(szMeshBufferName, desc, szMeshBufferName);
    }
    {
      ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer);

      ezMeshResourceDescriptor md;
      md.UseExistingMeshBuffer(hMeshBuffer);
      md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
      md.SetMaterial(0, "");
      md.ComputeBounds();

      hMesh = ezResourceManager::CreateResource<ezMeshResource>(szMeshName, md, pMeshBuffer->GetResourceDescription());
    }
  }

  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetWriteMarker());

  ezGameObjectDesc obj;
  ezGameObject* pObj;

  // Preview Mesh
  {
    obj.m_sName.Assign("MaterialPreview");
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));
    pWorld->CreateObject(obj, pObj);

    ezMeshComponent* pMesh;
    ezMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(hMesh);
    ezStringBuilder sMaterialGuid;
    ezConversionUtils::ToString(GetDocumentGuid(), sMaterialGuid);
    m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(sMaterialGuid);

    // TODO: Once we allow switching the preview mesh, we should be set, 20 material overrides should be enough for everyone.
    for (ezUInt32 i = 0; i < 20; ++i)
    {
      pMesh->SetMaterial(i, m_hMaterial);
    }
  }

  // Lights
  {
    obj.m_sName.Assign("DirLight");
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0.0f, 1.0f, 0.0f), ezAngle::Degree(60.0f));

    pWorld->CreateObject(obj, pObj);

    ezDirectionalLightComponent* pDirLight;
    ezDirectionalLightComponent::CreateComponent(pObj, pDirLight);

    ezAmbientLightComponent* pAmbLight;
    ezAmbientLightComponent::CreateComponent(pObj, pAmbLight);
  }
}

void ezMaterialContext::OnDeinitialize()
{
}

ezEngineProcessViewContext* ezMaterialContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezMaterialViewContext, this);
}

void ezMaterialContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

bool ezMaterialContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  ezMaterialViewContext* pMaterialViewContext = static_cast<ezMaterialViewContext*>(pThumbnailViewContext);
  pMaterialViewContext->PositionThumbnailCamera();
  return true;
}
