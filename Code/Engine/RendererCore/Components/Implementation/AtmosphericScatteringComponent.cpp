#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Components/AtmosphericScatteringComponent.h>

#include <Core/Graphics/Geometry.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Pipeline/View.h>

#include <RendererCore/Lights/DirectionalLightComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAtmosphericScatteringComponent, 1, ezComponentMode::Static)
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

ezAtmosphericScatteringComponent::ezAtmosphericScatteringComponent() = default;
ezAtmosphericScatteringComponent::~ezAtmosphericScatteringComponent() = default;

void ezAtmosphericScatteringComponent::Initialize()
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

  const char* szMeshResourceName = "AtmosphericScatteringMesh";
  m_hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshResourceName);
  if (!m_hMesh.IsValid())
  {
    ezMeshResourceDescriptor desc;
    desc.UseExistingMeshBuffer(hMeshBuffer);
    desc.AddSubMesh(2, 0, 0);
    desc.ComputeBounds();

    m_hMesh = ezResourceManager::GetOrCreateResource<ezMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
  }

  ezStringBuilder cubeMapMaterialName = "AtmosphericScatteringMaterial";
  cubeMapMaterialName.AppendFormat("_{0}", ezArgP(GetWorld())); // make the resource unique for each world

  m_hMaterial = ezResourceManager::GetExistingResource<ezMaterialResource>(cubeMapMaterialName);
  if (!m_hMaterial.IsValid())
  {
    ezMaterialResourceDescriptor desc;
    desc.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("{ ea1cfb3f-d720-47d3-8d9a-188c48cbc830 }"); // AtmosphericScattering.ezMaterialAsset

    m_hMaterial = ezResourceManager::CreateResource<ezMaterialResource>(cubeMapMaterialName, std::move(desc), cubeMapMaterialName);
  }

  UpdateMaterials();
}

ezResult ezAtmosphericScatteringComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return EZ_SUCCESS;
}

void ezAtmosphericScatteringComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't extract sky render data for selection or in orthographic views.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory || msg.m_pView->GetCamera()->IsOrthographic())
    return;


  ezVec3 sunDirection = GetOwner()->GetGlobalTransform().m_qRotation * ezVec3(-1, 0, 0);

  ezMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalTransform.m_vPosition.SetZero(); // skybox should always be at the origin
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hMesh;
    pRenderData->m_hMaterial = m_hMaterial;
    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    pRenderData->FillBatchIdAndSortingKey();
  }

  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Sky, ezRenderData::Caching::Never);
}

void ezAtmosphericScatteringComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();
}

void ezAtmosphericScatteringComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = inout_stream.GetStream();
}

void ezAtmosphericScatteringComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateMaterials();
}

void ezAtmosphericScatteringComponent::UpdateMaterials()
{
  if (m_hMaterial.IsValid())
  {
    ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::AllowLoadingFallback);

    pMaterial->PreserveCurrentDesc();
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_ezAtmosphericScatteringComponent);
