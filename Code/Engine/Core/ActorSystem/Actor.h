#pragma once

#include <Core/ActorSystem/ActorPlugin.h>
#include <Foundation/Types/UniquePtr.h>

struct ezActorImpl;

class EZ_CORE_DLL ezActor final
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezActor);

public:
  ezActor(ezStringView sActorName, const void* pCreatedBy);
  ~ezActor();

  /// \brief Returns the name of this actor
  ezStringView GetName() const;

  /// \brief Returns the 'created by' pointer of the actor
  const void* GetCreatedBy() const;

  /// \brief Transfers ownership of the ezActorPlugin to the ezActor
  void AddPlugin(ezUniquePtr<ezActorPlugin>&& pPlugin);

  /// \brief Queries the ezActor for an ezActorPlugin of the given type. Returns null if no such plugin was added to the actor.
  ezActorPlugin* GetPlugin(const ezRTTI* pType) const;

  /// \brief Templated overload of GetPlugin() that automatically casts to the desired class type.
  template <typename Type>
  Type* GetPlugin() const
  {
    return static_cast<Type*>(GetPlugin(ezGetStaticRTTI<Type>()));
  }

private:
  friend class ezActorManager;

  /// \brief Called once per frame to update the actor state.
  void Update();

  bool m_bQueuedForDestruction = false;
  ezUniquePtr<ezActorImpl> m_pImpl;
};
