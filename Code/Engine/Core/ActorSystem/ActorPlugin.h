#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Reflection/Reflection.h>

class ezActor2;

class EZ_CORE_DLL ezActorPlugin : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorPlugin, ezReflectedClass);

public:
  ezActorPlugin();
  ~ezActorPlugin();

  ezActor2* GetActor() const;

private:
  friend class ezActor2;

  ezActor2* m_pOwningActor = nullptr;
};
