#include <PCH.h>
#include <FmodPlugin/Components/FmodListenerComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/FmodIncludes.h>

EZ_BEGIN_COMPONENT_TYPE(ezFmodListenerComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ListenerIndex", m_uiListenerIndex),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezFmodListenerComponent::ezFmodListenerComponent()
{
  m_uiListenerIndex = 0;
}


void ezFmodListenerComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_uiListenerIndex;
}


void ezFmodListenerComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = stream.GetStream();

  s >> m_uiListenerIndex;
}

void ezFmodListenerComponent::Update()
{
  const auto pos = GetOwner()->GetGlobalPosition();
  const auto vel = GetOwner()->GetVelocity();
  const auto fwd = (GetOwner()->GetGlobalRotation() * ezVec3(1, 0, 0)).GetNormalized();
  const auto up  = (GetOwner()->GetGlobalRotation() * ezVec3(0, 0, 1)).GetNormalized();

  ezFmod::GetSingleton()->SetListener(m_uiListenerIndex, pos, fwd, up, vel);
}



EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_Components_FmodListenerComponent);

