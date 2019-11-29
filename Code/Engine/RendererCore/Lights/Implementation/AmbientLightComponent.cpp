#include <RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAmbientLightComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("TopColor", GetTopColor, SetTopColor)->AddAttributes(new ezDefaultValueAttribute(ezColorGammaUB(ezColor(0.2f, 0.2f, 0.3f)))),
    EZ_ACCESSOR_PROPERTY("BottomColor", GetBottomColor, SetBottomColor)->AddAttributes(new ezDefaultValueAttribute(ezColorGammaUB(ezColor(0.1f, 0.1f, 0.15f)))),
    EZ_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f))
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering/Lighting"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezAmbientLightComponent::ezAmbientLightComponent() = default;
ezAmbientLightComponent::~ezAmbientLightComponent() = default;

void ezAmbientLightComponent::Deinitialize()
{
  ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

  SUPER::Deinitialize();
}

void ezAmbientLightComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();

  UpdateSkyIrradiance();
}

void ezAmbientLightComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  ezReflectionPool::ResetConstantSkyIrradiance(GetWorld());
}

void ezAmbientLightComponent::SetTopColor(ezColorGammaUB color)
{
  m_TopColor = color;

  if (IsActiveAndInitialized())
  {
    UpdateSkyIrradiance();
  }
}

ezColorGammaUB ezAmbientLightComponent::GetTopColor() const
{
  return m_TopColor;
}

void ezAmbientLightComponent::SetBottomColor(ezColorGammaUB color)
{
  m_BottomColor = color;

  if (IsActiveAndInitialized())
  {
    UpdateSkyIrradiance();
  }
}

ezColorGammaUB ezAmbientLightComponent::GetBottomColor() const
{
  return m_BottomColor;
}

void ezAmbientLightComponent::SetIntensity(float fIntensity)
{
  m_fIntensity = fIntensity;

  if (IsActiveAndInitialized())
  {
    UpdateSkyIrradiance();
  }
}

float ezAmbientLightComponent::GetIntensity() const
{
  return m_fIntensity;
}

void ezAmbientLightComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? ezDefaultSpatialDataCategories::RenderDynamic : ezDefaultSpatialDataCategories::RenderStatic);
}

void ezAmbientLightComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_TopColor;
  s << m_BottomColor;
  s << m_fIntensity;
}

void ezAmbientLightComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_TopColor;
  s >> m_BottomColor;
  s >> m_fIntensity;
}

void ezAmbientLightComponent::UpdateSkyIrradiance()
{
  ezColor topColor = ezColor(m_TopColor) * m_fIntensity;
  ezColor bottomColor = ezColor(m_BottomColor) * m_fIntensity;
  ezColor midColor = ezMath::Lerp(bottomColor, topColor, 0.5f);

  ezAmbientCube<ezColor> ambientLightIrradiance;
  ambientLightIrradiance.m_Values[ezAmbientCubeBasis::PosX] = midColor;
  ambientLightIrradiance.m_Values[ezAmbientCubeBasis::NegX] = midColor;
  ambientLightIrradiance.m_Values[ezAmbientCubeBasis::PosY] = midColor;
  ambientLightIrradiance.m_Values[ezAmbientCubeBasis::NegY] = midColor;
  ambientLightIrradiance.m_Values[ezAmbientCubeBasis::PosZ] = topColor;
  ambientLightIrradiance.m_Values[ezAmbientCubeBasis::NegZ] = bottomColor;

  ezReflectionPool::SetConstantSkyIrradiance(GetWorld(), ambientLightIrradiance);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class ezAmbientLightComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezAmbientLightComponentPatch_1_2()
    : ezGraphPatch("ezAmbientLightComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Top Color", "TopColor");
    pNode->RenameProperty("Bottom Color", "BottomColor");
  }
};

ezAmbientLightComponentPatch_1_2 g_ezAmbientLightComponentPatch_1_2;



EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_AmbientLightComponent);
