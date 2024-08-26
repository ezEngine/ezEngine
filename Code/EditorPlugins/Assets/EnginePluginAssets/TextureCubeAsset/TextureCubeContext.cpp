#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureCubeAsset/TextureCubeContext.h>
#include <EnginePluginAssets/TextureCubeAsset/TextureCubeView.h>

#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureCubeContext, 1, ezRTTIDefaultAllocator<ezTextureCubeContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Texture Cube"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTextureCubeContext::ezTextureCubeContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
}

void ezTextureCubeContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
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

void ezTextureCubeContext::OnInitialize()
{
  const char* szMeshName = "DefaultTextureCubePreviewMesh";
  ezStringBuilder sTextureGuid;
  ezConversionUtils::ToString(GetDocumentGuid(), sTextureGuid);
  const ezStringBuilder sMaterialResource(sTextureGuid.GetData(), " - TextureCube Preview");

  m_hPreviewMeshResource = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshName);
  m_hMaterial = ezResourceManager::GetExistingResource<ezMaterialResource>(sMaterialResource);

  m_hTexture = ezResourceManager::LoadResource<ezTextureCubeResource>(sTextureGuid);
  ezGALResourceFormat::Enum textureFormat = ezGALResourceFormat::Invalid;
  {
    ezResourceLock<ezTextureCubeResource> pTexture(m_hTexture, ezResourceAcquireMode::PointerOnly);

    textureFormat = pTexture->GetFormat();
    pTexture->m_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezTextureCubeContext::OnResourceEvent, this), m_TextureResourceEventSubscriber);
  }

  // Preview Mesh
  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "DefaultTextureCubePreviewMeshBuffer";

    ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      // Build geometry
      ezGeometry geom;
      geom.AddStackedSphere(0.5f, 64, 64);
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
    md.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Editor/Materials/TextureCubePreview.ezMaterial");

    auto& tb = md.m_TextureCubeBindings.ExpandAndGetRef();
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

    obj.m_sName.Assign("TextureCubePreview");
    obj.m_LocalRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(90));
    m_hPreviewObject = m_pWorld->CreateObject(obj, pObj);

    ezMeshComponent* pMesh;
    m_hPreviewMesh2D = ezMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hPreviewMeshResource);
    pMesh->SetMaterial(0, m_hMaterial);
  }
}

ezEngineProcessViewContext* ezTextureCubeContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezTextureCubeViewContext, this);
}

void ezTextureCubeContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

void ezTextureCubeContext::OnResourceEvent(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUpdated)
  {
    const ezTextureCubeResource* pTexture = static_cast<const ezTextureCubeResource*>(e.m_pResource);
    if (pTexture->GetFormat() != ezGALResourceFormat::Invalid)
    {
      ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::BlockTillLoaded);
      pMaterial->SetParameter("IsLinear", !ezGALResourceFormat::IsSrgb(pTexture->GetFormat()));
    }
  }
}
