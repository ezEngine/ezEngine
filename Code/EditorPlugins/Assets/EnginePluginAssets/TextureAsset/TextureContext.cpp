#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureAsset/TextureContext.h>
#include <EnginePluginAssets/TextureAsset/TextureView.h>

#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureContext, 1, ezRTTIDefaultAllocator<ezTextureContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Texture 2D;Render Target;Substance Package"),
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

  idx[0] = ref_geom.AddVertex(mTransform, ezVec3(-halfSize.x, 0, -halfSize.y), ezVec3(-1, 0, 0), ezVec2(-1, 2), color);
  idx[1] = ref_geom.AddVertex(mTransform, ezVec3(halfSize.x, 0, -halfSize.y), ezVec3(-1, 0, 0), ezVec2(2, 2), color);
  idx[2] = ref_geom.AddVertex(mTransform, ezVec3(halfSize.x, 0, halfSize.y), ezVec3(-1, 0, 0), ezVec2(2, -1), color);
  idx[3] = ref_geom.AddVertex(mTransform, ezVec3(-halfSize.x, 0, halfSize.y), ezVec3(-1, 0, 0), ezVec2(-1, -1), color);

  ref_geom.AddPolygon(idx, false);
}

ezTextureContext::ezTextureContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
}

void ezTextureContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezDocumentConfigMsgToEngine>() && !m_SlicePreviews.IsEmpty())
  {
    const ezDocumentConfigMsgToEngine* pMsg2 = static_cast<const ezDocumentConfigMsgToEngine*>(pMsg);

    if (pMsg2->m_sWhatToDo == "SetChannelMode")
    {
      for (auto& slice : m_SlicePreviews)
      {
        ezResourceLock<ezMaterialResource> pMaterial(slice.m_hMaterial, ezResourceAcquireMode::AllowLoadingFallback);
        pMaterial->SetParameter("ShowChannelMode", pMsg2->m_iValue);
        pMaterial->SetParameter("AlphaThreshold", pMsg2->m_fValue);
      }
    }
    else if (pMsg2->m_sWhatToDo == "SetLodLevel")
    {
      if (pMsg2->m_iValue != m_iLodLevel)
      {
        m_iLodLevel = pMsg2->m_iValue;
        for (auto& slice : m_SlicePreviews)
        {
          ezResourceLock<ezMaterialResource> pMaterial(slice.m_hMaterial, ezResourceAcquireMode::AllowLoadingFallback);
          pMaterial->SetParameter("LodLevel", pMsg2->m_iValue);
        }
      }
    }
    else if (pMsg2->m_sWhatToDo == "SetTexture" && pMsg2->m_sValue.IsEmpty() == false)
    {
      SetTexture(pMsg2->m_sValue);
    }
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewRedrawMsgToEngine>())
  {
    // Non-blocking: check current array size and rebuild objects if it changed.
    // Defaults to 1 if the texture is not yet loaded.
    ezUInt32 uiArraySlices = 1;
    if (m_hTexture.IsValid())
    {
      ezResourceLock<ezTexture2DResource> pTex(m_hTexture, ezResourceAcquireMode::PointerOnly);
      if (pTex->GetFormat() != ezGALResourceFormat::Invalid && pTex->GetType() == ezGALTextureType::Texture2DArray)
      {
        const ezGALTexture* pGALTex = ezGALDevice::GetDefaultDevice()->GetTexture(pTex->GetGALTexture());
        if (pGALTex != nullptr)
          uiArraySlices = pGALTex->GetDescription().m_uiArraySize;
      }
    }
    RebuildPreviewObjects(uiArraySlices);
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

void ezTextureContext::OnInitialize()
{
  // Preview Mesh
  const char* szMeshName = "DefaultTexturePreviewMesh";
  m_hPreviewMeshResource = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshName);

  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "DefaultTexturePreviewMeshBuffer";

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

    ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer, ezResourceAcquireMode::AllowLoadingFallback);

    ezMeshResourceDescriptor md;
    md.UseExistingMeshBuffer(hMeshBuffer);
    md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
    md.SetMaterial(0, "");
    md.ComputeBounds();

    m_hPreviewMeshResource = ezResourceManager::GetOrCreateResource<ezMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
  }

  ezStringBuilder sTextureGuid;
  ezConversionUtils::ToString(GetDocumentGuid(), sTextureGuid);
  SetTexture(sTextureGuid);
}

ezEngineProcessViewContext* ezTextureContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezTextureViewContext, this);
}

void ezTextureContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

void ezTextureContext::RebuildPreviewObjects(ezUInt32 uiNumArraySlices)
{
  if (uiNumArraySlices == 0)
    uiNumArraySlices = 1;

  if (uiNumArraySlices == m_uiNumArraySlices)
    return;

  m_uiNumArraySlices = uiNumArraySlices;

  ezStringBuilder sTextureGuid;
  ezConversionUtils::ToString(GetDocumentGuid(), sTextureGuid);

  EZ_LOCK(m_pWorld->GetWriteMarker());

  for (auto& slice : m_SlicePreviews)
    m_pWorld->DeleteObjectDelayed(slice.m_hObject);
  m_SlicePreviews.Clear();

  // Each subsequent slice is placed slightly behind (positive X) and shifted in YZ
  // so its edges peek out from behind the previous slice (deck-of-cards effect).
  // The preview quad lies in the YZ plane after the 90 degree Z rotation on the game object.
  const float fOffsetYZ = 0.25f;
  const float fOffsetX = 0.2f;

  for (ezUInt32 i = 0; i < uiNumArraySlices; ++i)
  {
    // Per-slice material
    ezStringBuilder sMaterialName;
    sMaterialName.SetFormat("{} - Texture Preview Slice {}", sTextureGuid, i);

    ezMaterialResourceHandle hMaterial = ezResourceManager::GetExistingResource<ezMaterialResource>(sMaterialName);
    if (!hMaterial.IsValid())
    {
      ezMaterialResourceDescriptor md;
      md.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Editor/Materials/TexturePreview.ezMaterial");
      hMaterial = ezResourceManager::GetOrCreateResource<ezMaterialResource>(sMaterialName, std::move(md));
    }

    {
      ezResourceLock<ezMaterialResource> pMaterial(hMaterial, ezResourceAcquireMode::AllowLoadingFallback);
      pMaterial->SetParameter("ArrayIndex", (int)i);
      pMaterial->SetParameter("LodLevel", m_iLodLevel);

      if (m_hTexture.IsValid())
        pMaterial->SetTexture2DBinding("BaseTexture", m_hTexture);
    }

    // Game object at stacked position
    ezGameObjectDesc obj;
    ezGameObject* pObj = nullptr;
    obj.m_sName.Assign("TexturePreview");
    obj.m_LocalRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(90));
    obj.m_LocalPosition = (float)i * ezVec3(fOffsetX, fOffsetYZ, -fOffsetYZ);
    ezGameObjectHandle hObject = m_pWorld->CreateObject(obj, pObj);

    ezMeshComponent* pMesh = nullptr;
    ezComponentHandle hMeshComp = ezMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hPreviewMeshResource);
    pMesh->SetMaterial(0, hMaterial);

    auto& slice = m_SlicePreviews.ExpandAndGetRef();
    slice.m_hObject = hObject;
    slice.m_hMeshComponent = hMeshComp;
    slice.m_hMaterial = hMaterial;
  }
}

void ezTextureContext::SetTexture(ezStringView sTextureFile)
{
  if (m_hTexture.IsValid() && m_hTexture.GetResourceID() == sTextureFile)
    return;

  m_hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(sTextureFile);

  {
    ezResourceLock<ezTexture2DResource> pTexture(m_hTexture, ezResourceAcquireMode::PointerOnly);
    pTexture->m_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezTextureContext::OnResourceEvent, this), m_TextureResourceEventSubscriber);
  }

  // Reset slice count so the next redraw triggers a rebuild with the new texture.
  m_uiNumArraySlices = 0;
}

void ezTextureContext::OnResourceEvent(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUpdated)
  {
    const ezTexture2DResource* pTexture = static_cast<const ezTexture2DResource*>(e.m_pResource);
    if (pTexture->GetFormat() != ezGALResourceFormat::Invalid)
    {
      const bool bIsLinear = !ezGALResourceFormat::IsSrgb(pTexture->GetFormat());
      for (auto& slice : m_SlicePreviews)
      {
        ezResourceLock<ezMaterialResource> pMaterial(slice.m_hMaterial, ezResourceAcquireMode::BlockTillLoaded);
        pMaterial->SetParameter("IsLinear", bIsLinear);
      }

      // Force a rebuild on the next redraw in case the array size changed.
      m_uiNumArraySlices = 0;
    }
  }
}
