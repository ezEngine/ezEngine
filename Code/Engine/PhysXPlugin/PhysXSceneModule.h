#pragma once

#include <PhysXPlugin/Basics.h>
#include <Foundation/Configuration/Plugin.h>
#include <Core/Scene/SceneModule.h>

struct ezPhysXData;

class EZ_PHYSXPLUGIN_DLL ezPhysXSceneModule : public ezSceneModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPhysXSceneModule, ezSceneModule);

public:
  ezPhysXSceneModule();

protected:
  virtual void InternalStartup() override;

  virtual void InternalShutdown() override;

  virtual void InternalUpdate() override;

private:
  static void InitializePhysX();
  static void DeinitializePhysX();

  static ezPhysXData* s_pPhysXData;

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(PhysX, PhysXPlugin);
};


EZ_DYNAMIC_PLUGIN_DECLARATION(EZ_PHYSXPLUGIN_DLL, ezPhysXPlugin);