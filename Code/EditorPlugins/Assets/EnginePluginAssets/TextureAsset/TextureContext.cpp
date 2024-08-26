#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureAsset/TextureContext.h>
#include <EnginePluginAssets/TextureAsset/TextureView.h>

#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureContext, 1, ezRTTIDefaultAllocator<ezTextureContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Texture 2D;Render Target"),
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
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezDocumentConfigMsgToEngine>())
  {
    const ezDocumentConfigMsgToEngine* pMsg2 = static_cast<const ezDocumentConfigMsgToEngine*>(pMsg);

    if (pMsg2->m_sWhatToDo == "ChannelMode" && m_hMaterial.IsValid())
    {
      ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::AllowLoadingFallback);
      pMaterial->SetParameter("ShowChannelMode", pMsg2->m_iValue);
      pMaterial->SetParameter("LodLevel", pMsg2->m_fValue);
    }
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

void ezTextureContext::OnInitialize()
{
  ezStringBuilder sTextureGuid;
  ezConversionUtils::ToString(GetDocumentGuid(), sTextureGuid);
  const ezStringBuilder sMaterialResource(sTextureGuid.GetData(), " - Texture Preview");

  m_hMaterial = ezResourceManager::GetExistingResource<ezMaterialResource>(sMaterialResource);

  m_hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(sTextureGuid);
  ezGALResourceFormat::Enum textureFormat = ezGALResourceFormat::Invalid;
  {
    ezResourceLock<ezTexture2DResource> pTexture(m_hTexture, ezResourceAcquireMode::PointerOnly);

    textureFormat = pTexture->GetFormat();
    pTexture->m_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezTextureContext::OnResourceEvent, this), m_TextureResourceEventSubscriber);
  }

  // Preview Mesh
  const char* szMeshName = "DefaultTexturePreviewMesh";
  m_hPreviewMeshResource = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshName);

  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "DefaultTexturePreviewMeshBuffer";

    ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      // Build geometry
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

      m_hPreviewMeshResource = ezResourceManager::GetOrCreateResource<ezMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
    }
  }

  // Preview Material
  if (!m_hMaterial.IsValid())
  {
    ezMaterialResourceDescriptor md;
    md.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Editor/Materials/TexturePreview.ezMaterial");

    auto& tb = md.m_Texture2DBindings.ExpandAndGetRef();
    tb.m_Name.Assign("BaseTexture");
    tb.m_Value = m_hTexture;

    auto& param = md.m_Parameters.ExpandAndGetRef();
    param.m_Name.Assign("IsLinear");
    param.m_Value = textureFormat != ezGALResourceFormat::Invalid ? !ezGALResourceFormat::IsSrgb(textureFormat) : false;

    m_hMaterial = ezResourceManager::GetOrCreateResource<ezMaterialResource>(sMaterialResource, std::move(md));
  }

  // Preview Object
  {
    EZ_LOCK(m_pWorld->GetWriteMarker());

    ezGameObjectDesc obj;
    ezGameObject* pObj;

    obj.m_sName.Assign("TexturePreview");
    obj.m_LocalRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(90));
    m_hPreviewObject = m_pWorld->CreateObject(obj, pObj);

    ezMeshComponent* pMesh;
    m_hPreviewMesh2D = ezMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hPreviewMeshResource);
    pMesh->SetMaterial(0, m_hMaterial);
  }
}

ezEngineProcessViewContext* ezTextureContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezTextureViewContext, this);
}

void ezTextureContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

void ezTextureContext::OnResourceEvent(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUpdated)
  {
    const ezTexture2DResource* pTexture = static_cast<const ezTexture2DResource*>(e.m_pResource);
    if (pTexture->GetFormat() != ezGALResourceFormat::Invalid)
    {
      ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::BlockTillLoaded);
      pMaterial->SetParameter("IsLinear", !ezGALResourceFormat::IsSrgb(pTexture->GetFormat()));
    }
  }
}
