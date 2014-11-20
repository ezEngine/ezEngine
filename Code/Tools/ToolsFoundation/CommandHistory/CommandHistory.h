#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Command/Command.h>

class ezCommandHistory;

class EZ_TOOLSFOUNDATION_DLL ezCommandTransaction : public ezCommandBase
{
public:
  virtual ezStatus Do(bool bRedo) override;
  virtual ezStatus Undo() override;

  ezStatus AddCommand(ezCommandBase& command);

private:
  friend class ezCommandHistory;
  ezStatus AddCommand(ezCommandBase* command);

private:
  ezHybridArray<ezCommandBase*, 8> m_ChildActions;
  ezHybridArray<ezCommandBase*, 8> m_CreatedActions;
};

class EZ_TOOLSFOUNDATION_DLL ezCommandHistory
{
public:
  ezCommandHistory(ezDocumentBase* pDocument);
   
  ezStatus Undo();
  ezStatus Redo();

  bool CanUndo() const;
  bool CanRedo() const;

  ezCommandTransaction* StartTransaction();
  void EndTransaction();
  void CancelTransaction();


private:
  void ClearUndoHistory();
  void ClearRedoHistory();

private:
  ezHybridArray<ezCommandTransaction*, 4> m_TransactionStack;
  ezDeque<ezCommandTransaction*> m_UndoHistory;
  ezDeque<ezCommandTransaction*> m_RedoHistory;
  ezDocumentBase* m_pDocument;
};
