#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Reflection/Reflection.h>

class ezActor;

class EZ_CORE_DLL ezActorDevice : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorDevice, ezReflectedClass);

public:
  ezActorDevice();
  ~ezActorDevice();

  ezActor* GetActor() const;

private:
  friend class ezActor;

  ezActor* m_pOwningActor = nullptr;
};
