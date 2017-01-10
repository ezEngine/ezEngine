#pragma once

#include <PhysXPlugin/Basics.h>

namespace physx
{
  class PxPhysics;
  class PxCooking;
}

class ezCollisionFilterConfig;

class ezPhysXInterface
{
public:

  virtual physx::PxPhysics* GetPhysXAPI() = 0;

  virtual ezCollisionFilterConfig& GetCollisionFilterConfig() = 0;

  virtual void LoadCollisionFilters() = 0;
};

struct EZ_PHYSXPLUGIN_DLL ezPxSteppingMode
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    Variable,
    Fixed,
    SemiFixed,

    Default = SemiFixed
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PHYSXPLUGIN_DLL, ezPxSteppingMode);

struct ezPxSettings
{
  ezPxSettings()
  {
    m_vObjectGravity.Set(0, 0, -9.81f);
    m_vCharacterGravity.Set(0, 0, -12.0f);

    m_SteppingMode = ezPxSteppingMode::SemiFixed;
    m_fFixedFrameRate = 60.0f;
    m_uiMaxSubSteps = 4;
  }

  ezVec3 m_vObjectGravity;
  ezVec3 m_vCharacterGravity;

  ezEnum<ezPxSteppingMode> m_SteppingMode;
  float m_fFixedFrameRate;
  ezUInt32 m_uiMaxSubSteps;
};

