#pragma once

#include <Core/World/WorldModule.h>
#include <GameEngine/Basics.h>

class EZ_GAMEENGINE_DLL ezWindWorldModuleInterface : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezWindWorldModuleInterface, ezWorldModule);

public:
  ezWindWorldModuleInterface(ezWorld* pWorld);
  ~ezWindWorldModuleInterface();

  virtual ezVec3 GetWindAt(const ezVec3& vPosition);
  virtual void SetFallbackWind(const ezVec3& vWind);

private:
  ezVec3 m_vFallbackWind;
};
