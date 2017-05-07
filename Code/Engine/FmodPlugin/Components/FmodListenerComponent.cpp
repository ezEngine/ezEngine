#include <PCH.h>
#include <FmodPlugin/Components/FmodListenerComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/FmodIncludes.h>

EZ_BEGIN_COMPONENT_TYPE(ezFmodListenerComponent, 1)
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

  FMOD_3D_ATTRIBUTES attr;
  attr.position.x = pos.x;
  attr.position.y = pos.y;
  attr.position.z = pos.z;
  attr.forward.x = fwd.x;
  attr.forward.y = fwd.y;
  attr.forward.z = fwd.z;
  attr.up.x = up.x;
  attr.up.y = up.y;
  attr.up.z = up.z;
  attr.velocity.x = vel.x;
  attr.velocity.y = vel.y;
  attr.velocity.z = vel.z;

  ezFmod::GetSingleton()->GetStudioSystem()->setListenerAttributes(m_uiListenerIndex, &attr);
}



EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_Components_FmodListenerComponent);

