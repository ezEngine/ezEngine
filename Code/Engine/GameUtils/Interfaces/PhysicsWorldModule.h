#pragma once

#include <GameUtils/Basics.h>
#include <Core/World/WorldModule.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezGameObjectHandle;

typedef ezTypedResourceHandle<class ezSurfaceResource> ezSurfaceResourceHandle;

class EZ_GAMEUTILS_DLL ezPhysicsWorldModuleInterface : public ezWorldModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPhysicsWorldModuleInterface, ezWorldModule);

protected:
  ezPhysicsWorldModuleInterface(ezWorld* pWorld) : ezWorldModule(pWorld) { }

public:

  virtual bool CastRay(const ezVec3& vStart, const ezVec3& vDir, float fMaxLen, ezUInt8 uiCollisionLayer, ezVec3& out_vHitPos, ezVec3& out_vHitNormal, ezGameObjectHandle& out_hHitGameObject, ezSurfaceResourceHandle& out_hSurface) = 0;

  virtual bool SweepTestCapsule(const ezTransform& start, const ezVec3& vDir, float fCapsuleRadius, float fCapsuleHeight, float fDistance, ezUInt8 uiCollisionLayer, float& out_fDistance, ezVec3& out_Position, ezVec3& out_Normal) = 0;

  virtual ezVec3 GetGravity() const = 0;

private:

};
