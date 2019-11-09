#include <BakingPluginPCH.h>

#include <BakingPlugin/Components/BakingSettingsComponent.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezBakingSettingsComponent, 1, ezComponentMode::Static)
{
  /*EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("TopColor", GetTopColor, SetTopColor)->AddAttributes(new ezDefaultValueAttribute(ezColorGammaUB(ezColor(0.2f, 0.2f, 0.3f)))),
    EZ_ACCESSOR_PROPERTY("BottomColor", GetBottomColor, SetBottomColor)->AddAttributes(new ezDefaultValueAttribute(ezColorGammaUB(ezColor(0.1f, 0.1f, 0.15f)))),
    EZ_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f))
  }
  EZ_END_PROPERTIES;*/
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering/Baking"),
    new ezLongOpAttribute("ezLongOpProxy_BakeScene")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezBakingSettingsComponent::ezBakingSettingsComponent() = default;
ezBakingSettingsComponent::~ezBakingSettingsComponent() = default;

void ezBakingSettingsComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
}

void ezBakingSettingsComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();
}

void ezBakingSettingsComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  if (m_bShowDebugOverlay || m_bShowDebugProbes)
  {
    msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? ezDefaultSpatialDataCategories::RenderDynamic : ezDefaultSpatialDataCategories::RenderStatic);
  }
}

void ezBakingSettingsComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();
}

void ezBakingSettingsComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();
}
