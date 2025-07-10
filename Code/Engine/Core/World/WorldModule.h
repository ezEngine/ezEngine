#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/HashedString.h>

class ezWorld;

struct ezWorldUpdatePhase
{
  using StorageType = ezUInt8;

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
  using UpdateFunction = ezDelegate<void(const UpdateContext&)>;

  /// \brief Description of an update function that can be registered at the world.
  struct UpdateFunctionDesc
  {
    UpdateFunctionDesc(const UpdateFunction& function, ezStringView sFunctionName)
      : m_Function(function)
    {
      m_sFunctionName.Assign(sFunctionName);
    }

    UpdateFunction m_Function;                    ///< Delegate to the actual update function.
    ezHashedString m_sFunctionName;               ///< Name of the function. Use the EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC macro to create a description
                                                  ///< with the correct name.
    ezHybridArray<ezHashedString, 4> m_DependsOn; ///< Array of other functions on which this function depends on. This function will be
                                                  ///< called after all its dependencies have been called.
    ezEnum<ezWorldUpdatePhase> m_Phase;           ///< The update phase in which this update function should be called. See ezWorld for a description on the different phases.
    bool m_bOnlyUpdateWhenSimulating = false;     ///< The update function is only called when the world simulation is enabled.
    ezUInt16 m_uiAsyncPhaseBatchSize = 0;         ///< 0 means m_Function is called once per frame, to update all components, but still in parallel with other world modules.
                                                  ///< >0 means m_Function is called multiple times (in parallel) with batches of roughly this size.
    float m_fPriority = 0.0f;                     ///< Higher priority (higher number) means that this function is called earlier than a function with lower priority.
  };

  /// \brief Registers the given update function at the world.
  void RegisterUpdateFunction(const UpdateFunctionDesc& desc);

  /// \brief De-registers the given update function from the world. Note that only the m_Function and the m_Phase of the description have to
  /// be valid for de-registration.
  void DeregisterUpdateFunction(const UpdateFunctionDesc& desc);

  /// \brief Returns the allocator used by the world.
  ezAllocator* GetAllocator();

  /// \brief Returns the block allocator used by the world.
  ezInternal::WorldLargeBlockAllocator* GetBlockAllocator();

  /// \brief Returns whether the world simulation is enabled.
  bool GetWorldSimulationEnabled() const;

protected:
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
  ezWorldModule* CreateWorldModule(ezUInt16 uiTypeId, ezWorld* pWorld);

  /// \brief Register explicit a mapping of a world module interface to a specific implementation.
  ///
  /// This is necessary if there are multiple implementations of the same interface.
  /// If there is only one implementation for an interface this implementation is registered automatically.
  void RegisterInterfaceImplementation(ezStringView sInterfaceName, ezStringView sImplementationName);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, WorldModuleFactory);

  using CreatorFunc = ezWorldModule* (*)(ezAllocator*, ezWorld*);

  ezWorldModuleFactory();
  ezWorldModuleTypeId RegisterWorldModule(const ezRTTI* pRtti, CreatorFunc creatorFunc);

  static void PluginEventHandler(const ezPluginEvent& EventData);
  void FillBaseTypeIds();
  void ClearUnloadedTypeToIDs();
  void AdjustBaseTypeId(const ezRTTI* pParentRtti, const ezRTTI* pRtti, ezUInt16 uiParentTypeId);

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
#define EZ_DECLARE_WORLD_MODULE()                      \
public:                                                \
  static EZ_ALWAYS_INLINE ezWorldModuleTypeId TypeId() \
  {                                                    \
    return s_TypeId;                                   \
  }                                                    \
                                                       \
private:                                               \
  static ezWorldModuleTypeId s_TypeId;

/// \brief Implements the given module type. Add this macro to a cpp outside of the type declaration.
#define EZ_IMPLEMENT_WORLD_MODULE(moduleType) \
  ezWorldModuleTypeId moduleType::s_TypeId = ezWorldModuleFactory::GetInstance()->RegisterWorldModule<moduleType, moduleType>();

/// \brief Helper macro to create an update function description with proper name
#define EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(func, instance) ezWorldModule::UpdateFunctionDesc(ezWorldModule::UpdateFunction(&func, instance), #func)

#include <Core/World/Implementation/WorldModule_inl.h>
