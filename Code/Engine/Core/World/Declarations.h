#pragma once

#include <Foundation/Memory/BlockStorage.h>
#include <Foundation/Memory/LargeBlockAllocator.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Id.h>

#include <Core/CoreDLL.h>

#ifndef EZ_WORLD_INDEX_BITS
#  define EZ_WORLD_INDEX_BITS 8
#endif

#define EZ_MAX_WORLDS (1 << EZ_WORLD_INDEX_BITS)

class ezWorld;
class ezSpatialSystem;
class ezCoordinateSystemProvider;

namespace ezInternal
{
  class WorldData;

  enum
  {
    DEFAULT_BLOCK_SIZE = 1024 * 16
  };

  using WorldLargeBlockAllocator = ezLargeBlockAllocator<DEFAULT_BLOCK_SIZE>;
} // namespace ezInternal

class ezGameObject;
struct ezGameObjectDesc;

class ezComponentManagerBase;
class ezComponent;

struct ezMsgDeleteGameObject;

/// \brief Internal game object id used by ezGameObjectHandle.
struct ezGameObjectId
{
  using StorageType = ezUInt64;

  EZ_DECLARE_ID_TYPE(ezGameObjectId, 32, 8);

  static_assert(EZ_WORLD_INDEX_BITS > 0 && EZ_WORLD_INDEX_BITS <= 24);

  EZ_FORCE_INLINE ezGameObjectId(StorageType instanceIndex, ezUInt8 uiGeneration, ezUInt8 uiWorldIndex = 0)
  {
    m_Data = 0;
    m_InstanceIndex = static_cast<ezUInt32>(instanceIndex);
    m_Generation = uiGeneration;
    m_WorldIndex = uiWorldIndex;
  }

  union
  {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : 32;
      StorageType m_Generation : 8;
      StorageType m_WorldIndex : EZ_WORLD_INDEX_BITS;
    };
  };
};

/// \brief A handle to a game object.
///
/// Never store a direct pointer to a game object. Always store a handle instead. A pointer to a game object can
/// be received by calling ezWorld::TryGetObject with the handle.
/// Note that the object might have been deleted so always check the return value of TryGetObject.
struct ezGameObjectHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGameObjectHandle, ezGameObjectId);

  friend class ezWorld;
  friend class ezGameObject;
};

/// \brief HashHelper implementation so game object handles can be used as key in a hash table.
template <>
struct ezHashHelper<ezGameObjectHandle>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezGameObjectHandle value) { return ezHashHelper<ezUInt64>::Hash(value.GetInternalID().m_Data); }

  EZ_ALWAYS_INLINE static bool Equal(ezGameObjectHandle a, ezGameObjectHandle b) { return a == b; }
};

/// \brief Currently not implemented as it is not needed for game object handles.
EZ_CORE_DLL void operator<<(ezStreamWriter& inout_stream, const ezGameObjectHandle& hValue);
EZ_CORE_DLL void operator>>(ezStreamReader& inout_stream, ezGameObjectHandle& ref_hValue);

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezGameObjectHandle);
EZ_DECLARE_CUSTOM_VARIANT_TYPE(ezGameObjectHandle);
#define EZ_COMPONENT_TYPE_INDEX_BITS (24 - EZ_WORLD_INDEX_BITS)
#define EZ_MAX_COMPONENT_TYPES (1 << EZ_COMPONENT_TYPE_INDEX_BITS)

/// \brief Internal component id used by ezComponentHandle.
struct ezComponentId
{
  using StorageType = ezUInt64;

  EZ_DECLARE_ID_TYPE(ezComponentId, 32, 8);

  static_assert(EZ_COMPONENT_TYPE_INDEX_BITS > 0 && EZ_COMPONENT_TYPE_INDEX_BITS <= 16);

  EZ_ALWAYS_INLINE ezComponentId(StorageType instanceIndex, ezUInt8 uiGeneration, ezUInt16 uiTypeId = 0, ezUInt8 uiWorldIndex = 0)
  {
    m_Data = 0;
    m_InstanceIndex = static_cast<ezUInt32>(instanceIndex);
    m_Generation = uiGeneration;
    m_TypeId = uiTypeId;
    m_WorldIndex = uiWorldIndex;
  }

  union
  {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : 32;
      StorageType m_Generation : 8;
      StorageType m_WorldIndex : EZ_WORLD_INDEX_BITS;
      StorageType m_TypeId : EZ_COMPONENT_TYPE_INDEX_BITS;
    };
  };
};

/// \brief A handle to a component.
///
/// Never store a direct pointer to a component. Always store a handle instead. A pointer to a component can
/// be received by calling ezWorld::TryGetComponent or TryGetComponent on the corresponding component manager.
/// Note that the component might have been deleted so always check the return value of TryGetComponent.
struct ezComponentHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezComponentHandle, ezComponentId);

  friend class ezWorld;
  friend class ezComponentManagerBase;
  friend class ezComponent;
};

/// \brief A typed handle to a component.
///
/// This should be preferred if the component type to be stored inside the handle is known, as it provides
/// compile time checks against wrong usages (e.g. assigning unrelated types) and more clearly conveys intent.
///
/// See struct \see ezComponentHandle for more information about general component handle usage.
template <typename TYPE>
struct ezTypedComponentHandle : public ezComponentHandle
{
  ezTypedComponentHandle() = default;
  explicit ezTypedComponentHandle(const ezComponentHandle& hUntyped)
  {
    m_InternalId = hUntyped.GetInternalID();
  }

  template <typename T, std::enable_if_t<std::is_convertible_v<T*, TYPE*>, bool> = true>
  explicit ezTypedComponentHandle(const ezTypedComponentHandle<T>& other)
    : ezTypedComponentHandle(static_cast<const ezComponentHandle&>(other))
  {
  }

  template <typename T, std::enable_if_t<std::is_convertible_v<T*, TYPE*>, bool> = true>
  EZ_ALWAYS_INLINE void operator=(const ezTypedComponentHandle<T>& other)
  {
    ezComponentHandle::operator=(other);
  }
};

/// \brief HashHelper implementation so component handles can be used as key in a hashtable.
template <>
struct ezHashHelper<ezComponentHandle>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezComponentHandle value)
  {
    ezComponentId id = value.GetInternalID();
    ezUInt64 data = *reinterpret_cast<ezUInt64*>(&id);
    return ezHashHelper<ezUInt64>::Hash(data);
  }

  EZ_ALWAYS_INLINE static bool Equal(ezComponentHandle a, ezComponentHandle b) { return a == b; }
};

/// \brief Currently not implemented as it is not needed for component handles.
EZ_CORE_DLL void operator<<(ezStreamWriter& inout_stream, const ezComponentHandle& hValue);
EZ_CORE_DLL void operator>>(ezStreamReader& inout_stream, ezComponentHandle& ref_hValue);

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezComponentHandle);
EZ_DECLARE_CUSTOM_VARIANT_TYPE(ezComponentHandle);

/// \brief Internal flags of game objects or components.
struct ezObjectFlags
{
  using StorageType = ezUInt32;

  enum Enum
  {
    None = 0,
    Dynamic = EZ_BIT(0),                              ///< Usually detected automatically. A dynamic object will not cache render data across frames.
    ForceDynamic = EZ_BIT(1),                         ///< Set by the user to enforce the 'Dynamic' mode. Necessary when user code (or scripts) should change
                                                      ///< objects, and the automatic detection cannot know that.
    ActiveFlag = EZ_BIT(2),                           ///< The object/component has the 'active flag' set
    ActiveState = EZ_BIT(3),                          ///< The object/component and all its parents have the active flag
    Initialized = EZ_BIT(4),                          ///< The object/component has been initialized
    Initializing = EZ_BIT(5),                         ///< The object/component is currently initializing. Used to prevent recursions during initialization.
    SimulationStarted = EZ_BIT(6),                    ///< OnSimulationStarted() has been called on the component
    SimulationStarting = EZ_BIT(7),                   ///< Used to prevent recursion during OnSimulationStarted()
    UnhandledMessageHandler = EZ_BIT(8),              ///< For components, when a message is not handled, a virtual function is called

    ChildChangesNotifications = EZ_BIT(9),            ///< The object should send a notification message when children are added or removed.
    ComponentChangesNotifications = EZ_BIT(10),       ///< The object should send a notification message when components are added or removed.
    StaticTransformChangesNotifications = EZ_BIT(11), ///< The object should send a notification message if it is static and its transform changes.
    ParentChangesNotifications = EZ_BIT(12),          ///< The object should send a notification message when the parent is changes.

    CreatedByPrefab = EZ_BIT(13),                     ///< Such flagged objects and components are ignored during scene export (see ezWorldWriter) and will be removed when a prefab needs to be re-instantiated.

    UserFlag0 = EZ_BIT(24),
    UserFlag1 = EZ_BIT(25),
    UserFlag2 = EZ_BIT(26),
    UserFlag3 = EZ_BIT(27),
    UserFlag4 = EZ_BIT(28),
    UserFlag5 = EZ_BIT(29),
    UserFlag6 = EZ_BIT(30),
    UserFlag7 = EZ_BIT(31),

    Default = None
  };

  struct Bits
  {
    StorageType Dynamic : 1;                             //< 0
    StorageType ForceDynamic : 1;                        //< 1
    StorageType ActiveFlag : 1;                          //< 2
    StorageType ActiveState : 1;                         //< 3
    StorageType Initialized : 1;                         //< 4
    StorageType Initializing : 1;                        //< 5
    StorageType SimulationStarted : 1;                   //< 6
    StorageType SimulationStarting : 1;                  //< 7
    StorageType UnhandledMessageHandler : 1;             //< 8
    StorageType ChildChangesNotifications : 1;           //< 9
    StorageType ComponentChangesNotifications : 1;       //< 10
    StorageType StaticTransformChangesNotifications : 1; //< 11
    StorageType ParentChangesNotifications : 1;          //< 12

    StorageType CreatedByPrefab : 1;                     //< 13

    StorageType Padding : 10;                            // 14 - 23

    StorageType UserFlag0 : 1;                           //< 24
    StorageType UserFlag1 : 1;                           //< 25
    StorageType UserFlag2 : 1;                           //< 26
    StorageType UserFlag3 : 1;                           //< 27
    StorageType UserFlag4 : 1;                           //< 28
    StorageType UserFlag5 : 1;                           //< 29
    StorageType UserFlag6 : 1;                           //< 30
    StorageType UserFlag7 : 1;                           //< 31
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezObjectFlags);

/// \brief Specifies the mode of an object. This enum is only used in the editor.
///
/// \sa ezObjectFlags
struct ezObjectMode
{
  using StorageType = ezUInt8;

  enum Enum : ezUInt8
  {
    Automatic,
    ForceDynamic,

    Default = Automatic
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezObjectMode);

/// \brief Specifies the mode of a component. Dynamic components may change an object's transform, static components must not.
///
/// \sa ezObjectFlags
struct ezComponentMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Static,
    Dynamic,

    Default = Static
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezComponentMode);

/// \brief Specifies at which phase the queued message should be processed.
struct ezObjectMsgQueueType
{
  using StorageType = ezUInt8;

  enum Enum : StorageType
  {
    PostAsync,        ///< Process the message in the PostAsync phase.
    PostTransform,    ///< Process the message in the PostTransform phase.
    NextFrame,        ///< Process the message in the PreAsync phase of the next frame.
    AfterInitialized, ///< Process the message after new components have been initialized.
    COUNT,

    Default = NextFrame
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezObjectMsgQueueType);

/// \brief Certain components may delete themselves or their owner when they are finished with their main purpose
struct EZ_CORE_DLL ezOnComponentFinishedAction
{
  using StorageType = ezUInt8;

  enum Enum : StorageType
  {
    None,             ///< Nothing happens after the action is finished.
    DeleteComponent,  ///< The component deletes only itself, but its game object stays.
    DeleteGameObject, ///< When finished the component deletes its owner game object. If there are multiple objects with this mode, the component instead deletes itself, and only the last such component deletes the game object.

    Default = None
  };

  /// \brief Call this when a component is 'finished' with its work.
  ///
  /// Pass in the desired action (usually configured by the user) and the 'this' pointer of the component.
  /// The helper function will delete this component and maybe also attempt to delete the entire object.
  /// For that it will coordinate with other components, and delay the object deletion, if necessary,
  /// until the last component has finished it's work.
  static void HandleFinishedAction(ezComponent* pComponent, ezOnComponentFinishedAction::Enum action);

  /// \brief Call this function in a message handler for ezMsgDeleteGameObject messages.
  ///
  /// This is needed to coordinate object deletion across multiple components that use the
  /// ezOnComponentFinishedAction mechanism.
  /// Depending on the state of this component, the function will either execute the object deletion,
  /// or delay it, until its own work is done.
  static void HandleDeleteObjectMsg(ezMsgDeleteGameObject& ref_msg, ezEnum<ezOnComponentFinishedAction>& ref_action);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezOnComponentFinishedAction);

/// \brief Same as ezOnComponentFinishedAction, but additionally includes 'Restart'
struct EZ_CORE_DLL ezOnComponentFinishedAction2
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None,             ///< Nothing happens after the action is finished.
    DeleteComponent,  ///< The component deletes only itself, but its game object stays.
    DeleteGameObject, ///< When finished the component deletes its owner game object. If there are multiple objects with this mode, the component instead deletes itself, and only the last such component deletes the game object.
    Restart,          ///< When finished, restart from the beginning.

    Default = None
  };

  /// \brief See ezOnComponentFinishedAction::HandleFinishedAction()
  static void HandleFinishedAction(ezComponent* pComponent, ezOnComponentFinishedAction2::Enum action);

  /// \brief See ezOnComponentFinishedAction::HandleDeleteObjectMsg()
  static void HandleDeleteObjectMsg(ezMsgDeleteGameObject& ref_msg, ezEnum<ezOnComponentFinishedAction2>& ref_action);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezOnComponentFinishedAction2);

/// \brief Used as return value of visitor functions to define whether calling function should stop or continue visiting.
struct ezVisitorExecution
{
  enum Enum
  {
    Continue, ///< Continue regular iteration
    Skip,     ///< In a depth-first iteration mode this will skip the entire sub-tree below the current object
    Stop      ///< Stop the entire iteration
  };
};

using ezSpatialDataId = ezGenericId<24, 8>;
class ezSpatialDataHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezSpatialDataHandle, ezSpatialDataId);
};

#define EZ_MAX_WORLD_MODULE_TYPES EZ_MAX_COMPONENT_TYPES
using ezWorldModuleTypeId = ezUInt16;
static_assert(ezMath::MaxValue<ezWorldModuleTypeId>() >= EZ_MAX_WORLD_MODULE_TYPES - 1);

using ezComponentInitBatchId = ezGenericId<24, 8>;
class ezComponentInitBatchHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezComponentInitBatchHandle, ezComponentInitBatchId);
};
