#pragma once

#include <FmodPlugin/Basics.h>
#include <FmodPlugin/PluginInterface.h>
#include <FmodPlugin/FmodSingleton.h>
#include <Foundation/Configuration/Plugin.h>
#include <Core/World/WorldModule.h>

class EZ_FMODPLUGIN_DLL ezFmodSceneModule : public ezWorldModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFmodSceneModule, ezWorldModule);

public:
  ezFmodSceneModule() {}



protected:
  virtual void InternalStartup() override;
  virtual void InternalShutdown() override;
  virtual void InternalUpdate() override;
  virtual void InternalReinit() override;

private:

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Fmod, FmodPlugin);
};

