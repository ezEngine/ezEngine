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

protected:
  friend class ezWorld;
  friend class ezInternal::WorldData;
  friend class ezMemoryUtils;

  struct UpdateContext
  {
    ezUInt32 m_uiFirstComponentIndex;
    ezUInt32 m_uiComponentCount;
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
      m_Phase = Phase::PreAsync;
      m_bOnlyUpdateWhenSimulating = false;
      m_uiGranularity = 0;
      m_fPriority = 0.0f;
    }

    UpdateFunction m_Function;      ///< Delegate to the actual update function.
    ezHashedString m_sFunctionName; ///< Name of the function. Use the EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC macro to create a description
                                    ///< with the correct name.
    ezHybridArray<ezHashedString, 4> m_DependsOn; ///< Array of other functions on which this function depends on. This function will be
                                                  ///< called after all its dependencies have been called.
    ezEnum<Phase> m_Phase; ///< The update phase in which this update function should be called. See ezWorld for a description on the
                           ///< different phases.
    bool m_bOnlyUpdateWhenSimulating; ///< The update function is only called when the world simulation is enabled.
    ezUInt16 m_uiGranularity; ///< The granularity in which batch updates should happen during the asynchronous phase. Has to be 0 for
                              ///< synchronous functions.
    float m_fPriority; ///< Higher priority (higher number) means that this function is called earlier than a function with lower priority.
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

  ezWorld* m_pWorld;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Helper class to get component type ids and create new instances of world modules from rtti.
class EZ_CORE_DLL ezWorldModuleFactory
{
public:
  static ezWorldModuleFactory* GetInstance();

  template <typename ModuleType, typename RTTIType>
  ezUInt16 RegisterWorldModule();

  /// \brief Returns the module type id to the given rtti module/component type.
  ezUInt16 GetTypeId(const ezRTTI* pRtti);

  /// \brief Creates a new instance of the world module with the given type id and world.
  ezWorldModule* CreateWorldModule(ezUInt16 typeId, ezWorld* pWorld);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, WorldModuleFactory);

  typedef ezWorldModule* (*CreatorFunc)(ezAllocatorBase*, ezWorld*);

  ezWorldModuleFactory();
  ezUInt16 RegisterWorldModule(const ezRTTI* pRtti, CreatorFunc creatorFunc);

  static void PluginEventHandler(const ezPlugin::PluginEvent& EventData);
  void FillBaseTypeIds();
  void ClearUnloadedTypeToIDs();

  ezHashTable<const ezRTTI*, ezUInt16> m_TypeToId;

  ezDynamicArray<CreatorFunc> m_CreatorFuncs;
};

/// \brief Add this macro to the declaration of your module type.
#define EZ_DECLARE_WORLD_MODULE()                                                                                                          \
public:                                                                                                                                    \
  static EZ_ALWAYS_INLINE ezUInt16 TypeId() { return TYPE_ID; }                                                                            \
                                                                                                                                           \
private:                                                                                                                                   \
  static ezUInt16 TYPE_ID;

/// \brief Implements the given module type. Add this macro to a cpp outside of the type declaration.
#define EZ_IMPLEMENT_WORLD_MODULE(moduleType)                                                                                              \
  ezUInt16 moduleType::TYPE_ID = ezWorldModuleFactory::GetInstance()->RegisterWorldModule<moduleType, moduleType>();

/// \brief Helper macro to create an update function description with proper name
#define EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(func, instance)                                                                              \
  ezWorldModule::UpdateFunctionDesc(ezWorldModule::UpdateFunction(&func, instance), #func)

#include <Core/World/Implementation/WorldModule_inl.h>
