#include <PCH.h>
#include <EnginePluginTest/Components.h>

EZ_BEGIN_COMPONENT_TYPE(ShipComponent, ezComponent, 1, ShipComponentManager);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Health", m_iHealth),
    EZ_MEMBER_PROPERTY("ShootDelay", m_iShootDelay),
    EZ_MEMBER_PROPERTY("Max Ammo", m_iMaxAmmunition),
    EZ_MEMBER_PROPERTY("Ammo Per Shot", m_iAmmoPerShot),
  EZ_END_PROPERTIES
EZ_END_COMPONENT_TYPE();

