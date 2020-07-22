#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct EZ_CORE_DLL ezMsgCollision : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgCollision, ezMessage);

  ezGameObjectHandle m_hObjectA;
  ezGameObjectHandle m_hObjectB;

  ezComponentHandle m_hComponentA;
  ezComponentHandle m_hComponentB;

  ezVec3 m_vPosition; ///< The collision position in world space.
  ezVec3 m_vNormal;   ///< The collision normal on the surface of object B.
  ezVec3 m_vImpulse;  ///< The collision impulse applied from object A to object B.
};
