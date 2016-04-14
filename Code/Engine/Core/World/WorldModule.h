#pragma once

#include <Core/Basics.h>
#include <Foundation/Reflection/Implementation/DynamicRTTI.h>

class ezWorld;

class EZ_CORE_DLL ezWorldModule : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezWorldModule, ezReflectedClass);

public:
  ezWorldModule();
  ~ezWorldModule();

  void Startup(ezWorld* pOwner);
  void Shutdown();
  void Update();
  void Reinit();

  ezWorld* GetWorld() const { return m_pOwnerWorld; }

  /// \brief Searches for the world module of the given type, that is responsible for the given world.
  static ezWorldModule* FindModule(const ezWorld* pWorld, const ezRTTI* pWorldModuleType);

protected:
  virtual void InternalStartup() = 0;
  virtual void InternalShutdown() = 0;
  virtual void InternalUpdate() = 0;
  virtual void InternalReinit() = 0;

  ezWorld* m_pOwnerWorld;

private:
  static ezDynamicArray<ezWorldModule*> s_AllWorldModules;
};

