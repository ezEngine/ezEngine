#include <PCH.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/Graphics/Geometry.h>

EZ_BEGIN_COMPONENT_TYPE(ezSkyBoxComponent, 3)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("ExposureBias", GetExposureBias, SetExposureBias)->AddAttributes(new ezClampValueAttribute(-32.0f, 32.0f)),
    EZ_ACCESSOR_PROPERTY("InverseTonemap", GetInverseTonemap, SetInverseTonemap)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("CubeMap", GetCubeMap, SetCubeMap)->AddAttributes(new ezAssetBrowserAttribute("Texture Cube"))
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
    ezGeometry geom;
    geom.AddRectXY(ezVec2(2.0f), ezColor::White);

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

    hMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>(szBufferResourceName, desc, szBufferResourceName);
  }

  const char* szMeshResourceName = "SkyBoxMesh";
  m_hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshResourceName);
  if (!m_hMesh.IsValid())
  {
    ezMeshResourceDescriptor desc;
    desc.UseExistingMeshBuffer(hMeshBuffer);
    desc.AddSubMesh(2, 0, 0);
    desc.CalculateBounds();

    m_hMesh = ezResourceManager::CreateResource<ezMeshResource>(szMeshResourceName, desc, szMeshResourceName);
  }

  const char* cubeMapMaterialName = "SkyBoxMaterial_CubeMap";

  m_hCubeMapMaterial = ezResourceManager::GetExistingResource<ezMaterialResource>(cubeMapMaterialName);
  if (!m_hCubeMapMaterial.IsValid())
  {
    ezMaterialResourceDescriptor desc;
    desc.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("{ b4b75b1c-c2c8-4a0e-8076-780bdd46d18b }"); // SkyMaterial

    m_hCubeMapMaterial = ezResourceManager::CreateResource<ezMaterialResource>(cubeMapMaterialName, desc, cubeMapMaterialName);
  }

  UpdateMaterials();
}

ezResult ezSkyBoxComponent::GetLocalBounds(ezBoundingBoxSphere& bounds)
{
  ///\todo
  bounds = ezBoundingSphere(ezVec3::ZeroVector(), 1.0f);
  return EZ_SUCCESS;
}

void ezSkyBoxComponent::OnExtractRenderData(ezExtractRenderDataMessage& msg) const
{
  // Don't render in orthographic views
  if (msg.m_pView->GetRenderCamera()->IsOrthographic())
    return;

  const ezUInt32 uiMeshIDHash = m_hMesh.GetResourceIDHash();

  ezMaterialResourceHandle hMaterial = m_hCubeMapMaterial;
  const ezUInt32 uiMaterialIDHash = hMaterial.IsValid() ? hMaterial.GetResourceIDHash() : 0;

  // Generate batch id from mesh, material and part index.
  ezUInt32 data[] = { uiMeshIDHash, uiMaterialIDHash };
  ezUInt32 uiBatchId = ezHashing::MurmurHash(data, sizeof(data));

  ezMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner(), uiBatchId);
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalTransform.m_vPosition.SetZero(); // skybox should always be at the origin
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hMesh;
    pRenderData->m_hMaterial = hMaterial;
    pRenderData->m_uiPartIndex = 0;
    pRenderData->m_uiUniqueID = GetUniqueID();
  }

  // Determine render data category.
  ezRenderData::Category category;
  if (msg.m_OverrideCategory != ezInvalidIndex)
  {
    category = msg.m_OverrideCategory;
  }
  else
  {
    category = ezDefaultRenderDataCategories::Sky;
  }

  // Sort by material and then by mesh
  ezUInt32 uiSortingKey = (uiMaterialIDHash << 16) | (uiMeshIDHash & 0xFFFF);
  msg.m_pExtractedRenderData->AddRenderData(pRenderData, category, uiSortingKey);
}

void ezSkyBoxComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_fExposureBias;
  s << m_bInverseTonemap;
  s << m_hCubeMap;
}

void ezSkyBoxComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = stream.GetStream();

  s >> m_fExposureBias;
  s >> m_bInverseTonemap;

  if (uiVersion > 2)
  {
    s >> m_hCubeMap;
  }
  else
  {
    ezTexture2DResourceHandle dummyHandle;
    for (int i = 0; i < 6; i++)
    {
      s >> dummyHandle;
    }
  }
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


void ezSkyBoxComponent::SetCubeMap(const char* szFile)
{
  ezTextureCubeResourceHandle hCubeMap;
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hCubeMap = ezResourceManager::LoadResource<ezTextureCubeResource>(szFile);
  }

  m_hCubeMap = hCubeMap;

  UpdateMaterials();
}

const char* ezSkyBoxComponent::GetCubeMap() const
{
  return m_hCubeMap.IsValid() ? m_hCubeMap.GetResourceID() : "";
}

void ezSkyBoxComponent::UpdateMaterials()
{
  if (m_hCubeMapMaterial.IsValid())
  {
    ezResourceLock<ezMaterialResource> pMaterial(m_hCubeMapMaterial);

    pMaterial->SetParameter( "ExposureBias", m_fExposureBias );
    pMaterial->SetParameter( "InverseTonemap", m_bInverseTonemap );
    pMaterial->SetTextureCubeBinding("CubeMap", m_hCubeMap);

    pMaterial->PreserveCurrentDesc();
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class ezSkyBoxComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezSkyBoxComponentPatch_1_2()
    : ezGraphPatch(ezGetStaticRTTI<ezSkyBoxComponent>(), 2) {}

  virtual void Patch(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Exposure Bias", "ExposureBias");
    pNode->RenameProperty("Inverse Tonemap", "InverseTonemap");
    pNode->RenameProperty("Left Texture", "LeftTexture");
    pNode->RenameProperty("Front Texture", "FrontTexture");
    pNode->RenameProperty("Right Texture", "RightTexture");
    pNode->RenameProperty("Back Texture", "BackTexture");
    pNode->RenameProperty("Up Texture", "UpTexture");
    pNode->RenameProperty("Down Texture", "DownTexture");
  }
};

ezSkyBoxComponentPatch_1_2 g_ezSkyBoxComponentPatch_1_2;




EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SkyBoxComponent);

