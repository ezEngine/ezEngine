#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcess/GameState.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Utilities/Stats.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Logging/Log.h>

#include <RendererCore/RenderContext/RenderContext.h>

void ezEngineProcessGameState::SendProjectReadyMessage()
{
  ezProjectReadyMsgToEditor msg;
  m_IPC.SendMessage(&msg);
}


void ezEngineProcessGameState::SendReflectionInformation()
{
  ezSet<const ezRTTI*> types;
  ezReflectionUtils::GatherTypesDerivedFromClass(ezGetStaticRTTI<ezComponent>(), types, true);
  ezDynamicArray<const ezRTTI*> sortedTypes;
  ezReflectionUtils::CreateDependencySortedTypeArray(types, sortedTypes);

  for (auto type : sortedTypes)
  {
    ezUpdateReflectionTypeMsgToEditor TypeMsg;
    ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(type, TypeMsg.m_desc);
    m_IPC.SendMessage(&TypeMsg);
  }
}

void ezEngineProcessGameState::EventHandlerIPC(const ezProcessCommunication::Event& e)
{
  // Project Messages:
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezSetupProjectMsgToEngine>())
  {
    const ezSetupProjectMsgToEngine* pSetupMsg = static_cast<const ezSetupProjectMsgToEngine*>(e.m_pMessage);
    ezApplicationConfig::SetProjectDirectory(pSetupMsg->m_sProjectDir);

    const_cast<ezSetupProjectMsgToEngine*>(pSetupMsg)->m_Config.Apply();
    // Project setup, we are now ready to accept document messages.
    SendProjectReadyMessage();
    return;
  }
  else if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezSimpleConfigMsgToEngine>())
  {
    const ezSimpleConfigMsgToEngine* pMsg = static_cast<const ezSimpleConfigMsgToEngine*>(e.m_pMessage);

    if (pMsg->m_sWhatToDo == "ReloadAssetLUT")
    {
      ezFileSystem::ReloadAllExternalDataDirectoryConfigs();
    }
    else if (pMsg->m_sWhatToDo == "ReloadResources")
    {
      ezResourceManager::ReloadAllResources();
    }
    else
      ezLog::Warning("Unknown ezSimpleConfigMsgToEngine '%s'", pMsg->m_sWhatToDo.GetData());
  }

  //static ezUInt32 uiMessagesPerFrame = 0;
  //static ezUInt32 uiBlockingMessagesPerFrame = 0;
  //static ezUInt32 uiSyncObjMessagesPerFrame = 0;

  //++uiMessagesPerFrame;

  // Document Messages:
  if (!e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineDocumentMsg>())
    return;

  const ezEditorEngineDocumentMsg* pDocMsg = (const ezEditorEngineDocumentMsg*) e.m_pMessage;

  ezEngineProcessDocumentContext* pDocumentContext = ezEngineProcessDocumentContext::GetDocumentContext(pDocMsg->m_DocumentGuid);

  if (pDocMsg->GetDynamicRTTI()->IsDerivedFrom<ezDocumentOpenMsgToEngine>()) // Document was opened or closed
  {
    const ezDocumentOpenMsgToEngine* pMsg = static_cast<const ezDocumentOpenMsgToEngine*>(pDocMsg);

    if (pMsg->m_bDocumentOpen && pMsg->m_DocumentGuid.IsValid())
    {
      ezDocumentOpenResponseMsgToEditor m;
      m.m_DocumentGuid = pMsg->m_DocumentGuid;

      ezRTTI* pRtti = ezRTTI::GetFirstInstance();
      while (pRtti)
      {
        if (pRtti->IsDerivedFrom<ezEngineProcessDocumentContext>())
        {
          auto* pProp = pRtti->FindPropertyByName("DocumentType");
          if (pProp && pProp->GetCategory() == ezPropertyCategory::Constant)
          {
            if (static_cast<ezAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<ezString>() == pMsg->m_sDocumentType)
            {
              ezLog::Info("Created Context of type '%s' for '%s'", pRtti->GetTypeName(), pMsg->m_sDocumentType.GetData());

              pDocumentContext = static_cast<ezEngineProcessDocumentContext*>(pRtti->GetAllocator()->Allocate());

              ezEngineProcessDocumentContext::AddDocumentContext(pDocMsg->m_DocumentGuid, pDocumentContext, &m_IPC);
              break;
            }
          }
        }

        pRtti = pRtti->GetNextInstance();
      }

      m_IPC.SendMessage(&m);
    }
    else
    {
      ezEngineProcessDocumentContext::DestroyDocumentContext(pDocMsg->m_DocumentGuid);
    }

    return;
  }

  EZ_ASSERT_DEV(pDocumentContext != nullptr, "Document Context is invalid!");

  pDocumentContext->HandleMessage(pDocMsg);


}

