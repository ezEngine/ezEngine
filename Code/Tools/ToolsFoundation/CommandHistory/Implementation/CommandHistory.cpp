#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/IO/MemoryStream.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCommandTransaction, ezCommandBase, 1, ezRTTIDefaultAllocator<ezCommandTransaction>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezCommandTransaction::~ezCommandTransaction()
{
  EZ_ASSERT_DEV(m_ChildActions.IsEmpty(), "The list should be cleared in 'Cleanup'");
}

ezStatus ezCommandTransaction::Do(bool bRedo)
{
  EZ_ASSERT_DEV(bRedo == true, "Implementation error");

  const ezUInt32 uiChildActions = m_ChildActions.GetCount();
  for (ezUInt32 i = 0; i < uiChildActions; ++i)
  {
    if (m_ChildActions[i]->Do(bRedo).m_Result == EZ_FAILURE)
    {
      for (ezInt32 j = i - 1; j >= 0; --j)
      {
        ezStatus status = m_ChildActions[j]->Undo(true);
        EZ_ASSERT_DEV(status.m_Result == EZ_SUCCESS, "Failed redo could not be recovered! Inconsistent state!");
      }
      // A command that originally succeeded failed on redo!
      return ezStatus(EZ_FAILURE);
    }
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCommandTransaction::Undo(bool bFireEvents)
{
  const ezUInt32 uiChildActions = m_ChildActions.GetCount();
  for (ezInt32 i = uiChildActions - 1; i >= 0; --i)
  {
    if (m_ChildActions[i]->Undo(bFireEvents).m_Result == EZ_FAILURE)
    {
      for (ezUInt32 j = i + 1; j < uiChildActions; ++j)
      {
        ezStatus status = m_ChildActions[j]->Do(true);
        EZ_ASSERT_DEV(status.m_Result == EZ_SUCCESS, "Failed undo could not be recovered! Inconsistent state!");
      }
      // A command that originally succeeded failed on redo!
      return ezStatus(EZ_FAILURE);
    }
  }
  return ezStatus(EZ_SUCCESS);
}

void ezCommandTransaction::Cleanup(CommandState state)
{
  for (ezCommandBase* pCommand : m_ChildActions)
  {
    pCommand->Cleanup(state);
    pCommand->GetDynamicRTTI()->GetAllocator()->Deallocate(pCommand);
  }

  m_ChildActions.Clear();
}

ezStatus ezCommandTransaction::AddCommand(ezCommandBase& command)
{
  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezMemoryStreamReader reader(&storage);

  /// \todo Clone action, instead of writing to JSON and then reading from it again

  ezReflectionSerializer::WriteObjectToJSON(writer, command.GetDynamicRTTI(), &command, ezJSONWriter::WhitespaceMode::None);

  const ezRTTI* pRtti;
  ezCommandBase* pCommand = (ezCommandBase*) ezReflectionSerializer::ReadObjectFromJSON(reader, pRtti);

  pCommand->m_pDocument = m_pDocument;
  ezStatus ret = pCommand->Do(false);

  if (ret.m_Result == EZ_FAILURE)
  {
    pCommand->Cleanup(ezCommandBase::CommandState::WasDone);
    pCommand->GetDynamicRTTI()->GetAllocator()->Deallocate(pCommand);
    return ret;
  }

  m_ChildActions.PushBack(pCommand);
  
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCommandTransaction::AddCommand(ezCommandBase* pCommand)
{
  pCommand->m_pDocument = m_pDocument;
  ezStatus res = pCommand->Do(false);
  if (res.m_Result == EZ_SUCCESS)
  {
    m_ChildActions.PushBack(pCommand);
    return ezStatus(EZ_SUCCESS);
  }
  return ezStatus(EZ_FAILURE);
}

ezCommandHistory::ezCommandHistory(ezDocumentBase* pDocument) : m_pDocument(pDocument)
{
  m_bTemporaryMode = false;
  m_bIsInUndoRedo = false;
}

ezCommandHistory::~ezCommandHistory()
{
  EZ_ASSERT_ALWAYS(m_UndoHistory.IsEmpty(), "Must clear history before destructor as object manager will be dead already");
  EZ_ASSERT_ALWAYS(m_RedoHistory.IsEmpty(), "Must clear history before destructor as object manager will be dead already");
}

void ezCommandHistory::BeginTemporaryCommands()
{
  EZ_ASSERT_DEV(!m_bTemporaryMode, "Temporary Mode cannot be nested");

  if (m_TransactionStack.IsEmpty())
  {
    StartTransaction();
    m_bTempTransaction = true;
  }

  m_bTemporaryMode = true;
}

void ezCommandHistory::EndTemporaryCommands(bool bCancel)
{
  EZ_ASSERT_DEV(m_bTemporaryMode, "Temporary Mode was not enabled");

  m_bTemporaryMode = false;

  if (m_bTempTransaction)
  {
    EndTransaction(bCancel);
    m_bTempTransaction = false;

    EZ_ASSERT_DEV(m_TransactionStack.IsEmpty(), "Transaction stack should be empty now");
  }
}

ezStatus ezCommandHistory::Undo()
{
  EZ_ASSERT_DEV(!m_bIsInUndoRedo, "invalidly nested undo/redo");

  m_bIsInUndoRedo = true;

  EZ_ASSERT_DEV(m_TransactionStack.IsEmpty(), "Can't undo with active transaction!");
  EZ_ASSERT_DEV(!m_UndoHistory.IsEmpty(), "Can't undo with empty undo queue!");

  ezCommandTransaction* pTransaction = m_UndoHistory.PeekBack();
  
  if (pTransaction->Undo(true).m_Result == EZ_SUCCESS)
  {
    m_UndoHistory.PopBack();
    m_RedoHistory.PushBack(pTransaction);

    m_pDocument->SetModified(true);

    Event e;
    e.m_Type = Event::Type::ExecutedUndo;
    m_Events.Broadcast(e);

    m_bIsInUndoRedo = false;
    return ezStatus(EZ_SUCCESS);
  }
  
  m_bIsInUndoRedo = false;
  return ezStatus(EZ_FAILURE);
}

ezStatus ezCommandHistory::Redo()
{
  EZ_ASSERT_DEV(!m_bIsInUndoRedo, "invalidly nested undo/redo");

  m_bIsInUndoRedo = true;
  EZ_ASSERT_DEV(m_TransactionStack.IsEmpty(), "Can't redo with active transaction!");
  EZ_ASSERT_DEV(!m_RedoHistory.IsEmpty(), "Can't redo with empty undo queue!");

  ezCommandTransaction* pTransaction = m_RedoHistory.PeekBack();
  
  if (pTransaction->Do(true).m_Result == EZ_SUCCESS)
  {
    m_RedoHistory.PopBack();
    m_UndoHistory.PushBack(pTransaction);

    m_pDocument->SetModified(true);

    Event e;
    e.m_Type = Event::Type::ExecutedRedo;
    m_Events.Broadcast(e);

    m_bIsInUndoRedo = false;
    return ezStatus(EZ_SUCCESS);
  }
  
  m_bIsInUndoRedo = false;
  return ezStatus(EZ_FAILURE);
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

void ezCommandHistory::StartTransaction()
{
  /// \todo Allow to have a limited transaction history and clean up transactions after a while

  ezCommandTransaction* pTransaction;

  if (m_bTemporaryMode && !m_TransactionStack.IsEmpty())
  {
    pTransaction = m_TransactionStack.PeekBack();
    pTransaction->Undo(false);
    pTransaction->Cleanup(ezCommandBase::CommandState::WasUndone);
    m_TransactionStack.PushBack(pTransaction);
    return;
  }

  pTransaction = (ezCommandTransaction*) ezGetStaticRTTI<ezCommandTransaction>()->GetAllocator()->Allocate();
  pTransaction->m_pDocument = m_pDocument;

  if (!m_TransactionStack.IsEmpty())
  {
    m_TransactionStack.PeekBack()->AddCommand(pTransaction);
  }
  m_TransactionStack.PushBack(pTransaction);

  return;
}

void ezCommandHistory::EndTransaction(bool bCancel)
{
  Event et;
  et.m_Type = bCancel ? Event::Type::BeforeEndTransactionCancel : Event::Type::BeforeEndTransaction;
  m_Events.Broadcast(et);

  EZ_ASSERT_DEV(!m_TransactionStack.IsEmpty(), "Trying to end transaction without starting one!");

  if (!bCancel)
  {
    if (m_TransactionStack.GetCount() > 1)
    {
      m_TransactionStack.PopBack();
    }
    else
    {
      m_UndoHistory.PushBack(m_TransactionStack.PeekBack());
      m_TransactionStack.PopBack();
      ClearRedoHistory();

      m_pDocument->SetModified(true);

      Event e;
      e.m_Type = Event::Type::NewTransation;
      m_Events.Broadcast(e);
    }
  }
  else
  {
    ezCommandTransaction* pTransaction = m_TransactionStack.PeekBack();

    pTransaction->Undo(true);
    m_TransactionStack.PopBack();

    if (m_TransactionStack.IsEmpty())
    {
      pTransaction->Cleanup(ezCommandBase::CommandState::WasUndone);
      pTransaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pTransaction);
    }
  }

  et.m_Type = Event::Type::AfterEndTransaction;
  m_Events.Broadcast(et);
}

ezStatus ezCommandHistory::AddCommand(ezCommandBase& command)
{
  EZ_ASSERT_DEV(!m_TransactionStack.IsEmpty(), "Cannot add command while no transaction is started");

  return m_TransactionStack.PeekBack()->AddCommand(command);
}

void ezCommandHistory::ClearUndoHistory()
{
  while (!m_UndoHistory.IsEmpty())
  {
    ezCommandTransaction* pTransaction = m_UndoHistory.PeekBack();

    pTransaction->Cleanup(ezCommandBase::CommandState::WasDone);
    pTransaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pTransaction);

    m_UndoHistory.PopBack();
  }
}

void ezCommandHistory::ClearRedoHistory()
{
  while (!m_RedoHistory.IsEmpty())
  {
    ezCommandTransaction* pTransaction = m_RedoHistory.PeekBack();

    pTransaction->Cleanup(ezCommandBase::CommandState::WasUndone);
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


