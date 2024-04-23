#include <CppProjectPlugin/CppProjectPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <CppProjectPlugin/Components/SampleBounceComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(SampleBounceComponent, 1 /* version */, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Amplitude", m_fAmplitude)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(0, 10)),
    EZ_MEMBER_PROPERTY("Speed", m_Speed)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(90))),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Game"), // Component menu group
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

SampleBounceComponent::SampleBounceComponent() = default;
SampleBounceComponent::~SampleBounceComponent() = default;

void SampleBounceComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // this component doesn't need to anything for initialization
}

void SampleBounceComponent::Update()
{
  const ezTime curTime = GetWorld()->GetClock().GetAccumulatedTime();
  const ezAngle curAngle = curTime.AsFloatInSeconds() * m_Speed;
  const float curHeight = ezMath::Sin(curAngle) * m_fAmplitude;

  GetOwner()->SetLocalPosition(ezVec3(0, 0, curHeight));
}

void SampleBounceComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  if (OWNTYPE::GetStaticRTTI()->GetTypeVersion() == 1)
  {
    // this automatically serializes all properties
    // if you need more control, increase the component 'version' at the top of this file
    // and then use the code path below for manual serialization
    ezReflectionSerializer::WriteObjectToBinary(s, GetDynamicRTTI(), this);
  }
  else
  {
    // do custom serialization, for example:
    // s << m_fAmplitude;
    // s << m_Speed;
  }
}

void SampleBounceComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  if (uiVersion == 1)
  {
    // this automatically de-serializes all properties
    // if you need more control, increase the component 'version' at the top of this file
    // and then use the code path below for manual de-serialization
    ezReflectionSerializer::ReadObjectPropertiesFromBinary(s, *GetDynamicRTTI(), this);
  }
  else
  {
    // do custom de-serialization, for example:
    // s >> m_fAmplitude;
    // s >> m_Speed;
  }
}
