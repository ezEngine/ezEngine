#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <Core/ActorSystem/ActorPlugin.h>
#include <Foundation/Communication/Event.h>

class ezActor2;
struct ezActor2Impl;

class EZ_CORE_DLL ezActor2 : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActor2, ezReflectedClass);

public:
  ezActor2(const char* szActorName, const void* pCreatedBy);
  ~ezActor2();

  /// \brief Returns the name of this actor
  const char* GetName() const;

  /// \brief Returns the 'created by' pointer of the actor
  const void* GetCreatedBy() const;

  /// \brief Transfers ownership of the ezActorDevice to the ezActor
  void AddPlugin(ezUniquePtr<ezActorPlugin>&& pDevice);

  /// \brief Queries the ezActor for an ezActorDevice of the given type. Returns null if no such devices was added to the actor.
  ezActorPlugin* GetPlugin(const ezRTTI* pDeviceType) const;

  /// \brief Templated overload of GetDevice() that automatically casts to the desired class type.
  template <typename ActorPluginType>
  ActorPluginType* GetPlugin() const
  {
    return static_cast<ActorPluginType*>(GetPlugin(ezGetStaticRTTI<ActorPluginType>()));
  }

  // TODO: RemovePlugin

  /// \brief Fills the list with all devices that have been added to the actor.
  void GetAllPlugins(ezHybridArray<ezActorPlugin*, 8>& out_AllDevices);

private: // directly touched by ezActorManager2
  friend class ezActorManager2;

  virtual void Update();

private:
  ezUniquePtr<ezActor2Impl> m_pImpl;
};
