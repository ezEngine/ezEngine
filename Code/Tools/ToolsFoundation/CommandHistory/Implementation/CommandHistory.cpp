#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/IO/MemoryStream.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCommandTransaction, ezCommandBase, 1, ezRTTIDefaultAllocator<ezCommandTransaction>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezCommandTransaction::~ezCommandTransaction()
{
  for (ezCommandBase* pCommand : m_ChildActions)
  {
    pCommand->GetDynamicRTTI()->GetAllocator()->Deallocate(pCommand);
  }
}

ezStatus ezCommandTransaction::Do(bool bRedo)
{
  EZ_ASSERT(bRedo == true, "Implementation error");

  const ezUInt32 uiChildActions = m_ChildActions.GetCount();
  for (ezUInt32 i = 0; i < uiChildActions; ++i)
  {
    if (m_ChildActions[i]->Do(bRedo).m_Result == EZ_FAILURE)
    {
      for (ezInt32 j = i - 1; j >= 0; --j)
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
  for (ezInt32 i = uiChildActions - 1; i >= 0; --i)
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

void ezCommandTransaction::Cleanup(CommandState state)
{
  for (ezCommandBase* pCommand : m_ChildActions)
  {
    pCommand->Cleanup(state);
  }
}

ezStatus ezCommandTransaction::AddCommand(ezCommandBase& command)
{
  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezMemoryStreamReader reader(&storage);

  ezReflectionUtils::WriteObjectToJSON(writer, command.GetDynamicRTTI(), &command, ezJSONWriter::WhitespaceMode::None);

  const ezRTTI* pRtti;
  ezCommandBase* pCommand = (ezCommandBase*) ezReflectionUtils::ReadObjectFromJSON(reader, pRtti);

  pCommand->m_pDocument = m_pDocument;
  ezStatus ret = pCommand->Do(false);

  if (ret.m_Result == EZ_FAILURE)
  {
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

    m_pDocument->SetModified(true);
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

    m_pDocument->SetModified(true);
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
  ezCommandTransaction* pTransaction = (ezCommandTransaction*) ezGetStaticRTTI<ezCommandTransaction>()->GetAllocator()->Allocate();
  pTransaction->m_pDocument = m_pDocument;

  if (!m_TransactionStack.IsEmpty())
  {
    m_TransactionStack.PeekBack()->AddCommand(pTransaction);
  }
  m_TransactionStack.PushBack(pTransaction);

  return pTransaction;
}

void ezCommandHistory::EndTransaction(bool bCancel)
{
  EZ_ASSERT(!m_TransactionStack.IsEmpty(), "Trying to end transaction without starting one!");

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
    }
  }
  else
  {
    ezCommandTransaction* pTransaction = m_TransactionStack.PeekBack();

    pTransaction->Undo();
    m_TransactionStack.PopBack();

    if (m_TransactionStack.IsEmpty())
    {
      pTransaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pTransaction);
    }
  }

  // TODO: Fire event
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
