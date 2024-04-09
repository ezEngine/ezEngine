#pragma once

#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class ezStateMachinePin : public ezPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachinePin, ezPin);

public:
  ezStateMachinePin(Type type, const ezDocumentObject* pObject);
};

class ezStateMachineNodeManager : public ezDocumentNodeManager
{
public:
  ezStateMachineNodeManager();
  ~ezStateMachineNodeManager();

  bool IsInitialState(const ezDocumentObject* pObject) const;
  const ezDocumentObject* GetInitialState() const;

  bool IsAnyState(const ezDocumentObject* pObject) const;

private:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual ezStatus InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_Result) const override;

  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node) override;

  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override;
  virtual const ezRTTI* GetConnectionType() const override;

  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
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
  ezDocumentObject* m_pOldInitialStateObject = nullptr;
  ezDocumentObject* m_pNewInitialStateObject = nullptr;
};
