#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/HashedString.h>

class ezWorld;

class EZ_CORE_DLL ezWorldModule : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezWorldModule, ezReflectedClass);

protected:
  ezWorldModule(ezWorld* pWorld);
  virtual ~ezWorldModule();

public:
  /// \brief Returns the corresponding world to this module.
  ezWorld* GetWorld();

  /// \brief Returns the corresponding world to this module.
  const ezWorld* GetWorld() const;

  /// \brief Same as GetWorld()->GetIndex(). Needed to break circular include dependencies.
  ezUInt32 GetWorldIndex() const;

protected:
  friend class ezWorld;
  friend class ezInternal::WorldData;
  friend class ezMemoryUtils;

  struct UpdateContext
  {
    ezUInt32 m_uiFirstComponentIndex = 0;
    ezUInt32 m_uiComponentCount = 0;
  };

  /// \brief Update function delegate.
  typedef ezDelegate<void(const UpdateContext&)> UpdateFunction;

  /// \brief Description of an update function that can be registered at the world.
  struct UpdateFunctionDesc
  {
    struct Phase
    {
      typedef ezUInt8 StorageType;

      enum Enum
      {
        PreAsync,
        Async,
        PostAsync,
        PostTransform,
        COUNT,

        Default = PreAsync
      };
    };

    UpdateFunctionDesc(const UpdateFunction& function, const char* szFunctionName)
    {
      m_Function = function;
      m_sFunctionName.Assign(szFunctionName);
    }

    UpdateFunction m_Function;                    ///< Delegate to the actual update function.
    ezHashedString m_sFunctionName;               ///< Name of the function. Use the EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC macro to create a description
                                                  ///< with the correct name.
    ezHybridArray<ezHashedString, 4> m_DependsOn; ///< Array of other functions on which this function depends on. This function will be
                                                  ///< called after all its dependencies have been called.
    ezEnum<Phase> m_Phase;                        ///< The update phase in which this update function should be called. See ezWorld for a description on the
                                                  ///< different phases.
    bool m_bOnlyUpdateWhenSimulating = false;     ///< The update function is only called when the world simulation is enabled.
    ezUInt16 m_uiGranularity = 0;                 ///< The granularity in which batch updates should happen during the asynchronous phase. Has to be 0 for
                                                  ///< synchronous functions.
    float m_fPriority = 0.0f;                     ///< Higher priority (higher number) means that this function is called earlier than a function with lower priority.
  };

  /// \brief Registers the given update function at the world.
  void RegisterUpdateFunction(const UpdateFunctionDesc& desc);

  /// \brief De-registers the given update function from the world. Note that only the m_Function and the m_Phase of the description have to
  /// be valid for de-registration.
  void DeregisterUpdateFunction(const UpdateFunctionDesc& desc);

  /// \brief Returns the allocator used by the world.
  ezAllocatorBase* GetAllocator();

  /// \brief Returns the block allocator used by the world.
  ezInternal::WorldLargeBlockAllocator* GetBlockAllocator();

  /// \brief Returns whether the world simulation is enabled.
  bool GetWorldSimulationEnabled() const;

protected:
  /// \brief Internal methods called by the world
  virtual void InitializeInternal();
  virtual void DeinitializeInternal();

  /// \brief This method is called after the constructor. A derived type can override this method to do initialization work. Typically this
  /// is the method where updates function are registered.
  virtual void Initialize() {}

  /// \brief This method is called before the destructor. A derived type can override this method to do deinitialization work.
  virtual void Deinitialize() {}

  /// \brief This method is called at the start of the next world update when the world is simulated. This method will be called after the
  /// initialization method.
  virtual void OnSimulationStarted() {}

  /// \brief Called by ezWorld::Clear(). Can be used to clear cached data when a world is completely cleared of objects (but not deleted).
  virtual void WorldClear() {}

  ezWorld* m_pWorld;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Helper class to get component type ids and create new instances of world modules from rtti.
class EZ_CORE_DLL ezWorldModuleFactory
{
public:
  static ezWorldModuleFactory* GetInstance();

  template <typename ModuleType, typename RTTIType>
  ezWorldModuleTypeId RegisterWorldModule();

  /// \brief Returns the module type id to the given rtti module/component type.
  ezWorldModuleTypeId GetTypeId(const ezRTTI* pRtti);

  /// \brief Creates a new instance of the world module with the given type id and world.
  ezWorldModule* CreateWorldModule(ezUInt16 typeId, ezWorld* pWorld);

  /// \brief Register explicit a mapping of a world module interface to a specific implementation.
  ///
  /// This is necessary if there are multiple implementations of the same interface.
  /// If there is only one implementation for an interface this implementation is registered automatically.
  void RegisterInterfaceImplementation(ezStringView sInterfaceName, ezStringView sImplementationName);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, WorldModuleFactory);

  typedef ezWorldModule* (*CreatorFunc)(ezAllocatorBase*, ezWorld*);

  ezWorldModuleFactory();
  ezWorldModuleTypeId RegisterWorldModule(const ezRTTI* pRtti, CreatorFunc creatorFunc);

  static void PluginEventHandler(const ezPluginEvent& EventData);
  void FillBaseTypeIds();
  void ClearUnloadedTypeToIDs();

  ezHashTable<const ezRTTI*, ezWorldModuleTypeId> m_TypeToId;

  struct CreatorFuncContext
  {
    EZ_DECLARE_POD_TYPE();

    CreatorFunc m_Func;
    const ezRTTI* m_pRtti;
  };

  ezDynamicArray<CreatorFuncContext> m_CreatorFuncs;

  ezHashTable<ezString, ezString> m_InterfaceImplementations;
};

/// \brief Add this macro to the declaration of your module type.
#define EZ_DECLARE_WORLD_MODULE()                                          \
public:                                                                    \
  static EZ_ALWAYS_INLINE ezWorldModuleTypeId TypeId() { return TYPE_ID; } \
                                                                           \
private:                                                                   \
  static ezWorldModuleTypeId TYPE_ID;

/// \brief Implements the given module type. Add this macro to a cpp outside of the type declaration.
#define EZ_IMPLEMENT_WORLD_MODULE(moduleType) \
  ezWorldModuleTypeId moduleType::TYPE_ID = ezWorldModuleFactory::GetInstance()->RegisterWorldModule<moduleType, moduleType>();

/// \brief Helper macro to create an update function description with proper name
#define EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(func, instance) \
  ezWorldModule::UpdateFunctionDesc(ezWorldModule::UpdateFunction(&func, instance), #func)

#include <Core/World/Implementation/WorldModule_inl.h>
