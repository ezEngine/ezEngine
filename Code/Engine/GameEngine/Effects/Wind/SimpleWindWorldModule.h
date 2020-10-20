#pragma once

#include <Core/Interfaces/WindWorldModule.h>

class EZ_GAMEENGINE_DLL ezSimpleWindWorldModule : public ezWindWorldModuleInterface
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezSimpleWindWorldModule, ezWindWorldModuleInterface);

public:
  ezSimpleWindWorldModule(ezWorld* pWorld);
  ~ezSimpleWindWorldModule();

  virtual ezVec3 GetWindAt(const ezVec3& vPosition) override;

  void SetFallbackWind(const ezVec3& vWind);

private:
  ezVec3 m_vFallbackWind;
};
