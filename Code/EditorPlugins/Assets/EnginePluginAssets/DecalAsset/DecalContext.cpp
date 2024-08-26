#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/DecalAsset/DecalContext.h>
#include <EnginePluginAssets/DecalAsset/DecalView.h>
#include <RendererCore/Decals/DecalComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalContext, 1, ezRTTIDefaultAllocator<ezDecalContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Decal"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezDecalContext::ezDecalContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
}

void ezDecalContext::OnInitialize()
{
  const char* szMeshName = "DefaultDecalPreviewMesh";
  m_hPreviewMeshResource = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshName);

  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "DefaultDecalPreviewMeshBuffer";

    ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      // Build geometry
      ezGeometry geom;
      ezGeometry::GeoOptions opt;

      geom.AddBox(ezVec3(0.5f, 1.0f, 1.0f), true);

      ezMat4 t, r;
      t = ezMat4::MakeTranslation(ezVec3(0, 1.5f, 0));
      r = ezMat4::MakeRotationZ(ezAngle::MakeFromDegree(90));
      opt.m_Transform = t * r;
      geom.AddStackedSphere(0.5f, 64, 64, opt);

      t.SetTranslationVector(ezVec3(0, -1.5f, 0));
      r = ezMat4::MakeRotationY(ezAngle::MakeFromDegree(90));
      opt.m_Transform = t * r;
      geom.AddTorus(0.1f, 0.5f, 32, 64, true, opt);

      geom.ComputeTangents();

      ezMeshBufferResourceDescriptor desc;
      desc.AddCommonStreams();
      desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

      hMeshBuffer = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
    }
    {
      ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer, ezResourceAcquireMode::AllowLoadingFallback);

      ezMeshResourceDescriptor md;
      md.UseExistingMeshBuffer(hMeshBuffer);
      md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
      md.SetMaterial(0, "Materials/Common/TestBricks.ezMaterial");
      md.ComputeBounds();

      m_hPreviewMeshResource = ezResourceManager::GetOrCreateResource<ezMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
    }
  }

  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetWriteMarker());

  ezGameObjectDesc obj;
  ezGameObject* pObj;

  // Preview Mesh that the decals get projected onto
  {
    obj.m_sName.Assign("DecalPreview");
    pWorld->CreateObject(obj, pObj);

    ezMeshComponent* pMesh;
    ezMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hPreviewMeshResource);
  }

  // decals
  {
    ezStringBuilder sDecalGuid;
    ezConversionUtils::ToString(GetDocumentGuid(), sDecalGuid);

    // box
    {
      obj.m_sName.Assign("Decal1");
      obj.m_LocalPosition.Set(-0.25f, 0, 0);
      pWorld->CreateObject(obj, pObj);

      ezDecalComponent* pDecal;
      ezDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }

    // torus
    {
      obj.m_sName.Assign("Decal2");
      obj.m_LocalPosition.Set(-0.2f, -1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      ezDecalComponent* pDecal;
      ezDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }

    // sphere
    {
      obj.m_sName.Assign("Decal3");
      obj.m_LocalPosition.Set(-0.5f, 1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      ezDecalComponent* pDecal;
      ezDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }


    // box
    {
      obj.m_sName.Assign("Decal4");
      obj.m_LocalRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(180));
      obj.m_LocalPosition.Set(0.25f, 0, 0);
      pWorld->CreateObject(obj, pObj);

      ezDecalComponent* pDecal;
      ezDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetExtents(ezVec3(2));
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }

    // torus
    {
      obj.m_sName.Assign("Decal5");
      obj.m_LocalRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(180));
      obj.m_LocalPosition.Set(0.2f, -1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      ezDecalComponent* pDecal;
      ezDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetExtents(ezVec3(2));
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }

    // sphere
    {
      obj.m_sName.Assign("Decal6");
      obj.m_LocalRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(180));
      obj.m_LocalPosition.Set(0.5f, 1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      ezDecalComponent* pDecal;
      ezDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetExtents(ezVec3(2));
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }
  }
}

ezEngineProcessViewContext* ezDecalContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezDecalViewContext, this);
}

void ezDecalContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}
