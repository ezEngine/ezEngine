#pragma once

#include <Core/CoreDLL.h>

#include <Core/ActorSystem/ActorPlugin.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameApplication/WindowOutputTargetBase.h>
#include <System/Window/Window.h>

class ezActor;
struct ezActorImpl;

class EZ_CORE_DLL ezActor : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActor, ezReflectedClass);

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

  ezUniquePtr<ezWindowBase> m_pWindow;
  ezUniquePtr<ezWindowOutputTargetBase> m_pWindowOutputTarget;

protected:
  void UpdateAllPlugins();

private: // directly touched by ezActorManager
  friend class ezActorManager;

  virtual void Update();

private:
  ezUniquePtr<ezActorImpl> m_pImpl;
};
