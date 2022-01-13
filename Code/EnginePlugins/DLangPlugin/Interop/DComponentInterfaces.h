#pragma once

#include <DLangPlugin/DLangPluginDLL.h>

#include <Core/World/Component.h>

class ezGameObject;

class EZ_DLANGPLUGIN_DLL ezDLangBaseComponent
{
public:
  //virtual ezGameObject* GetOwner() = 0;
  // virtual ezWorld* GetWorld();
  // virtual ezComponentHandle GetHandle() const;
  // virtual void PostMessage(const ezMessage& msg, ezTime delay = ezTime::Zero(), ezObjectMsgQueueType::Enum queueType = ezObjectMsgQueueType::NextFrame) const;
  //virtual void OnActivated() = 0;
  //virtual void OnDeactivated() = 0;

  virtual void OnSimulationStarted() = 0;
  virtual void Update() = 0;
};
