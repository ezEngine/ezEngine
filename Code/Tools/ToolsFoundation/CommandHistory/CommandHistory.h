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

  ezString m_sDisplayString;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;
  ezStatus AddCommandTransaction(ezCommand* command);

private:
  friend class ezCommandHistory;
};

struct ezCommandHistoryEvent
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
  ezDocument* m_pDocument;
};

class EZ_TOOLSFOUNDATION_DLL ezCommandHistory
{
public:

  ezEvent<const ezCommandHistoryEvent&> m_Events;

public:
  ezCommandHistory(ezDocument* pDocument);
  ~ezCommandHistory();
   
  const ezDocument* GetDocument() const { return m_pDocument; }

  ezStatus Undo(ezUInt32 uiNumEntries = 1);
  ezStatus Redo(ezUInt32 uiNumEntries = 1);

  bool CanUndo() const;
  bool CanRedo() const;

  const char* GetUndoDisplayString() const;
  const char* GetRedoDisplayString() const;

  void StartTransaction(const char* szDisplayString);
  void CancelTransaction() { EndTransaction(true); }
  void FinishTransaction() { EndTransaction(false); }

  /// \brief Returns true, if between StartTransaction / EndTransaction. False during Undo/Redo.
  bool IsInTransaction() const { return !m_TransactionStack.IsEmpty(); }
  bool IsInUndoRedo() const { return m_bIsInUndoRedo; }

  /// \brief Call this to start a serious of transactions that typically change the same value over and over (e.g. dragging an object to a position).
  /// Every time a new transaction is started, the previous one is undone first. At the end of a serious of temporary transactions, only the last transaction will be stored as a single undo step.
  /// Call this first and then start a transaction inside it.
  void BeginTemporaryCommands(const char* szDisplayString, bool bFireEventsWhenUndoingTempCommands = false);
  void CancelTemporaryCommands() { EndTemporaryCommands(true); }
  void FinishTemporaryCommands() { EndTemporaryCommands(false); }

  ezStatus AddCommand(ezCommand& command);

  void ClearUndoHistory();
  void ClearRedoHistory();

  void MergeLastTwoTransactions();

  ezUInt32 GetUndoStackSize() const;
  ezUInt32 GetRedoStackSize() const;
  const ezCommandTransaction* GetUndoStackEntry(ezUInt32 iIndex) const;
  const ezCommandTransaction* GetRedoStackEntry(ezUInt32 iIndex) const;

private:
  friend class ezCommand;

  ezStatus UndoInternal();
  ezStatus RedoInternal();

  void EndTransaction(bool bCancel);
  void EndTemporaryCommands(bool bCancel);

  bool m_bFireEventsWhenUndoingTempCommands;
  bool m_bTemporaryMode;
  bool m_bTempTransaction;
  bool m_bIsInUndoRedo;

  ezHybridArray<ezCommandTransaction*, 4> m_TransactionStack;
  ezHybridArray<ezCommand*, 4> m_ActiveCommandStack;
  ezDeque<ezCommandTransaction*> m_UndoHistory;
  ezDeque<ezCommandTransaction*> m_RedoHistory;
  ezDocument* m_pDocument;
};
