#pragma once

#include <GameUtils/Basics.h>
#include <Core/World/WorldModule.h>

class ezGameObjectHandle;

class EZ_GAMEUTILS_DLL ezPhysicsWorldModuleInterface : public ezWorldModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPhysicsWorldModuleInterface, ezWorldModule);

public:

  virtual bool CastRay(const ezVec3& vStart, const ezVec3& vDir, float fMaxLen, ezUInt8 uiCollisionLayer, ezVec3& out_vHitPos, ezVec3& out_vHitNormal, ezGameObjectHandle& out_hHitGameObject) = 0;

private:

};