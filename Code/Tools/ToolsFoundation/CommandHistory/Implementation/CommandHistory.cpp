#include <PCH.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCommandTransaction, 1, ezRTTIDefaultAllocator<ezCommandTransaction>);
EZ_END_DYNAMIC_REFLECTED_TYPE

////////////////////////////////////////////////////////////////////////
// ezCommandTransaction
////////////////////////////////////////////////////////////////////////

ezCommandTransaction::~ezCommandTransaction()
{
  EZ_ASSERT_DEV(m_ChildActions.IsEmpty(), "The list should be cleared in 'Cleanup'");
}

ezStatus ezCommandTransaction::DoInternal(bool bRedo)
{
  EZ_ASSERT_DEV(bRedo == true, "Implementation error");
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCommandTransaction::UndoInternal(bool bFireEvents)
{
  return ezStatus(EZ_SUCCESS);
}

void ezCommandTransaction::CleanupInternal(CommandState state)
{
}

ezStatus ezCommandTransaction::AddCommandTransaction(ezCommand* pCommand)
{
  pCommand->m_pDocument = m_pDocument;
  m_ChildActions.PushBack(pCommand);
  return ezStatus(EZ_SUCCESS);
}

////////////////////////////////////////////////////////////////////////
// ezCommandHistory
////////////////////////////////////////////////////////////////////////

ezCommandHistory::ezCommandHistory(ezDocument* pDocument) : m_pDocument(pDocument)
{
  m_bTemporaryMode = false;
  m_bIsInUndoRedo = false;
}

ezCommandHistory::~ezCommandHistory()
{
  EZ_ASSERT_ALWAYS(m_UndoHistory.IsEmpty(), "Must clear history before destructor as object manager will be dead already");
  EZ_ASSERT_ALWAYS(m_RedoHistory.IsEmpty(), "Must clear history before destructor as object manager will be dead already");
}

void ezCommandHistory::BeginTemporaryCommands(const char* szDisplayString, bool bFireEventsWhenUndoingTempCommands)
{
  EZ_ASSERT_DEV(!m_bTemporaryMode, "Temporary Mode cannot be nested");
  StartTransaction(szDisplayString);
  StartTransaction("[Temporary]");

  m_bFireEventsWhenUndoingTempCommands = bFireEventsWhenUndoingTempCommands;
  m_bTemporaryMode = true;
  m_iTemporaryDepth = (ezInt32)m_TransactionStack.GetCount();
}

void ezCommandHistory::CancelTemporaryCommands()
{
  EndTemporaryCommands(true);
  EndTransaction(true);
}

void ezCommandHistory::FinishTemporaryCommands()
{
  EndTemporaryCommands(false);
  EndTransaction(false);
}

bool ezCommandHistory::InTemporaryTransaction() const
{
  return m_bTemporaryMode;
}


void ezCommandHistory::SuspendTemporaryTransaction()
{
  m_iPreSuspendTemporaryDepth = (ezInt32)m_TransactionStack.GetCount();
  EZ_ASSERT_DEV(m_bTemporaryMode, "No temporary transaction active.");
  while (m_iTemporaryDepth < (ezInt32)m_TransactionStack.GetCount())
  {
    EndTransaction(true);
  }
  EndTemporaryCommands(true);
}

void ezCommandHistory::ResumeTemporaryTransaction()
{
  EZ_ASSERT_DEV(m_iTemporaryDepth == (ezInt32)m_TransactionStack.GetCount() + 1, "Can't resume temporary, not before temporary depth.");
  while (m_iPreSuspendTemporaryDepth > (ezInt32)m_TransactionStack.GetCount())
  {
    StartTransaction("[Temporary]");
  }
  m_bTemporaryMode = true;
  EZ_ASSERT_DEV(m_iPreSuspendTemporaryDepth == (ezInt32)m_TransactionStack.GetCount(), "");
}

void ezCommandHistory::EndTemporaryCommands(bool bCancel)
{
  EZ_ASSERT_DEV(m_bTemporaryMode, "Temporary Mode was not enabled");
  EZ_ASSERT_DEV(m_iTemporaryDepth == (ezInt32)m_TransactionStack.GetCount(), "Transaction stack is at depth {0} but temporary is at {1}",
    m_TransactionStack.GetCount(), m_iTemporaryDepth);
  m_bTemporaryMode = false;

  EndTransaction(bCancel);
}

ezStatus ezCommandHistory::UndoInternal()
{
  EZ_ASSERT_DEV(!m_bIsInUndoRedo, "invalidly nested undo/redo");
  EZ_ASSERT_DEV(m_TransactionStack.IsEmpty(), "Can't undo with active transaction!");
  EZ_ASSERT_DEV(!m_UndoHistory.IsEmpty(), "Can't undo with empty undo queue!");

  m_bIsInUndoRedo = true;
  {
    ezCommandHistoryEvent e;
    e.m_pDocument = m_pDocument;
    e.m_Type = ezCommandHistoryEvent::Type::UndoStarted;
    m_Events.Broadcast(e);
  }

  ezCommandTransaction* pTransaction = m_UndoHistory.PeekBack();

  ezStatus status = pTransaction->Undo(true);
  if (status.m_Result == EZ_SUCCESS)
  {
    m_UndoHistory.PopBack();
    m_RedoHistory.PushBack(pTransaction);

    m_pDocument->SetModified(true);

    status = ezStatus(EZ_SUCCESS);
  }

  m_bIsInUndoRedo = false;
  {
    ezCommandHistoryEvent e;
    e.m_pDocument = m_pDocument;
    e.m_Type = ezCommandHistoryEvent::Type::UndoEnded;
    m_Events.Broadcast(e);
  }
  return status;
}

ezStatus ezCommandHistory::Undo(ezUInt32 uiNumEntries)
{
  for (ezUInt32 i = 0; i < uiNumEntries; i++)
  {
    EZ_SUCCEED_OR_RETURN(UndoInternal());
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCommandHistory::RedoInternal()
{
  EZ_ASSERT_DEV(!m_bIsInUndoRedo, "invalidly nested undo/redo");
  EZ_ASSERT_DEV(m_TransactionStack.IsEmpty(), "Can't redo with active transaction!");
  EZ_ASSERT_DEV(!m_RedoHistory.IsEmpty(), "Can't redo with empty undo queue!");

  m_bIsInUndoRedo = true;
  {
    ezCommandHistoryEvent e;
    e.m_pDocument = m_pDocument;
    e.m_Type = ezCommandHistoryEvent::Type::RedoStarted;
    m_Events.Broadcast(e);
  }

  ezCommandTransaction* pTransaction = m_RedoHistory.PeekBack();

  ezStatus status(EZ_FAILURE);
  if (pTransaction->Do(true).m_Result == EZ_SUCCESS)
  {
    m_RedoHistory.PopBack();
    m_UndoHistory.PushBack(pTransaction);

    m_pDocument->SetModified(true);

    status = ezStatus(EZ_SUCCESS);
  }

  m_bIsInUndoRedo = false;
  {
    ezCommandHistoryEvent e;
    e.m_pDocument = m_pDocument;
    e.m_Type = ezCommandHistoryEvent::Type::RedoEnded;
    m_Events.Broadcast(e);
  }
  return status;
}

ezStatus ezCommandHistory::Redo(ezUInt32 uiNumEntries)
{
  for (ezUInt32 i = 0; i < uiNumEntries; i++)
  {
    EZ_SUCCEED_OR_RETURN(RedoInternal());
  }

  return ezStatus(EZ_SUCCESS);
}

bool ezCommandHistory::CanUndo() const
{
  if (!m_TransactionStack.IsEmpty())
    return false;

  return !m_UndoHistory.IsEmpty();
}

bool ezCommandHistory::CanRedo() const
{
  if (!m_TransactionStack.IsEmpty())
    return false;

  return !m_RedoHistory.IsEmpty();
}


const char* ezCommandHistory::GetUndoDisplayString() const
{
  if (m_UndoHistory.IsEmpty())
    return "";

  return m_UndoHistory.PeekBack()->m_sDisplayString;
}


const char* ezCommandHistory::GetRedoDisplayString() const
{
  if (m_RedoHistory.IsEmpty())
    return "";

  return m_RedoHistory.PeekBack()->m_sDisplayString;
}

void ezCommandHistory::StartTransaction(const ezFormatString& sDisplayString)
{
  EZ_ASSERT_DEV(!m_bIsInUndoRedo, "Cannot start new transaction while redoing/undoing.");

  /// \todo Allow to have a limited transaction history and clean up transactions after a while

  ezCommandTransaction* pTransaction;

  if (m_bTemporaryMode && !m_TransactionStack.IsEmpty())
  {
    pTransaction = m_TransactionStack.PeekBack();
    pTransaction->Undo(m_bFireEventsWhenUndoingTempCommands);
    pTransaction->Cleanup(ezCommand::CommandState::WasUndone);
    m_TransactionStack.PushBack(pTransaction);
    m_ActiveCommandStack.PushBack(pTransaction);
    return;
  }

  ezStringBuilder tmp;

  pTransaction = (ezCommandTransaction*) ezGetStaticRTTI<ezCommandTransaction>()->GetAllocator()->Allocate();
  pTransaction->m_pDocument = m_pDocument;
  pTransaction->m_sDisplayString = sDisplayString.GetText(tmp);

  if (!m_TransactionStack.IsEmpty())
  {
    // Stacked transaction
    m_TransactionStack.PeekBack()->AddCommandTransaction(pTransaction);
    m_TransactionStack.PushBack(pTransaction);
    m_ActiveCommandStack.PushBack(pTransaction);
  }
  else
  {
    // Initial transaction
    m_TransactionStack.PushBack(pTransaction);
    m_ActiveCommandStack.PushBack(pTransaction);
    {
      ezCommandHistoryEvent e;
      e.m_pDocument = m_pDocument;
      e.m_Type = ezCommandHistoryEvent::Type::TransactionStarted;
      m_Events.Broadcast(e);
    }
  }
  return;
}

void ezCommandHistory::EndTransaction(bool bCancel)
{
  EZ_ASSERT_DEV(!m_TransactionStack.IsEmpty(), "Trying to end transaction without starting one!");

  if (m_TransactionStack.GetCount() == 1)
  {
    /// Empty transactions are always canceled, so that they do not create an unnecessary undo action and clear the redo stack

    const bool bDidAnything = m_TransactionStack.PeekBack()->HasChildActions();
    if (!bDidAnything)
      bCancel = true;

    ezCommandHistoryEvent e;
    e.m_pDocument = m_pDocument;
    e.m_Type = bCancel ? ezCommandHistoryEvent::Type::BeforeTransactionCanceled : ezCommandHistoryEvent::Type::BeforeTransactionEnded;
    m_Events.Broadcast(e);
  }

  if (!bCancel)
  {
    if (m_TransactionStack.GetCount() > 1)
    {
      m_TransactionStack.PopBack();
      m_ActiveCommandStack.PopBack();
    }
    else
    {
      const bool bDidAnything = m_TransactionStack.PeekBack()->HasChildActions();
      m_UndoHistory.PushBack(m_TransactionStack.PeekBack());
      m_TransactionStack.PopBack();
      m_ActiveCommandStack.PopBack();
      ClearRedoHistory();

      if (bDidAnything)
      {
        m_pDocument->SetModified(true);
      }
    }
  }
  else
  {
    ezCommandTransaction* pTransaction = m_TransactionStack.PeekBack();

    pTransaction->Undo(true);
    m_TransactionStack.PopBack();
    m_ActiveCommandStack.PopBack();

    if (m_TransactionStack.IsEmpty())
    {
      pTransaction->Cleanup(ezCommand::CommandState::WasUndone);
      pTransaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pTransaction);
    }
  }

  if (m_TransactionStack.IsEmpty())
  {
    // All transactions done
    ezCommandHistoryEvent e;
    e.m_pDocument = m_pDocument;
    e.m_Type = bCancel ? ezCommandHistoryEvent::Type::TransactionCanceled : ezCommandHistoryEvent::Type::TransactionEnded;
    m_Events.Broadcast(e);
  }
}

ezStatus ezCommandHistory::AddCommand(ezCommand& command)
{
  EZ_ASSERT_DEV(!m_TransactionStack.IsEmpty(), "Cannot add command while no transaction is started");
  EZ_ASSERT_DEV(!m_ActiveCommandStack.IsEmpty(), "Transaction stack is not synced anymore with m_ActiveCommandStack");

  return m_ActiveCommandStack.PeekBack()->AddSubCommand(command);
}

void ezCommandHistory::ClearUndoHistory()
{
  EZ_ASSERT_DEV(!m_bIsInUndoRedo, "Cannot clear undo/redo history while redoing/undoing.");
  while (!m_UndoHistory.IsEmpty())
  {
    ezCommandTransaction* pTransaction = m_UndoHistory.PeekBack();

    pTransaction->Cleanup(ezCommand::CommandState::WasDone);
    pTransaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pTransaction);

    m_UndoHistory.PopBack();
  }
}

void ezCommandHistory::ClearRedoHistory()
{
  EZ_ASSERT_DEV(!m_bIsInUndoRedo, "Cannot clear undo/redo history while redoing/undoing.");
  while (!m_RedoHistory.IsEmpty())
  {
    ezCommandTransaction* pTransaction = m_RedoHistory.PeekBack();

    pTransaction->Cleanup(ezCommand::CommandState::WasUndone);
    pTransaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pTransaction);

    m_RedoHistory.PopBack();
  }
}

void ezCommandHistory::MergeLastTwoTransactions()
{
  /// \todo This would not be necessary, if hierarchical transactions would not crash

  EZ_ASSERT_DEV(m_RedoHistory.IsEmpty(), "This can only be called directly after EndTransaction, when the redo history is empty");
  EZ_ASSERT_DEV(m_UndoHistory.GetCount() >= 2, "Can only do this when at least two transcations are in the queue");

  ezCommandTransaction* pLast = m_UndoHistory.PeekBack();
  m_UndoHistory.PopBack();

  ezCommandTransaction* pNowLast = m_UndoHistory.PeekBack();
  pNowLast->m_ChildActions.PushBackRange(pLast->m_ChildActions);

  pLast->m_ChildActions.Clear();

  pLast->GetDynamicRTTI()->GetAllocator()->Deallocate(pLast);
}

ezUInt32 ezCommandHistory::GetUndoStackSize() const
{
  return m_UndoHistory.GetCount();
}

ezUInt32 ezCommandHistory::GetRedoStackSize() const
{
  return m_RedoHistory.GetCount();
}

const ezCommandTransaction* ezCommandHistory::GetUndoStackEntry(ezUInt32 iIndex) const
{
  return m_UndoHistory[GetUndoStackSize() - 1 - iIndex];
}

const ezCommandTransaction* ezCommandHistory::GetRedoStackEntry(ezUInt32 iIndex) const
{
  return m_RedoHistory[GetRedoStackSize() - 1 - iIndex];
}


