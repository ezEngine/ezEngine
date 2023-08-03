#pragma once

#include <ToolsFoundation/Command/Command.h>

class ezDocumentObject;
class ezCommandHistory;
class ezPin;

class EZ_TOOLSFOUNDATION_DLL ezRemoveNodeCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRemoveNodeCommand, ezCommand);

public:
  ezRemoveNodeCommand();

public: // Properties
  ezUuid m_Object;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  ezDocumentObject* m_pObject = nullptr;
};


class EZ_TOOLSFOUNDATION_DLL ezMoveNodeCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMoveNodeCommand, ezCommand);

public:
  ezMoveNodeCommand();

public: // Properties
  ezUuid m_Object;
  ezVec2 m_NewPos = ezVec2::MakeZero();

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  ezDocumentObject* m_pObject = nullptr;
  ezVec2 m_vOldPos = ezVec2::MakeZero();
};


class EZ_TOOLSFOUNDATION_DLL ezConnectNodePinsCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConnectNodePinsCommand, ezCommand);

public:
  ezConnectNodePinsCommand();

public: // Properties
  ezUuid m_ConnectionObject;
  ezUuid m_ObjectSource;
  ezUuid m_ObjectTarget;
  ezString m_sSourcePin;
  ezString m_sTargetPin;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  ezDocumentObject* m_pConnectionObject = nullptr;
  ezDocumentObject* m_pObjectSource = nullptr;
  ezDocumentObject* m_pObjectTarget = nullptr;
};


class EZ_TOOLSFOUNDATION_DLL ezDisconnectNodePinsCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDisconnectNodePinsCommand, ezCommand);

public:
  ezDisconnectNodePinsCommand();

public: // Properties
  ezUuid m_ConnectionObject;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  ezDocumentObject* m_pConnectionObject = nullptr;
  const ezDocumentObject* m_pObjectSource = nullptr;
  const ezDocumentObject* m_pObjectTarget = nullptr;
  ezString m_sSourcePin;
  ezString m_sTargetPin;
};


class EZ_TOOLSFOUNDATION_DLL ezNodeCommands
{
public:
  static ezStatus AddAndConnectCommand(ezCommandHistory* pHistory, const ezRTTI* pConnectionType, const ezPin& sourcePin, const ezPin& targetPin);
  static ezStatus DisconnectAndRemoveCommand(ezCommandHistory* pHistory, const ezUuid& connectionObject);
};
