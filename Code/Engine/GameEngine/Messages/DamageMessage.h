#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Communication/Message.h>

struct EZ_GAMEENGINE_DLL ezMsgDamage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgDamage, ezMessage);

  double m_fDamage = 0;
  ezString m_sHitObjectName; ///< The actual game object that was hit (may be a child of the object to which the message is sent)

  ezVec3 m_vGlobalPosition;  ///< The global position at which the damage was applied. Set to zero, if unused.
  ezVec3 m_vImpactDirection; ///< The direction into which the damage was applied (e.g. direction of a projectile). May be zero.
};
