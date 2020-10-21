#pragma once

#include <Core/World/WorldModule.h>

class EZ_CORE_DLL ezWindWorldModuleInterface : public ezWorldModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezWindWorldModuleInterface, ezWorldModule);

protected:
  ezWindWorldModuleInterface(ezWorld* pWorld);

public:
  virtual ezVec3 GetWindAt(const ezVec3& vPosition) = 0;
};
