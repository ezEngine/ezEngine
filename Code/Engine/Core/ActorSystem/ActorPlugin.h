#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class ezActor;

/// \brief Base class for plugins that extend actor functionality.
///
/// Actor plugins are owned by actors and provide specific behaviors or capabilities.
/// They are updated each frame by the owning actor and can access the actor through GetActor().
/// Derived classes should override Update() to implement plugin-specific behavior.
class EZ_CORE_DLL ezActorPlugin : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorPlugin, ezReflectedClass);

public:
  ezActorPlugin();
  ~ezActorPlugin();

  ezActor* GetActor() const;

protected:
  friend class ezActor;
  virtual void Update() {}

private:
  ezActor* m_pOwningActor = nullptr;
};
