#include <PCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <SampleGamePlugin/Components/DemoComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(DemoComponent, 2, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(0, 10)),
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(90)),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("SampleGame"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

DemoComponent::DemoComponent() {}

void DemoComponent::OnSimulationStarted() {}


void DemoComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fHeight;
  s << m_fSpeed;
}


void DemoComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fHeight;
  s >> m_fSpeed;
}

void DemoComponent::Update()
{
  const ezWorld* pWorld = GetWorld();
  const ezClock& clock = pWorld->GetClock();
  const ezTime currentTime = clock.GetAccumulatedTime();

  const ezAngle angle = ezAngle::Degree(static_cast<float>(currentTime.GetSeconds()) * m_fSpeed);

  const float curHeight = ezMath::Sin(angle) * m_fHeight;

  GetOwner()->SetLocalPosition(ezVec3(0, 0, curHeight));
}
