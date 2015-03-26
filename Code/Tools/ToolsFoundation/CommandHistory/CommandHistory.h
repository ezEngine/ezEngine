#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Command/Command.h>

class ezCommandHistory;

class EZ_TOOLSFOUNDATION_DLL ezCommandTransaction : public ezCommandBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCommandTransaction);

public:
  ezCommandTransaction() { }
  ~ezCommandTransaction();

  virtual ezStatus Do(bool bRedo) override;
  virtual ezStatus Undo(bool bFireEvents) override;
  virtual void Cleanup(CommandState state) override;

private:
  friend class ezCommandHistory;
  ezStatus AddCommand(ezCommandBase& command);
  ezStatus AddCommand(ezCommandBase* command);

private:
  ezHybridArray<ezCommandBase*, 8> m_ChildActions;
};

class EZ_TOOLSFOUNDATION_DLL ezCommandHistory
{
public:
  ezCommandHistory(ezDocumentBase* pDocument);
  ~ezCommandHistory();
   
  ezStatus Undo();
  ezStatus Redo();

  bool CanUndo() const;
  bool CanRedo() const;

  void StartTransaction();
  void EndTransaction(bool bCancel);

  void BeginTemporaryCommands();
  void EndTemporaryCommands(bool bCancel);

  ezStatus AddCommand(ezCommandBase& command);

  void ClearUndoHistory();
  void ClearRedoHistory();

private:
  bool m_bTemporaryMode;
  bool m_bTempTransaction;

  ezHybridArray<ezCommandTransaction*, 4> m_TransactionStack;
  ezDeque<ezCommandTransaction*> m_UndoHistory;
  ezDeque<ezCommandTransaction*> m_RedoHistory;
  ezDocumentBase* m_pDocument;
};
