#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

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
  ezDocumentObject* m_pObject;
};


class EZ_TOOLSFOUNDATION_DLL ezMoveNodeCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMoveNodeCommand, ezCommand);

public:
  ezMoveNodeCommand();

public: // Properties
  ezUuid m_Object;
  ezVec2 m_NewPos;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override { }

private:
  ezDocumentObject* m_pObject;
  ezVec2 m_OldPos;
};


class EZ_TOOLSFOUNDATION_DLL ezConnectNodePinsCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConnectNodePinsCommand, ezCommand);

public:
  ezConnectNodePinsCommand();

public: // Properties
  ezUuid m_ObjectSource;
  ezUuid m_ObjectTarget;
  ezString m_sSourcePin;
  ezString m_sTargetPin;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override { }

private:
  ezDocumentObject* m_pObjectSource;
  ezDocumentObject* m_pObjectTarget;
};


class EZ_TOOLSFOUNDATION_DLL ezDisconnectNodePinsCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDisconnectNodePinsCommand, ezCommand);

public:
  ezDisconnectNodePinsCommand();

public: // Properties
  ezUuid m_ObjectSource;
  ezUuid m_ObjectTarget;
  ezString m_sSourcePin;
  ezString m_sTargetPin;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override { }

private:
  ezDocumentObject* m_pObjectSource;
  ezDocumentObject* m_pObjectTarget;
};
