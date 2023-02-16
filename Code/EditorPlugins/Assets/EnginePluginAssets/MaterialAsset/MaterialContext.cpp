#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MaterialAsset/MaterialContext.h>
#include <EnginePluginAssets/MaterialAsset/MaterialView.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

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

ezMaterialContext::ezMaterialContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
}

void ezMaterialContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezCreateThumbnailMsgToEngine>())
  {
    ezResourceManager::RestoreResource(m_hMaterial);
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezDocumentConfigMsgToEngine>())
  {
    const ezDocumentConfigMsgToEngine* pMsg2 = static_cast<const ezDocumentConfigMsgToEngine*>(pMsg);

    if (pMsg2->m_sWhatToDo == "InvalidateCache")
    {
      // make sure all scenes etc rebuild their render cache
      ezRenderWorld::DeleteAllCachedRenderData();
    }
    else if (pMsg2->m_sWhatToDo == "PreviewModel" && m_PreviewModel != (PreviewModel)pMsg2->m_iValue)
    {
      m_PreviewModel = (PreviewModel)pMsg2->m_iValue;

      auto pWorld = m_pWorld;
      EZ_LOCK(pWorld->GetWriteMarker());

      ezMeshComponent* pMesh;
      if (pWorld->TryGetComponent(m_hMeshComponent, pMesh))
      {
        switch (m_PreviewModel)
        {
          case PreviewModel::Ball:
            pMesh->SetMesh(m_hBallMesh);
            break;
          case PreviewModel::Sphere:
            pMesh->SetMesh(m_hSphereMesh);
            break;
          case PreviewModel::Box:
            pMesh->SetMesh(m_hBoxMesh);
            break;
          case PreviewModel::Plane:
            pMesh->SetMesh(m_hPlaneMesh);
            break;
        }
      }
    }
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

void ezMaterialContext::OnInitialize()
{
  {
    const char* szSphereMeshName = "SphereMaterialPreviewMesh";
    m_hSphereMesh = ezResourceManager::GetExistingResource<ezMeshResource>(szSphereMeshName);

    if (!m_hSphereMesh.IsValid())
    {
      const char* szMeshBufferName = "SphereMaterialPreviewMeshBuffer";

      ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        // Build geometry
        ezGeometry geom;

        ezGeometry::GeoOptions opt;
        opt.m_Color = ezColor::Red;
        opt.m_Transform.SetRotationMatrixZ(ezAngle::Degree(90));
        geom.AddSphere(0.1f, 64, 64, opt);
        geom.ComputeTangents();

        ezMeshBufferResourceDescriptor desc;
        desc.AddCommonStreams();
        desc.AddStream(ezGALVertexAttributeSemantic::TexCoord1, ezMeshTexCoordPrecision::ToResourceFormat(ezMeshTexCoordPrecision::Default));
        desc.AddStream(ezGALVertexAttributeSemantic::Color0, ezGALResourceFormat::RGBAUByteNormalized);
        desc.AddStream(ezGALVertexAttributeSemantic::Color1, ezGALResourceFormat::RGBAUByteNormalized);
        desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

        hMeshBuffer = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }

      {
        ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer, ezResourceAcquireMode::AllowLoadingFallback);

        ezMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "");
        md.ComputeBounds();

        m_hSphereMesh = ezResourceManager::GetOrCreateResource<ezMeshResource>(szSphereMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }
  }

  {
    const char* szBoxMeshName = "BoxMaterialPreviewMesh";
    m_hBoxMesh = ezResourceManager::GetExistingResource<ezMeshResource>(szBoxMeshName);

    if (!m_hBoxMesh.IsValid())
    {
      const char* szMeshBufferName = "BoxMaterialPreviewMeshBuffer";

      ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        ezGeometry::GeoOptions opt;
        opt.m_Color = ezColor::Red;

        // Build geometry
        ezGeometry geom;

        geom.AddBox(ezVec3(0.12f), true, opt);
        geom.ComputeTangents();

        ezMeshBufferResourceDescriptor desc;
        desc.AddCommonStreams();
        desc.AddStream(ezGALVertexAttributeSemantic::TexCoord1, ezMeshTexCoordPrecision::ToResourceFormat(ezMeshTexCoordPrecision::Default));
        desc.AddStream(ezGALVertexAttributeSemantic::Color0, ezGALResourceFormat::RGBAUByteNormalized);
        desc.AddStream(ezGALVertexAttributeSemantic::Color1, ezGALResourceFormat::RGBAUByteNormalized);
        desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

        hMeshBuffer = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }

      {
        ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer, ezResourceAcquireMode::AllowLoadingFallback);

        ezMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "");
        md.ComputeBounds();

        m_hBoxMesh = ezResourceManager::GetOrCreateResource<ezMeshResource>(szBoxMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }
  }

  {
    const char* szPlaneMeshName = "PlaneMaterialPreviewMesh";
    m_hPlaneMesh = ezResourceManager::GetExistingResource<ezMeshResource>(szPlaneMeshName);

    if (!m_hPlaneMesh.IsValid())
    {
      const char* szMeshBufferName = "PlaneMaterialPreviewMeshBuffer";

      ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        // Build geometry
        ezGeometry geom;

        ezGeometry::GeoOptions opt;
        opt.m_Color = ezColor::Red;
        opt.m_Transform.SetRotationMatrixZ(ezAngle::Degree(-90));
        geom.AddRectXY(ezVec2(0.2f), 64, 64, opt);
        geom.ComputeTangents();

        ezMeshBufferResourceDescriptor desc;
        desc.AddCommonStreams();
        desc.AddStream(ezGALVertexAttributeSemantic::TexCoord1, ezMeshTexCoordPrecision::ToResourceFormat(ezMeshTexCoordPrecision::Default));
        desc.AddStream(ezGALVertexAttributeSemantic::Color0, ezGALResourceFormat::RGBAUByteNormalized);
        desc.AddStream(ezGALVertexAttributeSemantic::Color1, ezGALResourceFormat::RGBAUByteNormalized);
        desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

        hMeshBuffer = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }

      {
        ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer, ezResourceAcquireMode::AllowLoadingFallback);

        ezMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "");
        md.ComputeBounds();

        m_hPlaneMesh = ezResourceManager::GetOrCreateResource<ezMeshResource>(szPlaneMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }
  }

  {
    m_hBallMesh = ezResourceManager::LoadResource<ezMeshResource>("Editor/Meshes/MaterialBall.ezMesh");
  }

  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetWriteMarker());

  ezGameObjectDesc obj;
  ezGameObject* pObj;

  // Preview Mesh
  {
    obj.m_sName.Assign("MaterialPreview");
    pWorld->CreateObject(obj, pObj);

    ezMeshComponent* pMesh;
    m_hMeshComponent = ezMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hBallMesh);
    ezStringBuilder sMaterialGuid;
    ezConversionUtils::ToString(GetDocumentGuid(), sMaterialGuid);
    m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(sMaterialGuid);

    // 20 material overrides should be enough for any mesh.
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
