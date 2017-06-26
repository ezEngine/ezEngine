#include <PCH.h>
#include <EnginePluginAssets/TextureCubeAsset/TextureCubeContext.h>
#include <EnginePluginAssets/TextureCubeAsset/TextureCubeView.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Core/Graphics/Geometry.h>
#include <SharedPluginAssets/Common/Messages.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <SharedPluginAssets/Common/Messages.h>
#include <RendererCore/Lights/AmbientLightComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureCubeContext, 1, ezRTTIDefaultAllocator<ezTextureCubeContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "TextureCube Asset"),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezTextureCubeContext::ezTextureCubeContext()
  : m_TextureFormat(ezGALResourceFormat::Invalid)
  , m_uiTextureWidthAndHeight(0)
  , m_bAddedEventHandler(false)
{

}

void ezTextureCubeContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezDocumentConfigMsgToEngine>())
  {
    const ezDocumentConfigMsgToEngine* pMsg2 = static_cast<const ezDocumentConfigMsgToEngine*>(pMsg);

    if (pMsg2->m_sWhatToDo == "ChannelMode" && m_hMaterial.IsValid())
    {
      ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial);
      pMaterial->SetParameter("ShowChannelMode", pMsg2->m_iValue);
      pMaterial->SetParameter("LodLevel", pMsg2->m_fValue);
    }
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

void ezTextureCubeContext::GetTextureStats(ezGALResourceFormat::Enum& format, ezUInt32& uiWidthAndHeight)
{
  format = m_TextureFormat;
  uiWidthAndHeight = m_uiTextureWidthAndHeight;

  UpdatePreview();
}

void ezTextureCubeContext::OnInitialize()
{
  const char* szMeshName = "DefaultTextureCubePreviewMesh";
  ezStringBuilder sTextureGuid;
  ezConversionUtils::ToString(GetDocumentGuid(), sTextureGuid);
  const ezStringBuilder sMaterialResource(sTextureGuid.GetData(), " - TextureCube Preview");

  ezMeshResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshName);
  m_hMaterial = ezResourceManager::GetExistingResource<ezMaterialResource>(sMaterialResource);

  m_hTexture = ezResourceManager::LoadResource<ezTextureCubeResource>(sTextureGuid);
  {
    ezResourceLock<ezTextureCubeResource> pTexture(m_hTexture, ezResourceAcquireMode::NoFallback);

    m_TextureFormat = pTexture->GetFormat();
    m_uiTextureWidthAndHeight = pTexture->GetWidthAndHeight();

    if (!pTexture->IsMissingResource())
    {
      m_bAddedEventHandler = true;
      pTexture->m_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezTextureCubeContext::OnResourceEvent, this));
    }
  }

  // Preview Mesh
  if (!hMesh.IsValid())
  {
    const char* szMeshBufferName = "DefaultTextureCubePreviewMeshBuffer";

    ezMeshBufferResourceHandle hMeshBuffer;
    {
      // Build geometry
      ezGeometry geom;
      geom.AddSphere(0.5f, 64, 64, ezColor::White);
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
      md.ComputeBounds();

      hMesh = ezResourceManager::CreateResource<ezMeshResource>(szMeshName, md, pMeshBuffer->GetResourceDescription());
    }
  }

  // Preview Material
  if (!m_hMaterial.IsValid())
  {
    ezMaterialResourceDescriptor md;
    md.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/Editor/TextureCubePreview.ezMaterial");

    auto& tb = md.m_TextureCubeBindings.ExpandAndGetRef();
    tb.m_Name.Assign("BaseTexture");
    tb.m_Value = m_hTexture;

    auto& param = md.m_Parameters.ExpandAndGetRef();
    param.m_Name.Assign("IsLinear");
    param.m_Value = !ezGALResourceFormat::IsSrgb(m_TextureFormat);

    m_hMaterial = ezResourceManager::CreateResource<ezMaterialResource>(sMaterialResource, md);
  }

  // Preview Object
  {
    EZ_LOCK(m_pWorld->GetWriteMarker());

    ezGameObjectDesc obj;
    ezGameObject* pObj;

    obj.m_sName.Assign("TextureCubePreview");
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));
    m_hPreviewObject = m_pWorld->CreateObject(obj, pObj);

    ezMeshComponent* pMesh;
    m_hPreviewMesh2D = ezMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(hMesh);
    pMesh->SetMaterial(0, m_hMaterial);
  }

  UpdatePreview();
}

void ezTextureCubeContext::OnDeinitialize()
{
  if (m_bAddedEventHandler)
  {
    ezResourceLock<ezTextureCubeResource> pTexture(m_hTexture, ezResourceAcquireMode::NoFallback);

    pTexture->m_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezTextureCubeContext::OnResourceEvent, this));
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
  if (e.m_EventType == ezResourceEventType::ResourceContentUpdated)
  {
    const ezTextureCubeResource* pTexture = static_cast<const ezTextureCubeResource*>(e.m_pResource);

    m_TextureFormat = pTexture->GetFormat();
    m_uiTextureWidthAndHeight = pTexture->GetWidthAndHeight();

    // cannot call this here, because the event happens on another thread at any possible time
    //UpdatePreview();
  }
}

void ezTextureCubeContext::UpdatePreview()
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  if (!m_bAddedEventHandler)
  {
    ezResourceLock<ezTextureCubeResource> pTexture(m_hTexture, ezResourceAcquireMode::NoFallback);

    if (!pTexture->IsMissingResource())
    {
      m_bAddedEventHandler = true;
      pTexture->m_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezTextureCubeContext::OnResourceEvent, this));
    }
  }
}

