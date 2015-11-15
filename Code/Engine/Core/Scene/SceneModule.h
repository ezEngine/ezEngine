#pragma once

#include <Core/Basics.h>
#include <Foundation/Reflection/Implementation/DynamicRTTI.h>

class ezScene;
class ezWorld;

class EZ_CORE_DLL ezSceneModule : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneModule, ezReflectedClass);

public:

  void Startup(ezScene* pOwner) { m_pOwnerScene = pOwner; InternalStartup(); }
  void Shutdown() { InternalShutdown(); }
  void Update() { InternalUpdate(); }

protected:
  virtual void InternalStartup() = 0;
  virtual void InternalShutdown() = 0;
  virtual void InternalUpdate() = 0;

  ezScene* m_pOwnerScene;
  ezWorld* GetWorld() const;

private:

};

