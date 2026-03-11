#pragma once

#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

/// Visual graph pin for state machine nodes.
///
/// Represents connection points between states. Output pins trigger transitions, input pins receive them.
class ezStateMachinePin : public ezVisualGraphPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachinePin, ezVisualGraphPin);

public:
  ezStateMachinePin(Type type, const ezDocumentObject* pObject);
};

/// Object manager for state machine graphs.
///
/// Manages states and transitions in a state machine. Ensures that exactly one initial state exists
/// and handles special states like the "Any State" node which can transition to any other state.
class ezStateMachineNodeManager : public ezVisualGraphObjectManager
{
public:
  ezStateMachineNodeManager();
  ~ezStateMachineNodeManager();

  bool IsInitialState(const ezDocumentObject* pObject) const;
  const ezDocumentObject* GetInitialState() const;

  bool IsAnyState(const ezDocumentObject* pObject) const;

private:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual ezStatus InternalCanConnect(const ezVisualGraphPin& source, const ezVisualGraphPin& target, CanConnectResult& out_Result) const override;

  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node) override;

  virtual void GetCreateableTypes(ezDynamicArray<const ezRTTI*>& out_types) const override;
  virtual const ezRTTI* GetConnectionType() const override;

  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
};

/// Command to designate which state is the initial state in a state machine.
///
/// Sets or changes the initial state of a state machine graph. The operation is undoable.
class ezStateMachine_SetInitialStateCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachine_SetInitialStateCommand, ezCommand);

public:
  ezStateMachine_SetInitialStateCommand();

public: // Properties
  ezUuid m_NewInitialStateObject;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  ezDocumentObject* m_pOldInitialStateObject = nullptr;
  ezDocumentObject* m_pNewInitialStateObject = nullptr;
};
