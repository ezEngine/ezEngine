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
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Texture 3D"),
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

    // The editor sends all three values before every redraw, not only when they change.
    // The material parameters are only marked as modified when the value actually differs.
    if (pMsg2->m_sWhatToDo == "PreviewMode")
    {
      if (m_iPreviewMode != pMsg2->m_iValue)
      {
        m_iPreviewMode = pMsg2->m_iValue;
        ApplyPreviewMode();
      }
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

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewRedrawMsgToEngine>() && m_bTextureFormatDirty)
  {
    ApplyTextureFormat();
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

  // The preview materials are created resources, so they have no data loader and can never be reloaded
  // once their data was unloaded (which happens when the document is closed and the material loses its
  // last reference). Reusing such a name would yield a material without base material or shader.
  // A process wide counter keeps the materials of every document context distinct.
  static ezAtomicInteger32 s_iNextMaterialGeneration = 0;
  const ezUInt32 uiMaterialGeneration = (ezUInt32)s_iNextMaterialGeneration.Increment();

  // Slice Material
  {
    ezStringBuilder sMaterialResource;
    sMaterialResource.SetFormat("{}-{} - Texture3D Slice Preview", sTextureGuid, uiMaterialGeneration);

    ezMaterialResourceDescriptor md;
    md.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Editor/Materials/Texture3DPreviewSlice.ezMaterial");

    m_hSliceMaterial = ezResourceManager::GetOrCreateResource<ezMaterialResource>(sMaterialResource, std::move(md));
  }

  // Raymarch Material
  {
    ezStringBuilder sMaterialResource;
    sMaterialResource.SetFormat("{}-{} - Texture3D Raymarch Preview", sTextureGuid, uiMaterialGeneration);

    ezMaterialResourceDescriptor md;
    md.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Editor/Materials/Texture3DPreviewRaymarch.ezMaterial");

    m_hRaymarchMaterial = ezResourceManager::GetOrCreateResource<ezMaterialResource>(sMaterialResource, std::move(md));
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

  {
    ezResourceLock<ezTexture3DResource> pTexture(m_hTexture, ezResourceAcquireMode::PointerOnly);
    pTexture->m_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezTexture3DContext::OnResourceEvent, this), m_TextureResourceEventSubscriber);
  }

  // The binding stays valid across reloads, the material picks up the new texture data automatically.
  // Only the format dependent parameters have to be updated, once the texture data is available.
  {
    ezResourceLock<ezMaterialResource> pMaterial(m_hSliceMaterial, ezResourceAcquireMode::BlockTillLoaded);
    pMaterial->SetTexture3DBinding("BaseTexture", m_hTexture);
  }

  {
    ezResourceLock<ezMaterialResource> pMaterial(m_hRaymarchMaterial, ezResourceAcquireMode::BlockTillLoaded);
    pMaterial->SetTexture3DBinding("BaseTexture", m_hTexture);
  }

  m_bTextureFormatDirty = true;
}

void ezTexture3DContext::ApplyTextureFormat()
{
  // Clear the flag before reading the format, so that a reload finishing in between isn't lost.
  m_bTextureFormatDirty = false;

  ezGALResourceFormat::Enum format = ezGALResourceFormat::Invalid;
  {
    ezResourceLock<ezTexture3DResource> pTexture(m_hTexture, ezResourceAcquireMode::PointerOnly);
    format = pTexture->GetFormat();
  }

  // Not loaded yet, try again on the next redraw.
  if (format == ezGALResourceFormat::Invalid)
  {
    m_bTextureFormatDirty = true;
    return;
  }

  // The preview shaders output to an sRGB target, so data that isn't sRGB encoded gets converted, to display the raw values.
  const bool bIsLinear = !ezGALResourceFormat::IsSrgb(format);

  ezResourceLock<ezMaterialResource> pSliceMaterial(m_hSliceMaterial, ezResourceAcquireMode::BlockTillLoaded);
  pSliceMaterial->SetParameter("IsLinear", bIsLinear);

  ezResourceLock<ezMaterialResource> pRaymarchMaterial(m_hRaymarchMaterial, ezResourceAcquireMode::BlockTillLoaded);
  pRaymarchMaterial->SetParameter("IsLinear", bIsLinear);
}

void ezTexture3DContext::OnResourceEvent(const ezResourceEvent& e)
{
  // This can be called from a loading thread, so only flag the change and apply it on the next redraw.
  // A reload after a transform may change the format (e.g. when the usage was switched between Color and Linear).
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUpdated)
  {
    m_bTextureFormatDirty = true;
  }
}
