#include <PCH.h>
#include <RendererCore/Decals/DecalComponent.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
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
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder),
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
  : m_vExtents(1.0f)
  , m_Color(ezColor::White)
  , m_fSortOrder(0.0f)
{
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

void ezDecalComponent::SetColor(ezColorGammaUB color)
{
  m_Color = color;
}

ezColorGammaUB ezDecalComponent::GetColor() const
{
  return m_Color;
}

void ezDecalComponent::SetSortOrder(float fOrder)
{
  m_fSortOrder = fOrder;
}

float ezDecalComponent::GetSortOrder() const
{
  return m_fSortOrder;
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

  auto hDecalAtlas = ezResourceManager::LoadResource<ezDecalAtlasResource>("AssetCache/PC/Decals.ezDecal");
  ezVec2 baseAtlasScale = ezVec2(0.5f);
  ezVec2 baseAtlasOffset = ezVec2(0.5f);

  {
    ezResourceLock<ezDecalAtlasResource> pDecalAtlas(hDecalAtlas, ezResourceAcquireMode::NoFallback);
    ezVec2U32 baseTextureSize = pDecalAtlas->GetBaseColorTextureSize();

    if (auto pDecalInfo = pDecalAtlas->GetAllDecals().GetValue(m_hDecal.GetResourceIDHash()))
    {
      baseAtlasScale.x = (float)pDecalInfo->m_BaseColorRect.width / baseTextureSize.x * 0.5f;
      baseAtlasScale.y = (float)pDecalInfo->m_BaseColorRect.height / baseTextureSize.y * 0.5f;
      baseAtlasOffset.x = (float)pDecalInfo->m_BaseColorRect.x / baseTextureSize.x + baseAtlasScale.x;
      baseAtlasOffset.y = (float)pDecalInfo->m_BaseColorRect.y / baseTextureSize.y + baseAtlasScale.y;
    }
  }

  ezUInt32 uiBatchId = 0;
  auto pRenderData = ezCreateRenderDataForThisFrame<ezDecalRenderData>(GetOwner(), uiBatchId);

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_vHalfExtents = m_vExtents * 0.5f;
  pRenderData->m_Color = m_Color;
  pRenderData->m_vBaseAtlasScale = baseAtlasScale;
  pRenderData->m_vBaseAtlasOffset = baseAtlasOffset;

  ezUInt32 uiSortingId = (ezUInt32)(m_fSortOrder * 65536.0f + 65536.0f);
  msg.m_pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::Decal, uiSortingId);
}
