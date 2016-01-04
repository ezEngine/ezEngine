#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Command/Command.h>

class ezCommandHistory;

class EZ_TOOLSFOUNDATION_DLL ezCommandTransaction : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCommandTransaction, ezCommand);

public:
  ezCommandTransaction() { }
  ~ezCommandTransaction();

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;
  ezStatus AddCommandTransaction(ezCommand* command);

private:
  friend class ezCommandHistory;
};

class EZ_TOOLSFOUNDATION_DLL ezCommandHistory
{
public:

  struct Event
  {
    enum class Type
    {
      UndoStarted,
      UndoEnded,
      RedoStarted,
      RedoEnded,
      TransactionStarted, ///< Emit after initial transaction started.
      BeforeTransactionEnded, ///< Emit before initial transaction ended.
      BeforeTransactionCanceled, ///< Emit before initial transaction ended.
      TransactionEnded, ///< Emit after initial transaction ended.
      TransactionCanceled, ///< Emit after initial transaction canceled.
    };

    Type m_Type;
  };

  ezEvent<const Event&> m_Events;

public:
  ezCommandHistory(ezDocument* pDocument);
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

  ezStatus AddCommand(ezCommand& command);

  void ClearUndoHistory();
  void ClearRedoHistory();

  void MergeLastTwoTransactions();

private:
  friend class ezCommand;

  void EndTransaction(bool bCancel);
  void EndTemporaryCommands(bool bCancel);

  bool m_bTemporaryMode;
  bool m_bTempTransaction;
  bool m_bIsInUndoRedo;

  ezHybridArray<ezCommandTransaction*, 4> m_TransactionStack;
  ezHybridArray<ezCommand*, 4> m_ActiveCommandStack;
  ezDeque<ezCommandTransaction*> m_UndoHistory;
  ezDeque<ezCommandTransaction*> m_RedoHistory;
  ezDocument* m_pDocument;
};
