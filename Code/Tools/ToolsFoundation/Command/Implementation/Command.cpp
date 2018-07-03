#include <PCH.h>
#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/IO/MemoryStream.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCommand, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezCommand::ezCommand() : m_sDescription(), m_bUndoable(true), m_pDocument(nullptr)
{
}

ezStatus ezCommand::Do(bool bRedo)
{
  ezStatus status = DoInternal(bRedo);
  if (status.m_Result == EZ_FAILURE)
  {
    if (bRedo)
    {
      // A command that originally succeeded failed on redo!
      return status;
    }
    else
    {
      for (ezInt32 j = m_ChildActions.GetCount() - 1; j >= 0; --j)
      {
        ezStatus status = m_ChildActions[j]->Undo(true);
        EZ_ASSERT_DEV(status.m_Result == EZ_SUCCESS, "Failed do could not be recovered! Inconsistent state!");
      }
      return status;
    }
  }
  if (!bRedo)
    return ezStatus(EZ_SUCCESS);

  const ezUInt32 uiChildActions = m_ChildActions.GetCount();
  for (ezUInt32 i = 0; i < uiChildActions; ++i)
  {
    status = m_ChildActions[i]->Do(bRedo);
    if (status.m_Result == EZ_FAILURE)
    {
      for (ezInt32 j = i - 1; j >= 0; --j)
      {
        ezStatus status2 = m_ChildActions[j]->Undo(true);
        EZ_ASSERT_DEV(status2.m_Result == EZ_SUCCESS, "Failed redo could not be recovered! Inconsistent state!");
      }
      // A command that originally succeeded failed on redo!
      return status;
    }
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCommand::Undo(bool bFireEvents)
{
  const ezUInt32 uiChildActions = m_ChildActions.GetCount();
  for (ezInt32 i = uiChildActions - 1; i >= 0; --i)
  {
    ezStatus status = m_ChildActions[i]->Undo(bFireEvents);
    if (status.m_Result == EZ_FAILURE)
    {
      for (ezUInt32 j = i + 1; j < uiChildActions; ++j)
      {
        ezStatus status2 = m_ChildActions[j]->Do(true);
        EZ_ASSERT_DEV(status2.m_Result == EZ_SUCCESS, "Failed undo could not be recovered! Inconsistent state!");
      }
      // A command that originally succeeded failed on undo!
      return status;
    }
  }

  ezStatus status = UndoInternal(bFireEvents);
  if (status.m_Result == EZ_FAILURE)
  {
    for (ezUInt32 j = 0; j < uiChildActions; ++j)
    {
      ezStatus status2 = m_ChildActions[j]->Do(true);
      EZ_ASSERT_DEV(status2.m_Result == EZ_SUCCESS, "Failed undo could not be recovered! Inconsistent state!");
    }
    // A command that originally succeeded failed on undo!
    return status;
  }

  return ezStatus(EZ_SUCCESS);
}

void ezCommand::Cleanup(CommandState state)
{
  CleanupInternal(state);

  for (ezCommand* pCommand : m_ChildActions)
  {
    pCommand->Cleanup(state);
    pCommand->GetDynamicRTTI()->GetAllocator()->Deallocate(pCommand);
  }

  m_ChildActions.Clear();
}


ezStatus ezCommand::AddSubCommand(ezCommand& command)
{
  ezCommand* pCommand = ezReflectionSerializer::Clone(&command);
  const ezRTTI* pRtti = pCommand->GetDynamicRTTI();

  pCommand->m_pDocument = m_pDocument;

  m_ChildActions.PushBack(pCommand);
  m_pDocument->GetCommandHistory()->m_ActiveCommandStack.PushBack(pCommand);
  ezStatus ret = pCommand->Do(false);
  m_pDocument->GetCommandHistory()->m_ActiveCommandStack.PopBack();

  if (ret.m_Result == EZ_FAILURE)
  {
    m_ChildActions.PopBack();
    pCommand->GetDynamicRTTI()->GetAllocator()->Deallocate(pCommand);
    return ret;
  }

  if (pCommand->HasReturnValues())
  {
    // Write properties back so any return values get written.
    ezMemoryStreamStorage storage;
    ezMemoryStreamWriter writer(&storage);
    ezMemoryStreamReader reader(&storage);

    ezReflectionSerializer::WriteObjectToBinary(writer, pCommand->GetDynamicRTTI(), pCommand);
    ezReflectionSerializer::ReadObjectPropertiesFromBinary(reader, *pRtti, &command);
  }

  return ezStatus(EZ_SUCCESS);
}
