#pragma once

#include <FmodPlugin/Basics.h>
#include <FmodPlugin/PluginInterface.h>
#include <FmodPlugin/FmodSingleton.h>
#include <Foundation/Configuration/Plugin.h>
#include <Core/World/WorldModule.h>

class EZ_FMODPLUGIN_DLL ezFmodSceneModule : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();

public:
  ezFmodSceneModule(ezWorld* pWorld);

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  void UpdateSound(const ezWorldModule::UpdateContext& context);

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Fmod, FmodPlugin);
};

