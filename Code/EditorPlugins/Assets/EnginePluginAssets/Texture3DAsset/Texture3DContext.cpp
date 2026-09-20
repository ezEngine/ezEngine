#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/Texture3DAsset/Texture3DContext.h>
#include <EnginePluginAssets/Texture3DAsset/Texture3DView.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTexture3DContext, 1, ezRTTIDefaultAllocator<ezTexture3DContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Texture 3D;LUT"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static void CreatePreviewRect(ezGeometry& ref_geom)
{
  const ezMat4 mTransform = ezMat4::MakeIdentity();
  const ezVec2 size(1.0f);
  const ezColor color = ezColor::White;

  const ezVec2 halfSize = size * 0.5f;

  ezUInt32 idx[4];

  idx[0] = ref_geom.AddVertex(mTransform, ezVec3(-halfSize.x, 0, -halfSize.y), ezVec3(-1, 0, 0), ezVec2(0, 1), color);
  idx[1] = ref_geom.AddVertex(mTransform, ezVec3(halfSize.x, 0, -halfSize.y), ezVec3(-1, 0, 0), ezVec2(1, 1), color);
  idx[2] = ref_geom.AddVertex(mTransform, ezVec3(halfSize.x, 0, halfSize.y), ezVec3(-1, 0, 0), ezVec2(1, 0), color);
  idx[3] = ref_geom.AddVertex(mTransform, ezVec3(-halfSize.x, 0, halfSize.y), ezVec3(-1, 0, 0), ezVec2(0, 0), color);

  ref_geom.AddPolygon(idx, false);
}

ezTexture3DContext::ezTexture3DContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
}

void ezTexture3DContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezDocumentConfigMsgToEngine>())
  {
    const ezDocumentConfigMsgToEngine* pMsg2 = static_cast<const ezDocumentConfigMsgToEngine*>(pMsg);

    if (pMsg2->m_sWhatToDo == "PreviewMode")
    {
      m_iPreviewMode = pMsg2->m_iValue;
      ApplyPreviewMode();
    }
    else if (pMsg2->m_sWhatToDo == "SliceCoordinate" && m_hSliceMaterial.IsValid())
    {
      ezResourceLock<ezMaterialResource> pMaterial(m_hSliceMaterial, ezResourceAcquireMode::AllowLoadingFallback);
      pMaterial->SetParameter("SliceCoordinate", pMsg2->m_fValue);
    }
    else if (pMsg2->m_sWhatToDo == "OpacityMultiplier" && m_hRaymarchMaterial.IsValid())
    {
      ezResourceLock<ezMaterialResource> pMaterial(m_hRaymarchMaterial, ezResourceAcquireMode::AllowLoadingFallback);
      pMaterial->SetParameter("OpacityMultiplier", pMsg2->m_fValue);
    }
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

void ezTexture3DContext::OnInitialize()
{
  ezStringBuilder sTextureGuid;
  ezConversionUtils::ToString(GetDocumentGuid(), sTextureGuid);

  // Slice Preview Mesh (unit quad)
  {
    const char* szMeshName = "DefaultTexture3DSlicePreviewMesh";
    m_hSlicePreviewMeshResource = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshName);

    if (!m_hSlicePreviewMeshResource.IsValid())
    {
      const char* szMeshBufferName = "DefaultTexture3DSlicePreviewMeshBuffer";

      ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        ezGeometry geom;
        CreatePreviewRect(geom);
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
        md.SetMaterial(0, "");
        md.ComputeBounds();

        m_hSlicePreviewMeshResource = ezResourceManager::GetOrCreateResource<ezMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }
  }

  // Raymarch Preview Mesh (unit box)
  {
    const char* szMeshName = "DefaultTexture3DRaymarchPreviewMesh";
    m_hRaymarchPreviewMeshResource = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshName);

    if (!m_hRaymarchPreviewMeshResource.IsValid())
    {
      const char* szMeshBufferName = "DefaultTexture3DRaymarchPreviewMeshBuffer";

      ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        ezGeometry geom;
        geom.AddBox(ezVec3(1.0f), false);
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
        md.SetMaterial(0, "");
        md.ComputeBounds();

        m_hRaymarchPreviewMeshResource = ezResourceManager::GetOrCreateResource<ezMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }
  }

  // Slice Material
  {
    const ezStringBuilder sMaterialResource(sTextureGuid.GetData(), " - Texture3D Slice Preview");
    m_hSliceMaterial = ezResourceManager::GetExistingResource<ezMaterialResource>(sMaterialResource);

    if (!m_hSliceMaterial.IsValid())
    {
      ezMaterialResourceDescriptor md;
      md.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Editor/Materials/Texture3DPreviewSlice.ezMaterial");

      m_hSliceMaterial = ezResourceManager::GetOrCreateResource<ezMaterialResource>(sMaterialResource, std::move(md));
    }
  }

  // Raymarch Material
  {
    const ezStringBuilder sMaterialResource(sTextureGuid.GetData(), " - Texture3D Raymarch Preview");
    m_hRaymarchMaterial = ezResourceManager::GetExistingResource<ezMaterialResource>(sMaterialResource);

    if (!m_hRaymarchMaterial.IsValid())
    {
      ezMaterialResourceDescriptor md;
      md.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Editor/Materials/Texture3DPreviewRaymarch.ezMaterial");

      m_hRaymarchMaterial = ezResourceManager::GetOrCreateResource<ezMaterialResource>(sMaterialResource, std::move(md));
    }
  }

  // Preview Objects
  {
    EZ_LOCK(m_pWorld->GetWriteMarker());

    {
      ezGameObjectDesc obj;
      ezGameObject* pObj;

      obj.m_sName.Assign("Texture3DSlicePreview");
      obj.m_LocalRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(90));
      m_hSlicePreviewObject = m_pWorld->CreateObject(obj, pObj);

      ezMeshComponent* pMesh;
      m_hSlicePreviewMesh = ezMeshComponent::CreateComponent(pObj, pMesh);
      pMesh->SetMesh(m_hSlicePreviewMeshResource);
      pMesh->SetMaterial(0, m_hSliceMaterial);
    }

    {
      ezGameObjectDesc obj;
      ezGameObject* pObj;

      obj.m_sName.Assign("Texture3DRaymarchPreview");
      m_hRaymarchPreviewObject = m_pWorld->CreateObject(obj, pObj);

      ezMeshComponent* pMesh;
      m_hRaymarchPreviewMesh = ezMeshComponent::CreateComponent(pObj, pMesh);
      pMesh->SetMesh(m_hRaymarchPreviewMeshResource);
      pMesh->SetMaterial(0, m_hRaymarchMaterial);
    }
  }

  ApplyPreviewMode();

  SetTexture(sTextureGuid);
}

ezEngineProcessViewContext* ezTexture3DContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezTexture3DViewContext, this);
}

void ezTexture3DContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

void ezTexture3DContext::ApplyPreviewMode()
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  ezMeshComponent* pSliceMesh = nullptr;
  if (m_pWorld->TryGetComponent(m_hSlicePreviewMesh, pSliceMesh))
  {
    pSliceMesh->SetActiveFlag(m_iPreviewMode == 0); // Slices
  }

  ezMeshComponent* pRaymarchMesh = nullptr;
  if (m_pWorld->TryGetComponent(m_hRaymarchPreviewMesh, pRaymarchMesh))
  {
    pRaymarchMesh->SetActiveFlag(m_iPreviewMode != 0); // RayMarch
  }
}

void ezTexture3DContext::SetTexture(ezStringView sTextureFile)
{
  if (m_hTexture.IsValid() && m_hTexture.GetResourceID() == sTextureFile)
    return;

  m_hTexture = ezResourceManager::LoadResource<ezTexture3DResource>(sTextureFile);
  ezResourceLock<ezTexture3DResource> pTexture(m_hTexture, ezResourceAcquireMode::PointerOnly);
  pTexture->m_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezTexture3DContext::OnResourceEvent, this), m_TextureResourceEventSubscriber);

  {
    ezResourceLock<ezMaterialResource> pMaterial(m_hSliceMaterial, ezResourceAcquireMode::BlockTillLoaded);
    pMaterial->SetTexture3DBinding("BaseTexture", m_hTexture);
    pMaterial->SetParameter("IsLinear", !ezGALResourceFormat::IsSrgb(pTexture->GetFormat()));
  }

  {
    ezResourceLock<ezMaterialResource> pMaterial(m_hRaymarchMaterial, ezResourceAcquireMode::BlockTillLoaded);
    pMaterial->SetTexture3DBinding("BaseTexture", m_hTexture);
    pMaterial->SetParameter("IsLinear", !ezGALResourceFormat::IsSrgb(pTexture->GetFormat()));
  }
}

void ezTexture3DContext::OnResourceEvent(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUpdated)
  {
    const ezTexture3DResource* pTexture = static_cast<const ezTexture3DResource*>(e.m_pResource);
    if (pTexture->GetFormat() != ezGALResourceFormat::Invalid)
    {
      const bool bIsLinear = !ezGALResourceFormat::IsSrgb(pTexture->GetFormat());

      ezResourceLock<ezMaterialResource> pSliceMaterial(m_hSliceMaterial, ezResourceAcquireMode::BlockTillLoaded);
      pSliceMaterial->SetParameter("IsLinear", bIsLinear);

      ezResourceLock<ezMaterialResource> pRaymarchMaterial(m_hRaymarchMaterial, ezResourceAcquireMode::BlockTillLoaded);
      pRaymarchMaterial->SetParameter("IsLinear", bIsLinear);
    }
  }
}
