#include <PCH.h>
#include <EnginePluginAssets/TextureAsset/TextureContext.h>
#include <EnginePluginAssets/TextureAsset/TextureView.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <CoreUtils/Geometry/GeomUtils.h>
#include <SharedPluginAssets/Common/Messages.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <SharedPluginAssets/Common/Messages.h>
#include <RendererCore/Lights/AmbientLightComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureContext, 1, ezRTTIDefaultAllocator<ezTextureContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Texture Asset"),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

void CreatePreviewRect(ezGeometry& geom)
{
  const ezMat4 mTransform = ezMat4::IdentityMatrix();
  const ezVec2 size(1.0f);
  const ezColor color = ezColor::White;

  const ezVec2 halfSize = size * 0.5f;

  ezUInt32 idx[4];

  idx[0] = geom.AddVertex(ezVec3(-halfSize.x, 0, -halfSize.y), ezVec3(-1, 0, 0), ezVec2(-1, 2), color, 0, mTransform);
  idx[1] = geom.AddVertex(ezVec3(halfSize.x, 0, -halfSize.y), ezVec3(-1, 0, 0), ezVec2(2, 2), color, 0, mTransform);
  idx[2] = geom.AddVertex(ezVec3(halfSize.x, 0, halfSize.y), ezVec3(-1, 0, 0), ezVec2(2, -1), color, 0, mTransform);
  idx[3] = geom.AddVertex(ezVec3(-halfSize.x, 0, halfSize.y), ezVec3(-1, 0, 0), ezVec2(-1, -1), color, 0, mTransform);

  geom.AddPolygon(idx, false);
}

void ezTextureContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
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

void ezTextureContext::OnInitialize()
{
  const char* szMeshName = "DefaultTexturePreviewMesh";
  const ezString sTextureGuid = ezConversionUtils::ToString(GetDocumentGuid());
  const ezStringBuilder sMaterialResource(sTextureGuid.GetData(), " - Texture Preview");

  ezMeshResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshName);
  m_hMaterial = ezResourceManager::GetExistingResource<ezMaterialResource>(sMaterialResource);

  // Preview Mesh
  if (!hMesh.IsValid())
  {
    const char* szMeshBufferName = "DefaultTexturePreviewMeshBuffer";

    ezMeshBufferResourceHandle hMeshBuffer;
    {
      // Build geometry
      ezGeometry geom;
      CreatePreviewRect(geom);
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

  // Preview Material
  if (!m_hMaterial.IsValid())
  {
    const ezMaterialResourceHandle hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/Editor/TexturePreview.ezMaterial");

    ezMaterialResourceDescriptor::TextureBinding tb;
    tb.m_Name.Assign("BaseTexture");
    tb.m_Value = ezResourceManager::LoadResource<ezTextureResource>(sTextureGuid);

    ezMaterialResourceDescriptor md;
    md.m_hBaseMaterial = hBaseMaterial;
    md.m_TextureBindings.PushBack(tb);

    m_hMaterial = ezResourceManager::CreateResource<ezMaterialResource>(sMaterialResource, md);
  }

  // Preview Object
  {
    EZ_LOCK(m_pWorld->GetWriteMarker());

    ezGameObjectDesc obj;
    ezGameObject* pObj;

    obj.m_sName.Assign("TexturePreview");
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));
    m_pWorld->CreateObject(obj, pObj);

    ezMeshComponent* pMesh;
    ezMeshComponent::CreateComponent(m_pWorld, pMesh);
    pMesh->SetMesh(hMesh);
    pMesh->SetMaterial(0, m_hMaterial);

    pObj->AttachComponent(pMesh);
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

