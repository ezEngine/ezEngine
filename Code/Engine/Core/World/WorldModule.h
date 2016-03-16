#pragma once

#include <Core/Basics.h>
#include <Foundation/Reflection/Implementation/DynamicRTTI.h>

class ezWorld;

class EZ_CORE_DLL ezWorldModule : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezWorldModule, ezReflectedClass);

public:

  void Startup(ezWorld* pOwner) { m_pOwnerWorld = pOwner; InternalStartup(); }
  void Shutdown() { InternalShutdown(); }
  void Update() { InternalUpdate(); }
  void Reinit() { InternalReinit(); }

  ezWorld* GetWorld() const { return m_pOwnerWorld; }

protected:
  virtual void InternalStartup() = 0;
  virtual void InternalShutdown() = 0;
  virtual void InternalUpdate() = 0;
  virtual void InternalReinit() = 0;

  ezWorld* m_pOwnerWorld;

private:

};

