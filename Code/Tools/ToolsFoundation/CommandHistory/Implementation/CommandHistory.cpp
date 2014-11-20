#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>

ezStatus ezCommandTransaction::Do(bool bRedo)
{
  const ezUInt32 uiChildActions = m_ChildActions.GetCount();
  for (ezUInt32 i = 0; i < uiChildActions; ++i)
  {
    if (m_ChildActions[i]->Do(bRedo).m_Result == EZ_FAILURE)
    {
      for (ezUInt32 j = i - 1; j > 0; --j)
      {
        ezStatus status = m_ChildActions[j]->Undo();
        EZ_ASSERT(status.m_Result == EZ_SUCCESS, "Failed redo could not be recovered! Inconsistent state!");
      }
      // A command that originally succeeded failed on redo!
      return ezStatus(EZ_FAILURE);
    }
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCommandTransaction::Undo()
{
  const ezUInt32 uiChildActions = m_ChildActions.GetCount();
  for (ezUInt32 i = uiChildActions - 1; i > 0; --i)
  {
    if (m_ChildActions[i]->Undo().m_Result == EZ_FAILURE)
    {
      for (ezUInt32 j = i + 1; j < uiChildActions; ++j)
      {
        ezStatus status = m_ChildActions[j]->Do(true);
        EZ_ASSERT(status.m_Result == EZ_SUCCESS, "Failed undo could not be recovered! Inconsistent state!");
      }
      // A command that originally succeeded failed on redo!
      return ezStatus(EZ_FAILURE);
    }
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCommandTransaction::AddCommand(ezCommandBase& command)
{
  // serialize

  // de-serialize

  // execute
  
  return ezStatus(EZ_FAILURE);
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
}

ezStatus ezCommandHistory::Undo()
{
  EZ_ASSERT(m_TransactionStack.IsEmpty(), "Can't undo with active transaction!");
  EZ_ASSERT(!m_UndoHistory.IsEmpty(), "Can't undo with empty undo queue!");

  ezCommandTransaction* pTransaction = m_UndoHistory.PeekBack();
  
  if (pTransaction->Undo().m_Result == EZ_SUCCESS)
  {
    m_UndoHistory.PopBack();
    m_RedoHistory.PushBack(pTransaction);
    return ezStatus(EZ_SUCCESS);
  }
  
  return ezStatus(EZ_FAILURE);
}

ezStatus ezCommandHistory::Redo()
{
  EZ_ASSERT(m_TransactionStack.IsEmpty(), "Can't redo with active transaction!");
  EZ_ASSERT(!m_RedoHistory.IsEmpty(), "Can't redo with empty undo queue!");

  ezCommandTransaction* pTransaction = m_RedoHistory.PeekBack();
  
  if (pTransaction->Do(true).m_Result == EZ_SUCCESS)
  {
    m_RedoHistory.PopBack();
    m_UndoHistory.PushBack(pTransaction);
    return ezStatus(EZ_SUCCESS);
  }
  
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

ezCommandTransaction* ezCommandHistory::StartTransaction()
{
  ezCommandTransaction* pTransaction = new ezCommandTransaction();
  pTransaction->m_pDocument = m_pDocument;

  if (!m_TransactionStack.IsEmpty())
  {
    m_TransactionStack.PeekBack()->AddCommand(pTransaction);
  }
  m_TransactionStack.PushBack(pTransaction);

  return pTransaction;
}

void ezCommandHistory::EndTransaction()
{
  EZ_ASSERT(!m_TransactionStack.IsEmpty(), "Trying to end transaction without starting one!");
  if (m_TransactionStack.GetCount() > 1)
  {
    m_TransactionStack.PopBack();
  }
  else
  {
    m_UndoHistory.PushBack(m_TransactionStack.PeekBack());
    m_TransactionStack.PopBack();
    ClearRedoHistory();
    // TODO: Fire event
  }
}

void ezCommandHistory::CancelTransaction()
{
  EZ_ASSERT(!m_TransactionStack.IsEmpty(), "Trying to cancel transaction without starting one!");
  m_TransactionStack.PeekBack()->Undo();
  m_TransactionStack.PopBack();
}

void ezCommandHistory::ClearUndoHistory()
{
  while (!m_UndoHistory.IsEmpty())
  {
    delete m_UndoHistory.PeekBack();
    m_UndoHistory.PopBack();
  }
}

void ezCommandHistory::ClearRedoHistory()
{
  while (!m_RedoHistory.IsEmpty())
  {
    delete m_RedoHistory.PeekBack();
    m_RedoHistory.PopBack();
  }
}
