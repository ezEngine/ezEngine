#include <PCH.h>
#include <RendererCore/Decals/DecalComponent.h>
#include <RendererCore/Decals/DecalResource.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/Graphics/Camera.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_COMPONENT_TYPE(ezDecalComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0), ezVariant())),
    EZ_ACCESSOR_PROPERTY("Decal", GetDecalFile, SetDecalFile)->AddAttributes(new ezAssetBrowserAttribute("Decal")),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("FX"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.5f, ezColor::LightSteelBlue),
    new ezBoxManipulatorAttribute("Extents"),
    new ezBoxVisualizerAttribute("Extents"),
  }
  EZ_END_ATTRIBUTES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_COMPONENT_TYPE

ezDecalComponent::ezDecalComponent()
{
  m_vExtents.Set(1.0f);
}

ezDecalComponent::~ezDecalComponent()
{
}

void ezDecalComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_hDecal;
}

void ezDecalComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = stream.GetStream();

  s >> m_hDecal;
}

ezResult ezDecalComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  bounds = ezBoundingBox(m_vExtents * -0.5f, m_vExtents * 0.5f);
  return EZ_SUCCESS;
}

void ezDecalComponent::SetExtents(const ezVec3& value)
{
  m_vExtents = value.CompMax(ezVec3::ZeroVector());

  TriggerLocalBoundsUpdate();
}

const ezVec3& ezDecalComponent::GetExtents() const
{
  return m_vExtents;
}

void ezDecalComponent::SetDecal(const ezDecalResourceHandle& hDecal)
{
  m_hDecal = hDecal;
}

const ezDecalResourceHandle& ezDecalComponent::GetDecal() const
{
  return m_hDecal;
}

void ezDecalComponent::SetDecalFile(const char* szFile)
{
  ezDecalResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezDecalResource>(szFile);
  }

  SetDecal(hResource);
}

const char* ezDecalComponent::GetDecalFile() const
{
  if (!m_hDecal.IsValid())
    return "";

  return m_hDecal.GetResourceID();
}

void ezDecalComponent::OnExtractRenderData(ezExtractRenderDataMessage& msg) const
{
  // Don't extract decal render data for selection.
  if (msg.m_OverrideCategory != ezInvalidIndex)
    return;

  ezUInt32 uiBatchId = 0;
  auto pRenderData = ezCreateRenderDataForThisFrame<ezDecalRenderData>(GetOwner(), uiBatchId);

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_vHalfExtents = m_vExtents * 0.5f;

  msg.m_pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::Decal, uiBatchId);
}
