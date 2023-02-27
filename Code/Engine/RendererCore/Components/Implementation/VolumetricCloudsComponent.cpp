#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Components/VolumetricCloudsComponent.h>

#include <Core/Graphics/Geometry.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Pipeline/View.h>

#include <RendererCore/Lights/DirectionalLightComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezVolumetricCloudsComponent, 1, ezComponentMode::Static)
{

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezVolumetricCloudsComponent::ezVolumetricCloudsComponent() = default;
ezVolumetricCloudsComponent::~ezVolumetricCloudsComponent() = default;

void ezVolumetricCloudsComponent::Initialize()
{
  SUPER::Initialize();

  const char* szBufferResourceName = "SkyBoxBuffer";
  ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szBufferResourceName);
  if (!hMeshBuffer.IsValid())
  {
    ezGeometry geom;
    geom.AddRectXY(ezVec2(2.0f));

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

    hMeshBuffer = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>(szBufferResourceName, std::move(desc), szBufferResourceName);
  }

  const char* szMeshResourceName = "VolulmetricCloudsMesh";
  m_hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshResourceName);
  if (!m_hMesh.IsValid())
  {
    ezMeshResourceDescriptor desc;
    desc.UseExistingMeshBuffer(hMeshBuffer);
    desc.AddSubMesh(2, 0, 0);
    desc.ComputeBounds();

    m_hMesh = ezResourceManager::GetOrCreateResource<ezMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
  }

  ezStringBuilder cubeMapMaterialName = "VolulmetricCloudsMaterial";
  cubeMapMaterialName.AppendFormat("_{0}", ezArgP(GetWorld())); // make the resource unique for each world

  m_hMaterial = ezResourceManager::GetExistingResource<ezMaterialResource>(cubeMapMaterialName);
  if (!m_hMaterial.IsValid())
  {
    ezMaterialResourceDescriptor desc;
    desc.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("{ 0ff457ed-7091-4bce-879f-21da28318922 }"); // VolulmetricClouds.ezMaterialAsset

    m_hMaterial = ezResourceManager::CreateResource<ezMaterialResource>(cubeMapMaterialName, std::move(desc), cubeMapMaterialName);
  }

  //m_hNoiseLut = ezResourceManager::LoadResource<ezTexture3DResource>("{ faa1b9db-72ec-4c99-af9a-82bcb18fbcf3 }"); // CloudNoise.ezLUTAsset (home-work)
  //m_hNoiseLut = ezResourceManager::LoadResource<ezTexture3DResource>("{ 68eb18b0-726b-4836-947c-209261299239 }"); // CloudNoise.ezLUTAsset (work)
  m_hNoiseLut = ezResourceManager::LoadResource<ezTexture3DResource>("{ f9eb17ec-7a4c-4b74-b51a-4bf2656e4b10 }"); // CloudNoise.ezLUTAsset (home-home)
  if(!m_hNoiseLut.IsValid())
  {
    ezLog::Error("Failed to find resource CloudNoise.ezLUTAsset (faa1b9db-72ec-4c99-af9a-82bcb18fbcf3)");
  }

  UpdateMaterials();
}

ezResult ezVolumetricCloudsComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return EZ_SUCCESS;
}

void ezVolumetricCloudsComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't extract sky render data for selection or in orthographic views.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory || msg.m_pView->GetCamera()->IsOrthographic())
    return;

  UpdateMaterials();

  ezMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = ezTransform::IdentityTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hMesh;
    pRenderData->m_hMaterial = m_hMaterial;
    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    pRenderData->FillBatchIdAndSortingKey();
  }

  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Sky, ezRenderData::Caching::Never);
}

void ezVolumetricCloudsComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();
}

void ezVolumetricCloudsComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = inout_stream.GetStream();
}

void ezVolumetricCloudsComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateMaterials();
}

void ezVolumetricCloudsComponent::UpdateMaterials() const
{
  if (m_hMaterial.IsValid())
  {
    ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::AllowLoadingFallback);

    ezVec3 sunDirection = GetOwner()->GetGlobalTransform().m_qRotation * ezVec3(-1, 0, 0);
    pMaterial->SetParameter("SunDir", sunDirection);
    pMaterial->SetTexture3DBinding("NoiseMap", m_hNoiseLut);

    pMaterial->PreserveCurrentDesc();
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_ezVolulmetricCloudsComponent);
