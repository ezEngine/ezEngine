#pragma once

#include <GameEngine/StateMachine/Implementation/StateMachineInstanceData.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>

class ezComponent;
class ezWorld;
class ezBlackboard;
class ezStateMachineInstance;

/// \brief Base class for a state in a state machine.
///
/// Note that states are shared between multiple instances and thus
/// shouldn't modify any data on their own but always operate on the passed instance and instance data.
/// \see ezStateMachineInstanceDataDesc
class EZ_GAMEENGINE_DLL ezStateMachineState : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineState, ezReflectedClass);

public:
  ezStateMachineState(ezStringView sName = ezStringView());

  void SetName(ezStringView sName);
  ezStringView GetName() const { return m_sName; }
  const ezHashedString& GetNameHashed() const { return m_sName; }

  virtual void OnEnter(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pFromState) const = 0;
  virtual void OnExit(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pToState) const;
  virtual void Update(ezStateMachineInstance& ref_instance, void* pInstanceData, ezTime deltaTime) const;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const;
  virtual ezResult Deserialize(ezStreamReader& inout_stream);

  /// \brief Returns whether this state needs additional instance data and if so fills the out_desc.
  ///
  /// \see ezStateMachineInstanceDataDesc
  virtual bool GetInstanceDataDesc(ezStateMachineInstanceDataDesc& out_desc);

private:
  // These are dummy functions for the scripting reflection
  void Reflection_OnEnter(ezStateMachineInstance* pStateMachineInstance, const ezStateMachineState* pFromState);
  void Reflection_OnExit(ezStateMachineInstance* pStateMachineInstance, const ezStateMachineState* pToState);
  void Reflection_Update(ezStateMachineInstance* pStateMachineInstance, ezTime deltaTime);

  ezHashedString m_sName;
};

struct ezStateMachineState_ScriptBaseClassFunctions
{
  enum Enum
  {
    OnEnter,
    OnExit,
    Update,

    Count
  };
};

/// \brief Base class for a transition in a state machine. The target state of a transition is automatically set
/// once its condition has been met.
///
/// Same as with states, transitions are also shared between multiple instances and thus
/// should decide their condition based on the passed instance and instance data.
/// \see ezStateMachineInstanceDataDesc
class EZ_GAMEENGINE_DLL ezStateMachineTransition : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineTransition, ezReflectedClass);

  virtual bool IsConditionMet(ezStateMachineInstance& ref_instance, void* pInstanceData) const = 0;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const;
  virtual ezResult Deserialize(ezStreamReader& inout_stream);

  /// \brief Returns whether this transition needs additional instance data and if so fills the out_desc.
  ///
  /// \see ezStateMachineInstanceDataDesc
  virtual bool GetInstanceDataDesc(ezStateMachineInstanceDataDesc& out_desc);
};

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

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

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

  ezStateMachineInternal::InstanceDataAllocator m_InstanceDataAllocator;
};

/// \brief The state machine instance represents the actual state machine.
/// Typically it is created from a description but for small use cases it can also be used without a description.
class EZ_GAMEENGINE_DLL ezStateMachineInstance
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezStateMachineInstance);

public:
  ezStateMachineInstance(ezReflectedClass& ref_owner, const ezSharedPtr<const ezStateMachineDescription>& pDescription = nullptr);
  ~ezStateMachineInstance();

  ezResult SetState(ezStateMachineState* pState);
  ezResult SetState(ezUInt32 uiStateIndex);
  ezResult SetState(const ezHashedString& sStateName);
  ezResult SetStateOrFallback(const ezHashedString& sStateName, ezUInt32 uiFallbackStateIndex = 0);
  ezStateMachineState* GetCurrentState() { return m_pCurrentState; }

  void Update(ezTime deltaTime);

  ezReflectedClass& GetOwner() { return m_Owner; }
  ezWorld* GetOwnerWorld();

  void SetBlackboard(const ezSharedPtr<ezBlackboard>& pBlackboard);
  const ezSharedPtr<ezBlackboard>& GetBlackboard() const { return m_pBlackboard; }

  /// \brief Returns how long the state machine is in its current state
  ezTime GetTimeInCurrentState() const { return m_TimeInCurrentState; }

public:
  // The following functions are for scripting reflection and shouldn't be called manually
  bool Reflection_SetState(ezStringView sStateName);
  ezComponent* Reflection_GetOwnerComponent() const;
  ezBlackboard* Reflection_GetBlackboard() const { return m_pBlackboard.Borrow(); }

private:
  void SetStateInternal(ezUInt32 uiStateIndex);
  void EnterCurrentState(const ezStateMachineState* pFromState);
  void ExitCurrentState(const ezStateMachineState* pToState);
  ezUInt32 FindNewStateToTransitionTo();

  EZ_ALWAYS_INLINE void* GetInstanceData(ezUInt32 uiOffset)
  {
    return ezStateMachineInternal::InstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), uiOffset);
  }

  EZ_ALWAYS_INLINE void* GetCurrentStateInstanceData()
  {
    if (m_pDescription != nullptr && m_uiCurrentStateIndex < m_pDescription->m_States.GetCount())
    {
      return GetInstanceData(m_pDescription->m_States[m_uiCurrentStateIndex].m_uiInstanceDataOffset);
    }
    return nullptr;
  }

  ezReflectedClass& m_Owner;
  ezSharedPtr<const ezStateMachineDescription> m_pDescription;
  ezSharedPtr<ezBlackboard> m_pBlackboard;

  ezStateMachineState* m_pCurrentState = nullptr;
  ezUInt32 m_uiCurrentStateIndex = ezInvalidIndex;
  ezTime m_TimeInCurrentState;

  const ezStateMachineDescription::TransitionArray* m_pCurrentTransitions = nullptr;

  ezBlob m_InstanceData;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezStateMachineInstance);
