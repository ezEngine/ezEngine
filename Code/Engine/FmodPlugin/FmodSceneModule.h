#pragma once

#include <FmodPlugin/Basics.h>
#include <FmodPlugin/PluginInterface.h>
#include <FmodPlugin/FmodSingleton.h>
#include <Foundation/Configuration/Plugin.h>
#include <Core/Scene/SceneModule.h>

class EZ_FMODPLUGIN_DLL ezFmodSceneModule : public ezSceneModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFmodSceneModule, ezSceneModule);

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

