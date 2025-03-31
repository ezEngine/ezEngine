#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Messages/DamageMessage.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgDamage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgDamage, 1, ezRTTIDefaultAllocator<ezMsgDamage>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Damage", m_fDamage),
    EZ_MEMBER_PROPERTY("HitObjectName", m_sHitObjectName),
    EZ_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    EZ_MEMBER_PROPERTY("ImpactDirection", m_vImpactDirection),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

EZ_STATICLINK_FILE(GameEngine, GameEngine_Messages_Implementation_DamageMessage);
