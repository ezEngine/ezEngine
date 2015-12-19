#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Command/Command.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/IO/MemoryStream.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCommand, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezCommand::ezCommand() : m_sDescription(), m_bUndoable(true), m_pDocument(nullptr)
{
}

ezStatus ezCommand::Do(bool bRedo)
{
  if (DoInternal(bRedo).m_Result == EZ_FAILURE)
  {
    if (bRedo)
    {
      // A command that originally succeeded failed on redo!
      return ezStatus(EZ_FAILURE);
    }
    else
    {
      for (ezInt32 j = m_ChildActions.GetCount() - 1; j >= 0; --j)
      {
        ezStatus status = m_ChildActions[j]->Undo(true);
        EZ_ASSERT_DEV(status.m_Result == EZ_SUCCESS, "Failed do could not be recovered! Inconsistent state!");
      }
      return ezStatus(EZ_FAILURE);
    }
  }
  if (!bRedo)
    return ezStatus(EZ_SUCCESS);

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

ezStatus ezCommand::Undo(bool bFireEvents)
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
      // A command that originally succeeded failed on undo!
      return ezStatus(EZ_FAILURE);
    }
  }

  if (UndoInternal(bFireEvents).m_Result == EZ_FAILURE)
  {
    for (ezUInt32 j = 0; j < uiChildActions; ++j)
    {
      ezStatus status = m_ChildActions[j]->Do(true);
      EZ_ASSERT_DEV(status.m_Result == EZ_SUCCESS, "Failed undo could not be recovered! Inconsistent state!");
    }
    // A command that originally succeeded failed on undo!
    return ezStatus(EZ_FAILURE);
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


ezStatus ezCommand::AddCommand(ezCommand& command)
{
  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezMemoryStreamReader reader(&storage);

  /// \todo Clone action, instead of writing to JSON and then reading from it again

  ezReflectionSerializer::WriteObjectToJSON(writer, command.GetDynamicRTTI(), &command, ezJSONWriter::WhitespaceMode::None);

  const ezRTTI* pRtti;
  ezCommand* pCommand = (ezCommand*)ezReflectionSerializer::ReadObjectFromJSON(reader, pRtti);

  pCommand->m_pDocument = m_pDocument;
  ezStatus ret = pCommand->Do(false);

  if (ret.m_Result == EZ_FAILURE)
  {
    pCommand->Cleanup(ezCommand::CommandState::WasDone);
    pCommand->GetDynamicRTTI()->GetAllocator()->Deallocate(pCommand);
    return ret;
  }

  m_ChildActions.PushBack(pCommand);

  return ezStatus(EZ_SUCCESS);
}