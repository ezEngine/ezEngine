#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation/Components/VoxelGridSettingsComponent.h>
#include <AiPlugin/Navigation/VoxelWorldModule.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAiVoxelGridSettingsComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("ResolutionX", GetResolutionX, SetResolutionX)->AddAttributes(new ezDefaultValueAttribute(64), new ezClampValueAttribute(4, 512)),
    EZ_ACCESSOR_PROPERTY("ResolutionY", GetResolutionY, SetResolutionY)->AddAttributes(new ezDefaultValueAttribute(64), new ezClampValueAttribute(4, 512)),
    EZ_ACCESSOR_PROPERTY("ResolutionZ", GetResolutionZ, SetResolutionZ)->AddAttributes(new ezDefaultValueAttribute(32), new ezClampValueAttribute(4, 256)),
    EZ_ACCESSOR_PROPERTY("VoxelSize", GetVoxelSize, SetVoxelSize)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.1f, 10.0f)),
    EZ_ACCESSOR_PROPERTY("CollisionLayer", GetCollisionLayer, SetCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI/Navigation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAiVoxelGridSettingsComponent::ezAiVoxelGridSettingsComponent() = default;
ezAiVoxelGridSettingsComponent::~ezAiVoxelGridSettingsComponent() = default;

void ezAiVoxelGridSettingsComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // Ensure the voxel world module is created when this settings component exists in the scene
  GetWorld()->GetOrCreateModule<ezAiVoxelWorldModule>();
}

void ezAiVoxelGridSettingsComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_uiResolutionX;
  s << m_uiResolutionY;
  s << m_uiResolutionZ;
  s << m_fVoxelSize;
  s << m_uiCollisionLayer;
}

void ezAiVoxelGridSettingsComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_uiResolutionX;
  s >> m_uiResolutionY;
  s >> m_uiResolutionZ;
  s >> m_fVoxelSize;
  s >> m_uiCollisionLayer;
}

void ezAiVoxelGridSettingsComponent::SetResolutionX(ezUInt32 uiValue)
{
  m_uiResolutionX = uiValue;
  SetModified(EZ_BIT(0));
}

void ezAiVoxelGridSettingsComponent::SetResolutionY(ezUInt32 uiValue)
{
  m_uiResolutionY = uiValue;
  SetModified(EZ_BIT(1));
}

void ezAiVoxelGridSettingsComponent::SetResolutionZ(ezUInt32 uiValue)
{
  m_uiResolutionZ = uiValue;
  SetModified(EZ_BIT(2));
}

void ezAiVoxelGridSettingsComponent::SetVoxelSize(float fValue)
{
  m_fVoxelSize = fValue;
  SetModified(EZ_BIT(3));
}

void ezAiVoxelGridSettingsComponent::SetCollisionLayer(ezUInt32 uiValue)
{
  m_uiCollisionLayer = uiValue;
  SetModified(EZ_BIT(4));
}

EZ_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation_Components_VoxelGridSettingsComponent);
