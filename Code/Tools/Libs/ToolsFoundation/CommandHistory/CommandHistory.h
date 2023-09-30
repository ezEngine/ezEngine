#pragma once

#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class ezCommandHistory;

class EZ_TOOLSFOUNDATION_DLL ezCommandTransaction : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCommandTransaction, ezCommand);

public:
  ezCommandTransaction();
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
    TransactionStarted,        ///< Emit after initial transaction started.
    BeforeTransactionEnded,    ///< Emit before initial transaction ended.
    BeforeTransactionCanceled, ///< Emit before initial transaction ended.
    TransactionEnded,          ///< Emit after initial transaction ended.
    TransactionCanceled,       ///< Emit after initial transaction canceled.
    HistoryChanged,
  };

  Type m_Type;
  const ezDocument* m_pDocument;
};

/// \brief Stores the undo / redo stacks of transactions done on a document.
class EZ_TOOLSFOUNDATION_DLL ezCommandHistory
{
public:
  ezEvent<const ezCommandHistoryEvent&, ezMutex> m_Events;

  // \brief Storage for the command history so it can be swapped when using multiple sub documents.
  class Storage : public ezRefCounted
  {
  public:
    ezHybridArray<ezCommandTransaction*, 4> m_TransactionStack;
    ezHybridArray<ezCommand*, 4> m_ActiveCommandStack;
    ezDeque<ezCommandTransaction*> m_UndoHistory;
    ezDeque<ezCommandTransaction*> m_RedoHistory;
    ezDocument* m_pDocument = nullptr;
    ezEvent<const ezCommandHistoryEvent&, ezMutex> m_Events;
  };

public:
  ezCommandHistory(ezDocument* pDocument);
  ~ezCommandHistory();

  const ezDocument* GetDocument() const { return m_pHistoryStorage->m_pDocument; }

  ezStatus Undo(ezUInt32 uiNumEntries = 1);
  ezStatus Redo(ezUInt32 uiNumEntries = 1);

  bool CanUndo() const;
  bool CanRedo() const;

  ezStringView GetUndoDisplayString() const;
  ezStringView GetRedoDisplayString() const;

  void StartTransaction(const ezFormatString& displayString);
  void CancelTransaction() { EndTransaction(true); }
  void FinishTransaction() { EndTransaction(false); }

  /// \brief Returns true, if between StartTransaction / EndTransaction. False during Undo/Redo.
  bool IsInTransaction() const { return !m_pHistoryStorage->m_TransactionStack.IsEmpty(); }
  bool IsInUndoRedo() const { return m_bIsInUndoRedo; }

  /// \brief Call this to start a series of transactions that typically change the same value over and over (e.g. dragging an object to a position).
  /// Every time a new transaction is started, the previous one is undone first. At the end of a series of temporary transactions, only the last
  /// transaction will be stored as a single undo step. Call this first and then start a transaction inside it.
  void BeginTemporaryCommands(ezStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands = false);
  void CancelTemporaryCommands();
  void FinishTemporaryCommands();

  bool InTemporaryTransaction() const;
  void SuspendTemporaryTransaction();
  void ResumeTemporaryTransaction();

  ezStatus AddCommand(ezCommand& ref_command);

  void ClearUndoHistory();
  void ClearRedoHistory();

  void MergeLastTwoTransactions();

  ezUInt32 GetUndoStackSize() const;
  ezUInt32 GetRedoStackSize() const;
  const ezCommandTransaction* GetUndoStackEntry(ezUInt32 uiIndex) const;
  const ezCommandTransaction* GetRedoStackEntry(ezUInt32 uiIndex) const;

  ezSharedPtr<ezCommandHistory::Storage> SwapStorage(ezSharedPtr<ezCommandHistory::Storage> pNewStorage);
  ezSharedPtr<ezCommandHistory::Storage> GetStorage() { return m_pHistoryStorage; }

private:
  friend class ezCommand;

  ezStatus UndoInternal();
  ezStatus RedoInternal();

  void EndTransaction(bool bCancel);
  void EndTemporaryCommands(bool bCancel);

  ezSharedPtr<ezCommandHistory::Storage> m_pHistoryStorage;

  ezEvent<const ezCommandHistoryEvent&, ezMutex>::Unsubscriber m_EventsUnsubscriber;

  bool m_bFireEventsWhenUndoingTempCommands = false;
  bool m_bTemporaryMode = false;
  ezInt32 m_iTemporaryDepth = -1;
  ezInt32 m_iPreSuspendTemporaryDepth = -1;
  bool m_bIsInUndoRedo = false;
};
