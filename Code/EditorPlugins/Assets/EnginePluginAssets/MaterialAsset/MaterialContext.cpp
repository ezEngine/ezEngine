#include <EnginePluginAssetsPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EnginePluginAssets/MaterialAsset/MaterialContext.h>
#include <EnginePluginAssets/MaterialAsset/MaterialView.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <SharedPluginAssets/Common/Messages.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialContext, 1, ezRTTIDefaultAllocator<ezMaterialContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Material"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezMaterialContext::ezMaterialContext() {}

void ezMaterialContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezCreateThumbnailMsgToEngine>())
  {
    ezResourceManager::RestoreResource(m_hMaterial);
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

void ezMaterialContext::OnInitialize()
{
  const char* szMeshName = "DefaultMaterialPreviewMesh";
  m_hPreviewMeshResource = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshName);

  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "DefaultMaterialPreviewMeshBuffer";

    ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      // Build geometry
      ezGeometry geom;

      geom.AddSphere(0.1f, 64, 64, ezColor::Red);
      geom.ComputeTangents();

      ezMeshBufferResourceDescriptor desc;
      desc.AddCommonStreams();
      desc.AddStream(ezGALVertexAttributeSemantic::TexCoord1, ezMeshTexCoordPrecision::ToResourceFormat(ezMeshTexCoordPrecision::Default));
      desc.AddStream(ezGALVertexAttributeSemantic::Color0, ezGALResourceFormat::RGBAUByteNormalized);
      desc.AddStream(ezGALVertexAttributeSemantic::Color1, ezGALResourceFormat::RGBAUByteNormalized);
      desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

      hMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
    }

    {
      ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer, ezResourceAcquireMode::AllowLoadingFallback);

      ezMeshResourceDescriptor md;
      md.UseExistingMeshBuffer(hMeshBuffer);
      md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
      md.SetMaterial(0, "");
      md.ComputeBounds();

      m_hPreviewMeshResource = ezResourceManager::CreateResource<ezMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
    }
  }

  auto pWorld = m_pWorld.Borrow();
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
    pMesh->SetMesh(m_hPreviewMeshResource);
    ezStringBuilder sMaterialGuid;
    ezConversionUtils::ToString(GetDocumentGuid(), sMaterialGuid);
    m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(sMaterialGuid);

    // TODO: Once we allow switching the preview mesh, we should be set, 20 material overrides should be enough for everyone.
    for (ezUInt32 i = 0; i < 20; ++i)
    {
      pMesh->SetMaterial(i, m_hMaterial);
    }
  }
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
