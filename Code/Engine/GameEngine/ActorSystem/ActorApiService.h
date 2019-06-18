#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Reflection/Reflection.h>

class EZ_GAMEENGINE_DLL ezActorApiService: public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorApiService, ezReflectedClass);

public:
  ezActorApiService();
  ~ezActorApiService();

protected:
  virtual void Activate() = 0;
  virtual void Update() = 0;

private: // directly accessed by ezActorManager
  friend class ezActorManager;

  bool m_bActivated = false;
};
