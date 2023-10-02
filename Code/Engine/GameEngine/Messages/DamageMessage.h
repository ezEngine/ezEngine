#pragma once

#include <Core/Messages/EventMessage.h>
#include <GameEngine/GameEngineDLL.h>

struct EZ_GAMEENGINE_DLL ezMsgDamage : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgDamage, ezEventMessage);

  double m_fDamage = 0;
  ezString m_sHitObjectName; ///< The actual game object that was hit (may be a child of the object to which the message is sent)

  ezVec3 m_vGlobalPosition;  ///< The global position at which the damage was applied. Set to zero, if unused.
  ezVec3 m_vImpactDirection; ///< The direction into which the damage was applied (e.g. direction of a projectile). May be zero.
};
