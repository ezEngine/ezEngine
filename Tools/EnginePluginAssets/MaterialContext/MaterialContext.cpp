#include <PCH.h>
#include <EnginePluginAssets/MaterialContext/MaterialContext.h>
#include <EnginePluginAssets/MaterialView/MaterialView.h>

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
#include <GameUtils/Components/CameraComponent.h>
#include <GameUtils/Components/InputComponent.h>
#include <EditorFramework/Gizmos/GizmoRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <SharedPluginAssets/MaterialAsset/MaterialMessages.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>

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
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineMaterialUpdateMsg>())
  {
    const ezEditorEngineMaterialUpdateMsg* pMsg2 = static_cast<const ezEditorEngineMaterialUpdateMsg*>(pMsg);

    ezUniquePtr<ezResourceLoaderFromMemory> loader(EZ_DEFAULT_NEW(ezResourceLoaderFromMemory));
    loader->m_ModificationTimestamp = ezTimestamp::CurrentTimestamp();
    loader->m_sResourceDescription = "MaterialImmediateEditorUpdate";
    ezMemoryStreamWriter memoryWriter(&loader->m_CustomData);
    memoryWriter.WriteBytes(pMsg2->m_Data.GetData(), pMsg2->m_Data.GetCount());

    ezResourceManager::UpdateResourceWithCustomLoader(m_hMaterial, std::move(loader));
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineRestoreResourceMsg>())
  {
    ezResourceManager::ReloadResource(m_hMaterial, true);
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

      geom.AddSphere(1, 64, 64, ezColor::White);
      geom.ComputeTangents();

      ezMeshBufferResourceDescriptor desc;
      desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);
      desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

      hMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>(szMeshBufferName, desc, szMeshBufferName);
    }
    {
      ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer);

      ezMeshResourceDescriptor md;
      md.UseExistingMeshBuffer(hMeshBuffer);
      md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
      md.SetMaterial(0, "");
      md.CalculateBounds();

      hMesh = ezResourceManager::CreateResource<ezMeshResource>(szMeshName, md, pMeshBuffer->GetResourceDescription());
    }
  }

  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetWriteMarker());

  ezMeshComponentManager* pMeshCompMan = pWorld->GetOrCreateComponentManager<ezMeshComponentManager>();
  ezRotorComponentManager* pRotorCompMan = pWorld->GetOrCreateComponentManager<ezRotorComponentManager>();

  ezGameObjectDesc obj;
  ezGameObject* pObj;
  ezMeshComponent* pMesh;


  // Preview Mesh
  {
    obj.m_sName.Assign("MaterialPreview");
    pWorld->CreateObject(obj, pObj);

    pMeshCompMan->CreateComponent(pMesh);
    pMesh->SetMesh(hMesh);
    ezString sMaterialGuid = ezConversionUtils::ToString(GetDocumentGuid());
    m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(sMaterialGuid);

    // TODO: Once we allow switching the preview mesh, we should be set, 20 material overrides should be enough for everyone.
    for (ezUInt32 i = 0; i < 20; ++i)
    {
      pMesh->SetMaterial(i, m_hMaterial);
    }
    pObj->AttachComponent(pMesh);
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
