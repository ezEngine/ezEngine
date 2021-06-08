#include <SampleGamePluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <SampleGamePlugin/Components/DemoComponent.h>

// BEGIN-DOCS-CODE-SNIPPET: customcomp-reflection
// clang-format off
// BEGIN-DOCS-CODE-SNIPPET: component-reflection
EZ_BEGIN_COMPONENT_TYPE(DemoComponent, 3 /* version */, ezComponentMode::Dynamic)
// END-DOCS-CODE-SNIPPET
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Amplitude", m_fAmplitude)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(0, 10)),
    EZ_MEMBER_PROPERTY("Speed", m_Speed)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(90))),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("SampleGamePlugin"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on
// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: customcomp-basics
DemoComponent::DemoComponent() = default;
DemoComponent::~DemoComponent() = default;

void DemoComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // this component doesn't need to anything for initialization
}

void DemoComponent::Update()
{
  const ezTime curTime = GetWorld()->GetClock().GetAccumulatedTime();
  const ezAngle curAngle = curTime.AsFloatInSeconds() * m_Speed;
  const float curHeight = ezMath::Sin(curAngle) * m_fAmplitude;

  GetOwner()->SetLocalPosition(ezVec3(0, 0, curHeight));
}

// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: component-serialize
void DemoComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fAmplitude;
  s << m_Speed;
}
// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: component-deserialize
void DemoComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fAmplitude;

  if (uiVersion <= 2)
  {
    // up to version 2 the angle was stored as a float in degree
    // convert this to ezAngle
    float fDegree;
    s >> fDegree;
    m_Speed = ezAngle::Degree(fDegree);
  }
  else
  {
    s >> m_Speed;
  }
}
// END-DOCS-CODE-SNIPPET
