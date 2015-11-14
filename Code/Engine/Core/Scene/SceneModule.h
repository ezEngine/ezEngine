#pragma once

#include <Core/Basics.h>
#include <Foundation/Reflection/Implementation/DynamicRTTI.h>

class EZ_CORE_DLL ezSceneModule : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneModule, ezReflectedClass);

public:

  void Startup() { InternalStartup(); }
  void Shutdown() { InternalShutdown(); }
  void Update() { InternalUpdate(); }

protected:
  virtual void InternalStartup() = 0;
  virtual void InternalShutdown() = 0;
  virtual void InternalUpdate() = 0;

private:

};

