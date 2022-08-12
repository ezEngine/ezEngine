#pragma once

#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class ezStateMachinePin : public ezPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachinePin, ezPin);

public:
  ezStateMachinePin(Type type, const ezDocumentObject* pObject);
};

class ezStateMachineTransition;

class ezStateMachineConnection : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineConnection, ezReflectedClass);

public:
  ezStateMachineTransition* m_pType = nullptr;
};

class ezStateMachineNodeBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineNodeBase, ezReflectedClass);
};

class ezStateMachineState;

class ezStateMachineNode : public ezStateMachineNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineNode, ezStateMachineNodeBase);

public:
  ezString m_sName;
  ezStateMachineState* m_pType = nullptr;
};

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
