#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Reflection/Reflection.h>

class ezActor;

class EZ_GAMEENGINE_DLL ezActorPlugin : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorPlugin, ezReflectedClass);

public:
  ezActorPlugin();
  ~ezActorPlugin();

  ezActor* GetActor() const;

protected:
  friend class ezActor;
  virtual void Update() = 0;

private:
  ezActor* m_pOwningActor = nullptr;
};
