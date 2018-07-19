#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/ArrayPtr.h>

struct EZ_PARTICLEPLUGIN_DLL ezParticleEvent
{
  EZ_DECLARE_POD_TYPE();

  ezTempHashedString m_EventType;
  ezVec3 m_vPosition;
  ezVec3 m_vDirection;
  ezVec3 m_vNormal;
};

typedef ezArrayPtr<ezParticleEvent> ezParticleEventQueue;
