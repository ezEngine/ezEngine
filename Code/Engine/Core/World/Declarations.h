#pragma once

#include <Foundation/Memory/BlockStorage.h>
#include <Foundation/Memory/LargeBlockAllocator.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Id.h>

#include <Core/CoreDLL.h>

class ezWorld;
class ezSpatialSystem;
class ezCoordinateSystemProvider;

namespace ezInternal
{
  class WorldData;

  enum
  {
    DEFAULT_BLOCK_SIZE = 1024 * 4
  };

  typedef ezLargeBlockAllocator<DEFAULT_BLOCK_SIZE> WorldLargeBlockAllocator;
} // namespace ezInternal

class ezGameObject;
struct ezGameObjectDesc;

class ezComponentManagerBase;
class ezComponent;

struct ezMsgDeleteGameObject;

/// \brief Internal game object id used by ezGameObjectHandle.
struct ezGameObjectId
{
  typedef ezUInt32 StorageType;

  EZ_DECLARE_ID_TYPE(ezGameObjectId, 20, 6);

  EZ_FORCE_INLINE ezGameObjectId(StorageType instanceIndex, StorageType generation, StorageType worldIndex = 0)
  {
    m_Data = 0;
    m_InstanceIndex = instanceIndex;
    m_Generation = generation;
    m_WorldIndex = worldIndex;
  }

  union {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : 20;
      StorageType m_Generation : 6;
      StorageType m_WorldIndex : 6;
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

/// \brief HashHelper implementation so game object handles can be used as key in a hashtable.
template <>
struct ezHashHelper<ezGameObjectHandle>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezGameObjectHandle value) { return ezHashHelper<ezUInt32>::Hash(value.GetInternalID().m_Data); }

  EZ_ALWAYS_INLINE static bool Equal(ezGameObjectHandle a, ezGameObjectHandle b) { return a == b; }
};

typedef ezGenericId<24, 8> ezGenericComponentId;

/// \brief Internal component id used by ezComponentHandle.
struct ezComponentId : public ezGenericComponentId
{
  EZ_ALWAYS_INLINE ezComponentId()
      : ezGenericComponentId()
  {
    m_TypeId = 0;
    m_WorldIndex = 0;
  }

  EZ_ALWAYS_INLINE ezComponentId(StorageType instanceIndex, StorageType generation, ezUInt16 typeId = 0, ezUInt16 worldIndex = 0)
      : ezGenericComponentId(instanceIndex, generation)
  {
    m_TypeId = typeId;
    m_WorldIndex = worldIndex;
  }

  EZ_ALWAYS_INLINE ezComponentId(ezGenericComponentId genericId, ezUInt16 typeId, ezUInt16 worldIndex)
      : ezGenericComponentId(genericId)
  {
    m_TypeId = typeId;
    m_WorldIndex = worldIndex;
  }

  EZ_ALWAYS_INLINE bool operator==(const ezComponentId other) const
  {
    const ezUInt32& d1 = reinterpret_cast<const ezUInt32&>(m_TypeId);
    const ezUInt32& d2 = reinterpret_cast<const ezUInt32&>(other.m_TypeId);

    return m_Data == other.m_Data && d1 == d2;
  }
  EZ_ALWAYS_INLINE bool operator!=(const ezComponentId other) const
  {
    const ezUInt32& d1 = reinterpret_cast<const ezUInt32&>(m_TypeId);
    const ezUInt32& d2 = reinterpret_cast<const ezUInt32&>(other.m_TypeId);

    return m_Data != other.m_Data || d1 != d2;
  }

  EZ_ALWAYS_INLINE bool operator<(const ezComponentId other) const
  {
    const ezUInt32& d1 = reinterpret_cast<const ezUInt32&>(m_TypeId);
    const ezUInt32& d2 = reinterpret_cast<const ezUInt32&>(other.m_TypeId);

    if (d1 == d2)
    {
      return m_Data < other.m_Data;
    }

    return d1 < d2;
  }

  // don't change the order or alignment of these, otherwise the comparison above fails
  ezUInt16 m_TypeId;
  ezUInt16 m_WorldIndex;
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

/// \brief Internal flags of game objects or components.
struct ezObjectFlags
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    None = 0,
    Dynamic = EZ_BIT(0),            ///< Usually detected automatically. A dynamic object will not cache render data across frames.
    ForceDynamic = EZ_BIT(1),       ///< Set by the user to enforce the 'Dynamic' mode. Necessary when user code (or scripts) should change
                                    ///< objects, and the automatic detection cannot know that.
    Active = EZ_BIT(2),             ///< The object/component is currently in the 'active' state
    Initialized = EZ_BIT(3),        ///< The object/component has been initialized
    Initializing = EZ_BIT(4),       ///< The object/component is currently initializing. Used to prevent recursions during initialization.
    SimulationStarted = EZ_BIT(5),  ///< OnSimulationStarted() has been called on the component
    SimulationStarting = EZ_BIT(6), ///< Used to prevent recursion during OnSimulationStarted()
    UnhandledMessageHandler = EZ_BIT(7), ///< For components, when a message is not handled, a virtual function is called

    ChildChangesNotifications = EZ_BIT(8), ///< The object should send a notification message when children are added or removed.
    ComponentChangesNotifications = EZ_BIT(9), ///< The object should send a notification message when components are added or removed.

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
    StorageType Dynamic : 1;
    StorageType ForceDynamic : 1;
    StorageType Active : 1;
    StorageType Initialized : 1;
    StorageType Initializing : 1;
    StorageType SimulationStarted : 1;
    StorageType SimulationStarting : 1;
    StorageType UnhandledMessageHandler : 1;

    StorageType ChildChangesNotifications : 1;
    StorageType ComponentChangesNotifications : 1;

    StorageType Padding : 14;

    StorageType UserFlag0 : 1;
    StorageType UserFlag1 : 1;
    StorageType UserFlag2 : 1;
    StorageType UserFlag3 : 1;
    StorageType UserFlag4 : 1;
    StorageType UserFlag5 : 1;
    StorageType UserFlag6 : 1;
    StorageType UserFlag7 : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezObjectFlags);

/// \brief Specifies the mode of an object. This enum is only used in the editor.
///
/// \sa ezObjectFlags
struct ezObjectMode
{
  typedef ezUInt8 StorageType;

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
  enum Enum
  {
    Static,
    Dynamic
  };
};

/// \brief Specifies at which phase the queued message should be processed.
struct ezObjectMsgQueueType
{
  enum Enum
  {
    PostAsync,        ///< Process the message in the PostAsync phase.
    PostTransform,    ///< Process the message in the PostTransform phase.
    NextFrame,        ///< Process the message in the PreAsync phase of the next frame.
    AfterInitialized, ///< Process the message after new components have been initialized.
    COUNT
  };
};

/// \brief Certain components may delete themselves or their owner when they are finished with their main purpose
struct EZ_CORE_DLL ezOnComponentFinishedAction
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None,
    DeleteComponent,
    DeleteGameObject,

    Default = None
  };

  // helper function
  static void HandleFinishedAction(ezComponent* pComponent, ezOnComponentFinishedAction::Enum action);
  static void HandleDeleteObjectMsg(ezMsgDeleteGameObject& msg, ezEnum<ezOnComponentFinishedAction>& action);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezOnComponentFinishedAction);

/// \brief Same as ezOnComponentFinishedAction, but additionally includes 'Restart'
struct EZ_CORE_DLL ezOnComponentFinishedAction2
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None,
    DeleteComponent,
    DeleteGameObject,
    Restart,

    Default = None
  };

  // helper function
  static void HandleFinishedAction(ezComponent* pComponent, ezOnComponentFinishedAction2::Enum action);
  static void HandleDeleteObjectMsg(ezMsgDeleteGameObject& msg, ezEnum<ezOnComponentFinishedAction2>& action);
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

typedef ezGenericId<24, 8> ezSpatialDataId;
class ezSpatialDataHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezSpatialDataHandle, ezSpatialDataId);
};

