#include <PCH.h>
#include <RendererCore/Decals/DecalComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/Graphics/Camera.h>
#include <RendererCore/Decals/DecalResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_COMPONENT_TYPE(ezDecalComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Decal", GetDecalFile, SetDecalFile)->AddAttributes(new ezAssetBrowserAttribute("Decal")),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("FX"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.5f, ezColor::LightSteelBlue),
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
  /// \todo bounds
  bounds = ezBoundingSphere(ezVec3::ZeroVector(), 1.0f);
  return EZ_SUCCESS;
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

}
