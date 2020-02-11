#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Reflection/Reflection.h>

class EZ_GAMEENGINE_DLL ezActorApiService: public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorApiService, ezReflectedClass);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezActorApiService);

public:
  ezActorApiService();
  ~ezActorApiService();

protected:
  virtual void Activate() = 0;
  virtual void Update() = 0;

private: // directly accessed by ezActorManager
  friend class ezActorManager;

  enum class State
  {
    New,
    Active,
    QueuedForDestruction
  };

  State m_State = State::New;

};
