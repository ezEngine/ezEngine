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

  struct Event
  {
    enum class Type
    {
      ExecutedUndo,
      ExecutedRedo,
      NewTransation,
      BeforeEndTransaction,
      BeforeEndTransactionCancel,
      AfterEndTransaction,
    };

    Type m_Type;
  };

  ezEvent<const Event&> m_Events;

public:
  ezCommandHistory(ezDocumentBase* pDocument);
  ~ezCommandHistory();
   
  ezStatus Undo();
  ezStatus Redo();

  bool CanUndo() const;
  bool CanRedo() const;

  void StartTransaction();
  void CancelTransaction() { EndTransaction(true); }
  void FinishTransaction() { EndTransaction(false); }

  /// \brief Returns true, if between StartTransaction / EndTransaction. False during Undo/Redo.
  bool IsInTransaction() const { return !m_TransactionStack.IsEmpty(); }
  bool IsInUndoRedo() const { return m_bIsInUndoRedo; }

  void BeginTemporaryCommands();
  void CancelTemporaryCommands() { EndTemporaryCommands(true); }
  void FinishTemporaryCommands() { EndTemporaryCommands(false); }

  ezStatus AddCommand(ezCommandBase& command);

  void ClearUndoHistory();
  void ClearRedoHistory();

  void MergeLastTwoTransactions();

private:
  void EndTransaction(bool bCancel);
  void EndTemporaryCommands(bool bCancel);

  bool m_bTemporaryMode;
  bool m_bTempTransaction;
  bool m_bIsInUndoRedo;

  ezHybridArray<ezCommandTransaction*, 4> m_TransactionStack;
  ezDeque<ezCommandTransaction*> m_UndoHistory;
  ezDeque<ezCommandTransaction*> m_RedoHistory;
  ezDocumentBase* m_pDocument;
};
