#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>

class ezBlackboard;
class ezStateMachineInstance;

/// \brief Base class for a state in a state machine.
///
/// Note that states are shared between multiple instances and thus
/// shouldn't modify any data on their own but always operate on the passed instance and instance data.
/// \see ezStateMachineInstanceDataTypeAttribute
class EZ_GAMEENGINE_DLL ezStateMachineState : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineState, ezReflectedClass);

public:
  ezStateMachineState(ezStringView sName = ezStringView());

  void SetName(ezStringView sName);
  ezStringView GetName() const { return m_sName; }
  const ezHashedString& GetNameHashed() const { return m_sName; }

protected:
  friend class ezStateMachineDescription;
  friend class ezStateMachineInstance;

  virtual void OnEnter(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pFromState) const = 0;
  virtual void OnExit(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pToState) const;
  virtual void Update(ezStateMachineInstance& instance, void* pInstanceData) const;

  virtual ezResult Serialize(ezStreamWriter& stream) const;
  virtual ezResult Deserialize(ezStreamReader& stream);

private:
  ezHashedString m_sName;
};

/// \brief Base class for a transition in a state machine. The target state of a transition is automatically set
/// once its condition has been met.
///
/// Same as with states, transitions are also shared between multiple instances and thus
/// should decide their condition based on the passed instance and instance data.
/// \see ezStateMachineInstanceDataTypeAttribute
class EZ_GAMEENGINE_DLL ezStateMachineTransition : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineTransition, ezReflectedClass);

protected:
  friend class ezStateMachineDescription;
  friend class ezStateMachineInstance;

  virtual bool IsConditionMet(ezStateMachineInstance& instance, void* pInstanceData) const = 0;

  virtual ezResult Serialize(ezStreamWriter& stream) const;
  virtual ezResult Deserialize(ezStreamReader& stream);
};

/// \brief Attribute to define instance data for a state or transition.
///
/// Since state machine states and transitions are shared between instances they can't hold their state in member
/// variables. Use this attribute to define the type of instance data necessary for a state or transition.
/// Instance data is then automatically allocated by the state machine instance and passed via the pInstanceData pointer
/// in the state or transition functions.
/// Use the EZ_STATE_MACHINE_INSTANCE_DATA_TYPE_ATTRIBUTE below to define such an attribute in the reflection section.
class EZ_GAMEENGINE_DLL ezStateMachineInstanceDataTypeAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineInstanceDataTypeAttribute, ezPropertyAttribute);

public:
  ezStateMachineInstanceDataTypeAttribute() = default;
  ezStateMachineInstanceDataTypeAttribute(ezUInt32 uiTypeSize, ezUInt32 uiTypeAlignment, ezMemoryUtils::ConstructorFunction constructorFunc, ezMemoryUtils::DestructorFunction destructorFunc)
    : m_uiTypeSize(uiTypeSize)
    , m_uiTypeAlignment(uiTypeAlignment)
    , m_ConstructorFunction(constructorFunc)
    , m_DestructorFunction(destructorFunc)
  {
  }

  ezUInt32 m_uiTypeSize = 0;
  ezUInt32 m_uiTypeAlignment = 0;
  ezMemoryUtils::ConstructorFunction m_ConstructorFunction = nullptr;
  ezMemoryUtils::DestructorFunction m_DestructorFunction = nullptr;
};

/// \brief Helper macro to define instance data. \see ezStateMachineInstanceDataTypeAttribute.
#define EZ_STATE_MACHINE_INSTANCE_DATA_TYPE_ATTRIBUTE(T) new ezStateMachineInstanceDataTypeAttribute(sizeof(T), EZ_ALIGNMENT_OF(T), ezMemoryUtils::MakeConstructorFunction<T>(), ezMemoryUtils::MakeDestructorFunction<T>())

/// \brief The state machine description defines the structure of a state machine like e.g.
/// what states it has and how to transition between them.
/// Once an instance is created from a description it is not allowed to change the description afterwards.
class EZ_GAMEENGINE_DLL ezStateMachineDescription : public ezRefCounted
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezStateMachineDescription);

public:
  ezStateMachineDescription();
  ~ezStateMachineDescription();

  /// \brief Adds the given state to the description and returns the state index.
  ezUInt32 AddState(ezUniquePtr<ezStateMachineState>&& pState);

  /// \brief Adds the given transition between the two given states. A uiFromStateIndex of ezInvalidIndex generates a transition that can be done from any other possible state.
  void AddTransition(ezUInt32 uiFromStateIndex, ezUInt32 uiToStateIndex, ezUniquePtr<ezStateMachineTransition>&& pTransistion);

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);

private:
  friend class ezStateMachineInstance;

  struct TransitionContext
  {
    ezUniquePtr<ezStateMachineTransition> m_pTransition;
    ezUInt32 m_uiToStateIndex = 0;
    ezUInt32 m_uiInstanceDataOffset = ezInvalidIndex;
  };

  using TransitionArray = ezSmallArray<TransitionContext, 2>;
  TransitionArray m_FromAnyTransitions;

  struct StateContext
  {
    ezUniquePtr<ezStateMachineState> m_pState;
    TransitionArray m_Transitions;
    ezUInt32 m_uiInstanceDataOffset = ezInvalidIndex;
  };

  ezDynamicArray<StateContext> m_States;
  ezHashTable<ezHashedString, ezUInt32> m_StateNameToIndexTable;

  ezDynamicArray<const ezStateMachineInstanceDataTypeAttribute*> m_InstanceDataAttributes;
  ezUInt32 m_uiTotalInstanceDataSize = 0;
};

/// \brief The state machine instance represents the actual state machine.
/// Typically it is created from a description but for small use cases it can also be used without a description.
class EZ_GAMEENGINE_DLL ezStateMachineInstance
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezStateMachineInstance);

public:
  ezStateMachineInstance(ezReflectedClass& owner, const ezSharedPtr<const ezStateMachineDescription>& pDescription = nullptr);
  ~ezStateMachineInstance();

  ezResult SetState(ezStateMachineState* pState);
  ezResult SetState(ezUInt32 uiStateIndex);
  ezResult SetState(const ezHashedString& sStateName);
  ezResult SetStateOrFallback(const ezHashedString& sStateName, ezUInt32 uiFallbackStateIndex = 0);
  ezStateMachineState* GetCurrentState() { return m_pCurrentState; }

  void Update();

  ezReflectedClass& GetOwner() { return m_owner; }

  void SetBlackboard(const ezSharedPtr<ezBlackboard>& blackboard);
  const ezSharedPtr<ezBlackboard>& GetBlackboard() const { return m_pBlackboard; }

private:
  void SetStateInternal(ezUInt32 uiStateIndex);
  void EnterCurrentState(const ezStateMachineState* pFromState);
  void ExitCurrentState(const ezStateMachineState* pToState);
  ezUInt32 FindNewStateToTransitionTo();

  EZ_ALWAYS_INLINE void* GetInstanceData(ezUInt32 uiOffset)
  {
    return (uiOffset != ezInvalidIndex) ? m_InstanceData.GetByteBlobPtr().GetPtr() + uiOffset : nullptr;
  }

  EZ_ALWAYS_INLINE void* GetCurrentStateInstanceData()
  {
    if (m_pDescription != nullptr && m_uiCurrentStateIndex < m_pDescription->m_States.GetCount())
    {
      return GetInstanceData(m_pDescription->m_States[m_uiCurrentStateIndex].m_uiInstanceDataOffset);
    }
    return nullptr;
  }

  ezReflectedClass& m_owner;
  ezSharedPtr<const ezStateMachineDescription> m_pDescription;
  ezSharedPtr<ezBlackboard> m_pBlackboard;

  ezStateMachineState* m_pCurrentState = nullptr;
  ezUInt32 m_uiCurrentStateIndex = ezInvalidIndex;

  const ezStateMachineDescription::TransitionArray* m_pCurrentTransitions = nullptr;

  ezBlob m_InstanceData;
};
