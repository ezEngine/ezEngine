#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/ActorSystem/ActorPlugin.h>
#include <Foundation/Types/UniquePtr.h>

struct ezActorImpl;

class EZ_GAMEENGINE_DLL ezActor : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActor, ezReflectedClass);

  EZ_DISALLOW_COPY_AND_ASSIGN(ezActor);

public:
  ezActor(const char* szActorName, const void* pCreatedBy);
  ~ezActor();

  /// \brief Returns the name of this actor
  const char* GetName() const;

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

  /// \brief Deletes the given plugin from the actor
  void DestroyPlugin(ezActorPlugin* pPlugin);

  /// \brief Fills the list with all plugins that have been added to the actor.
  void GetAllPlugins(ezHybridArray<ezActorPlugin*, 8>& out_AllPlugins);

protected:
  void UpdateAllPlugins();

 
protected: // directly touched by ezActorManager
  friend class ezActorManager;

  /// \brief Called shortly before the first call to Update()
  virtual void Activate();

  /// \brief Called once per frame to update the actor state.
  ///
  /// By default this calls UpdateAllPlugins() internally.
  virtual void Update();

private: // directly touched by ezActorManager

  enum class State
  {
    New,
    Active,
    QueuedForDestruction
  };

  State m_State = State::New;

private:
  ezUniquePtr<ezActorImpl> m_pImpl;
};
