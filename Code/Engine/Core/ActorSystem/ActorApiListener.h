#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Reflection/Reflection.h>

class EZ_CORE_DLL ezActorApiListener: public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorApiListener, ezReflectedClass);

public:
  ezActorApiListener();
  ~ezActorApiListener();

protected:
  virtual void Activate() = 0;
  virtual void Update() = 0;

private: // directly by ezActorManager2
  friend class ezActorManager2;

  bool m_bActivated = false;
};
