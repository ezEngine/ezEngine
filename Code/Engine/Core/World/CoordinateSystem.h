#pragma once

#include <Foundation/Math/Vec3.h>

#include <Core/World/Declarations.h>

struct EZ_CORE_DLL ezCoordinateSystem
{
  EZ_DECLARE_POD_TYPE();

  ezVec3 m_vForwardDir;
  ezVec3 m_vRightDir;
  ezVec3 m_vUpDir;
};

class EZ_CORE_DLL ezCoordinateSystemProvider
{
public:
  ezCoordinateSystemProvider(const ezWorld* pOwnerWorld) : m_pOwnerWorld(pOwnerWorld) 
  {
  }

  virtual ~ezCoordinateSystemProvider()
  {
  }

  virtual void GetCoordinateSystem(const ezVec3& vGlobalPosition, ezCoordinateSystem& out_CoordinateSystem) const = 0;

protected:
  friend class ezWorld;

  const ezWorld* m_pOwnerWorld;
};
