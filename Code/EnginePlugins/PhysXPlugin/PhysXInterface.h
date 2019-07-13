#pragma once

#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Enum.h>
#include <PhysXPlugin/PhysXPluginDLL.h>

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

//////////////////////////////////////////////////////////////////////////

struct ezOnPhysXContact
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    None = 0,
    SendReportMsg = EZ_BIT(0),
    ImpactReactions = EZ_BIT(1),
    SlideReactions = EZ_BIT(2),
    RollXReactions = EZ_BIT(3),
    RollYReactions = EZ_BIT(4),
    RollZReactions = EZ_BIT(5),

    Default = None
  };

  struct Bits
  {
    StorageType ContactReports : 1;
    StorageType ImpactReactions : 1;
    StorageType SlideReactions : 1;
    StorageType RollXReactions : 1;
    StorageType RollYReactions : 1;
    StorageType RollZReactions : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezOnPhysXContact);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_PHYSXPLUGIN_DLL, ezOnPhysXContact);

//////////////////////////////////////////////////////////////////////////

struct ezPxSettings
{
  ezPxSettings()
  {
    m_vObjectGravity.Set(0, 0, -9.81f);
    m_vCharacterGravity.Set(0, 0, -12.0f);
    m_fMaxDepenetrationVelocity = 1.0f;

    m_SteppingMode = ezPxSteppingMode::SemiFixed;
    m_fFixedFrameRate = 60.0f;
    m_uiMaxSubSteps = 4;

    m_uiScratchMemorySize = 256 * 1024;
  }

  ezVec3 m_vObjectGravity;
  ezVec3 m_vCharacterGravity;
  float m_fMaxDepenetrationVelocity;

  ezEnum<ezPxSteppingMode> m_SteppingMode;
  float m_fFixedFrameRate;
  ezUInt32 m_uiMaxSubSteps;

  ezUInt32 m_uiScratchMemorySize;
};
