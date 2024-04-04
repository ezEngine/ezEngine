#pragma once

#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class ezStateMachineState;
class ezStateMachineTransition;

class ezStateMachinePin : public ezPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachinePin, ezPin);

public:
  ezStateMachinePin(Type type, const ezDocumentObject* pObject);
};

/// \brief A connection that represents a state machine transition. Since we can't chose different connection
/// types in the Editor we allow the user to switch the type in the properties.
class ezStateMachineConnection : public ezDocumentObject_ConnectionBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineConnection, ezDocumentObject_ConnectionBase);

public:
  ezStateMachineTransition* m_pType = nullptr;
};

/// \brief Base class for nodes in the state machine graph
class ezStateMachineNodeBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineNodeBase, ezReflectedClass);
};

/// \brief A node that represents a state machine state. We don't use ezStateMachineState directly to allow
/// the user to switch the type in the properties similar to what we do with transitions.
class ezStateMachineNode : public ezStateMachineNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineNode, ezStateMachineNodeBase);

public:
  ezString m_sName;
  ezStateMachineState* m_pType = nullptr;
};

/// \brief A node that represents "any" state machine state. This can be used if a transition with the same conditions
/// is possible from any other state in the state machine. Instead of creating many connections with the same properties
/// an "any" state can be used to make the graph much easier to read and to maintain.
///
/// Note that there is no "any" state at runtime but rather only the transition is stored.
class ezStateMachineNodeAny : public ezStateMachineNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineNodeAny, ezStateMachineNodeBase);
};

class ezStateMachineNodeManager : public ezDocumentNodeManager
{
public:
  ezStateMachineNodeManager();
  ~ezStateMachineNodeManager();

  bool IsInitialState(const ezDocumentObject* pObject) const { return pObject == m_pInitialStateObject; }
  void SetInitialState(const ezDocumentObject* pObject);
  const ezDocumentObject* GetInitialState() const { return m_pInitialStateObject; }

  bool IsAnyState(const ezDocumentObject* pObject) const;

private:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual ezStatus InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_Result) const override;

  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node) override;

  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override;
  virtual const ezRTTI* GetConnectionType() const override;

  void ObjectHandler(const ezDocumentObjectEvent& e);

private:
  const ezDocumentObject* m_pInitialStateObject = nullptr;
};

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
  const ezDocumentObject* m_pOldInitialStateObject = nullptr;
  const ezDocumentObject* m_pNewInitialStateObject = nullptr;
};
