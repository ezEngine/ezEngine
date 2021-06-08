#pragma once

#include <Core/World/WorldModule.h>

struct EZ_CORE_DLL ezWindStrength
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Calm,
    LightBreeze,
    GentleBreeze,
    ModerateBreeze,
    StrongBreeze,
    Storm,

    Default = LightBreeze
  };

  static float GetInMetersPerSecond(ezWindStrength::Enum strength);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezWindStrength);

class EZ_CORE_DLL ezWindWorldModuleInterface : public ezWorldModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezWindWorldModuleInterface, ezWorldModule);

protected:
  ezWindWorldModuleInterface(ezWorld* pWorld);

public:
  virtual ezVec3 GetWindAt(const ezVec3& vPosition) const = 0;
};
