#pragma once

#include <PhysXPlugin/Basics.h>
#include <Foundation/Configuration/Plugin.h>
#include <Core/Scene/SceneModule.h>

class EZ_PHYSXPLUGIN_DLL ezPhysXSceneModule : public ezSceneModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPhysXSceneModule, ezSceneModule);

public:
  ezPhysXSceneModule();

protected:
  virtual void InternalStartup() override;

  virtual void InternalShutdown() override;

  virtual void InternalUpdate() override;

};


EZ_DYNAMIC_PLUGIN_DECLARATION(EZ_PHYSXPLUGIN_DLL, ezPhysXPlugin);