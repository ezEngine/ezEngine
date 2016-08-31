#include <RendererCore/PCH.h>
#include <RendererCore/Sky/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <CoreUtils/Geometry/GeomUtils.h>

EZ_BEGIN_COMPONENT_TYPE(ezSkyBoxComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Exposure Bias", GetExposureBias, SetExposureBias)->AddAttributes(new ezClampValueAttribute(-32.0f, 32.0f)),
    EZ_ACCESSOR_PROPERTY("Inverse Tonemap", GetInverseTonemap, SetInverseTonemap)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("Left Texture", GetLeftTextureFile, SetLeftTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_ACCESSOR_PROPERTY("Front Texture", GetFrontTextureFile, SetFrontTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_ACCESSOR_PROPERTY("Right Texture", GetRightTextureFile, SetRightTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_ACCESSOR_PROPERTY("Back Texture", GetBackTextureFile, SetBackTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_ACCESSOR_PROPERTY("Up Texture", GetUpTextureFile, SetUpTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_ACCESSOR_PROPERTY("Down Texture", GetDownTextureFile, SetDownTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
  }
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
  }
  EZ_END_ATTRIBUTES
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_COMPONENT_TYPE

ezSkyBoxComponent::ezSkyBoxComponent()
  : m_fExposureBias(0.0f)
  , m_bInverseTonemap(true)
{
}

ezSkyBoxComponent::~ezSkyBoxComponent()
{
}

void ezSkyBoxComponent::Initialize()
{
  SUPER::Initialize();

  const char* szBufferResourceName = "SkyBoxBuffer";
  ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szBufferResourceName);
  if (!hMeshBuffer.IsValid())
  {
    ezMat4 transform;
    transform.SetIdentity();
    transform.SetDiagonal(ezVec4(-1.0f, 1.0f, 1.0f, 1.0f));

    ezGeometry geom;
    geom.AddTexturedBox(ezVec3(1.0f), ezColor::White, transform);

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat);
    desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

    hMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>(szBufferResourceName, desc, szBufferResourceName);
  }

  const char* szMeshResourceName = "SkyBoxMesh";
  m_hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshResourceName);
  if (!m_hMesh.IsValid())
  {
    ezMeshResourceDescriptor desc;
    desc.UseExistingMeshBuffer(hMeshBuffer);
    for (ezUInt32 i = 0; i < 6; ++i)
    {
      desc.AddSubMesh(2, i*2, 0);
    }
    desc.CalculateBounds();

    m_hMesh = ezResourceManager::CreateResource<ezMeshResource>(szMeshResourceName, desc, szMeshResourceName);
  }

  for (ezUInt32 i = 0; i < 6; ++i)
  {
    ezMaterialResourceDescriptor desc;
    desc.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("{ b4b75b1c-c2c8-4a0e-8076-780bdd46d18b }"); // SkyMaterial

    {
      auto& param = desc.m_Parameters.ExpandAndGetRef();
      param.m_Name.Assign("ExposureBias");
      param.m_Value = m_fExposureBias;
    }
     
    {
      auto& param = desc.m_Parameters.ExpandAndGetRef();
      param.m_Name.Assign("InverseTonemap");
      param.m_Value = m_bInverseTonemap;
    }

    if (m_Textures[i].IsValid())
    {
      auto& binding = desc.m_TextureBindings.ExpandAndGetRef();
      binding.m_Name.Assign("BaseTexture");
      binding.m_Value = m_Textures[i];
    }

    ezStringBuilder temp;
    temp.Format("SkyBoxMaterial_%08X_%d", GetOwner()->GetHandle().GetInternalID().m_Data, i);
    m_Materials[i] = ezResourceManager::CreateResource<ezMaterialResource>(temp.GetData(), desc, temp.GetData());
  }
}

ezResult ezSkyBoxComponent::GetLocalBounds(ezBoundingBoxSphere& bounds)
{
  ///\todo
  bounds = ezBoundingSphere(ezVec3::ZeroVector(), 100000.0f);
  return EZ_SUCCESS;
}

void ezSkyBoxComponent::OnExtractRenderData(ezExtractRenderDataMessage& msg) const
{
  const ezUInt32 uiMeshIDHash = m_hMesh.GetResourceIDHash();

  for (ezUInt32 i = 0; i < 6; ++i)
  {
    ezMaterialResourceHandle hMaterial = m_Materials[i];
    const ezUInt32 uiMaterialIDHash = hMaterial.IsValid() ? hMaterial.GetResourceIDHash() : 0;

    // Generate batch id from mesh, material and part index. 
    ezUInt32 data[] = { uiMeshIDHash, uiMaterialIDHash, i };
    ezUInt32 uiBatchId = ezHashing::MurmurHash(data, sizeof(data));

    ezMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner(), uiBatchId);
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;
      pRenderData->m_uiPartIndex = i;
      pRenderData->m_uiEditorPickingID = GetEditorPickingID() | (i << 24);
    }

    // Determine render data category.
    ezRenderData::Category category;
    if (msg.m_OverrideCategory != ezInvalidIndex)
    {
      category = msg.m_OverrideCategory;
    }
    else
    {
      ///\todo: sky category
      category = ezDefaultRenderDataCategories::LitOpaque;
    }

    // Sort by material and then by mesh
    ezUInt32 uiSortingKey = (uiMaterialIDHash << 16) | (uiMeshIDHash & 0xFFFF);
    msg.m_pExtractedRenderData->AddRenderData(pRenderData, category, uiSortingKey);
  }
}

void ezSkyBoxComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  
}

void ezSkyBoxComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = stream.GetStream();

  
}

void ezSkyBoxComponent::SetExposureBias(float fExposureBias)
{
  m_fExposureBias = fExposureBias;

  UpdateMaterials();
}

void ezSkyBoxComponent::SetInverseTonemap(bool bInverseTonemap)
{
  m_bInverseTonemap = bInverseTonemap;

  UpdateMaterials();
}

void ezSkyBoxComponent::SetTextureFile(ezUInt32 uiIndex, const char* szFile)
{
  ezTextureResourceHandle hTexture;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hTexture = ezResourceManager::LoadResource<ezTextureResource>(szFile);
  }

  m_Textures[uiIndex] = hTexture;

  UpdateMaterials();
}

const char* ezSkyBoxComponent::GetTextureFile(ezUInt32 uiIndex) const
{
  return m_Textures[uiIndex].IsValid() ? m_Textures[uiIndex].GetResourceID() : "";
}

void ezSkyBoxComponent::UpdateMaterials()
{
  for (ezUInt32 i = 0; i < 6; ++i)
  {
    ezMaterialResourceHandle hMaterial = m_Materials[i];
    if (!hMaterial.IsValid())
      continue;

    ezResourceLock<ezMaterialResource> pMaterial(hMaterial);

    pMaterial->SetParameter(ezMakeHashedString("ExposureBias"), m_fExposureBias);
    pMaterial->SetParameter(ezMakeHashedString("InverseTonemap"), m_bInverseTonemap);
    pMaterial->SetTextureBinding(ezMakeHashedString("BaseTexture"), m_Textures[i]);
  }
}

