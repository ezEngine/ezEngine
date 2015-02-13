#include <PCH.h>
#include <EditorFramework/EditorApp.moc.h>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcess/Application.h>


void ezEditorProcessApp::EventHandlerIPC(const ezProcessCommunication::Event& e)
{
  const ezEditorEngineDocumentMsg* pMsg = (const ezEditorEngineDocumentMsg*) e.m_pMessage;

  ezViewContext* pViewContext = (ezViewContext*) ezEngineProcessViewContext::GetViewContext(pMsg->m_uiViewID);

  if (pViewContext == nullptr && pMsg->m_uiViewID != 0xFFFFFFFF)
  {
    ezLog::Info("Created new View 0x%08X for document %s", pMsg->m_uiViewID, ezConversionUtils::ToString(pMsg->m_DocumentGuid).GetData());

    pViewContext = EZ_DEFAULT_NEW(ezViewContext)(pMsg->m_uiViewID, pMsg->m_DocumentGuid);

    ezEngineProcessViewContext::AddViewContext(pMsg->m_uiViewID, pViewContext);
  }

  ezEngineProcessDocumentContext* pDocumentContext = ezEngineProcessDocumentContext::GetDocumentContext(pMsg->m_DocumentGuid);

  if (pDocumentContext == nullptr && pMsg->m_DocumentGuid.IsValid())
  {
    ezLog::Info("Created new Document context for Guid %s", ezConversionUtils::ToString(pMsg->m_DocumentGuid).GetData());

    pDocumentContext = EZ_DEFAULT_NEW(ezEngineProcessDocumentContext);

    ezEngineProcessDocumentContext::AddDocumentContext(pMsg->m_DocumentGuid, pDocumentContext);

    pDocumentContext->m_pWorld = EZ_DEFAULT_NEW(ezWorld)(ezConversionUtils::ToString(pMsg->m_DocumentGuid));
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewRedrawMsgToEngine>())
  {
    ezViewRedrawMsgToEngine* pRedrawMsg = (ezViewRedrawMsgToEngine*) pMsg;

    pViewContext->SetupRenderTarget((HWND) pRedrawMsg->m_uiHWND, pRedrawMsg->m_uiWindowWidth, pRedrawMsg->m_uiWindowHeight);
    pViewContext->Redraw();
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEntityMsgToEngine>())
  {
    ezEntityMsgToEngine* pEntityMsg = (ezEntityMsgToEngine*) pMsg;

    static ezHashTable<ezUuid, ezGameObjectHandle> g_AllObjects;

    const char* szDone = "unknown";
    switch (pEntityMsg->m_iMsgType)
    {
    case ezEntityMsgToEngine::ObjectAdded:
      {
        szDone = "Added";

        ezGameObjectDesc d;
        d.m_sName.Assign(ezConversionUtils::ToString(pEntityMsg->m_ObjectGuid).GetData());

        if (pEntityMsg->m_NewParentGuid.IsValid())
          d.m_Parent = g_AllObjects[pEntityMsg->m_NewParentGuid];

        ezGameObjectHandle hObject = pDocumentContext->m_pWorld->CreateObject(d);
        g_AllObjects[pEntityMsg->m_ObjectGuid] = hObject;

        ezGameObject* pObject;
        if (pDocumentContext->m_pWorld->TryGetObject(hObject, pObject))
        {
          ezMemoryStreamStorage storage;
          ezMemoryStreamWriter writer(&storage);
          ezMemoryStreamReader reader(&storage);

          writer.WriteBytes(pEntityMsg->m_sObjectData.GetData(), pEntityMsg->m_sObjectData.GetElementCount());

          ezReflectionUtils::ReadObjectPropertiesFromJSON(reader, *ezGetStaticRTTI<ezGameObject>(), pObject);
        }
      }
      break;

    case ezEntityMsgToEngine::ObjectMoved:
      {
        szDone = "Moved";

        ezGameObjectHandle hObject = g_AllObjects[pEntityMsg->m_ObjectGuid];

        ezGameObjectHandle hNewParent;
        if (pEntityMsg->m_NewParentGuid.IsValid())
          hNewParent = g_AllObjects[pEntityMsg->m_NewParentGuid];

        ezGameObject* pObject = nullptr;
        if (pDocumentContext->m_pWorld->TryGetObject(hObject, pObject))
        {
          pObject->SetParent(hNewParent);
        }
        else
          ezLog::Error("Couldn't access game object object %s in world %p", ezConversionUtils::ToString(pEntityMsg->m_ObjectGuid).GetData(), pDocumentContext->m_pWorld);
      }
      break;

    case ezEntityMsgToEngine::ObjectRemoved:
      {
        szDone = "Removed";

        pDocumentContext->m_pWorld->DeleteObject(g_AllObjects[pEntityMsg->m_ObjectGuid]);
      }
      break;

    case ezEntityMsgToEngine::PropertyChanged:
      {
        szDone = "Property";

        ezGameObjectHandle hObject = g_AllObjects[pEntityMsg->m_ObjectGuid];

        ezGameObject* pObject;
        if (pDocumentContext->m_pWorld->TryGetObject(hObject, pObject))
        {
          ezMemoryStreamStorage storage;
          ezMemoryStreamWriter writer(&storage);
          ezMemoryStreamReader reader(&storage);

          writer.WriteBytes(pEntityMsg->m_sObjectData.GetData(), pEntityMsg->m_sObjectData.GetElementCount());

          ezReflectionUtils::ReadObjectPropertiesFromJSON(reader, *ezGetStaticRTTI<ezGameObject>(), pObject);
        }
      }
      break;
    }

    ezStringBuilder s;
    s.Format("%s: Entity %s, OldParent %s, NewParent %s, Child %u ", 
                szDone,
                ezConversionUtils::ToString(pEntityMsg->m_ObjectGuid).GetData(),
                ezConversionUtils::ToString(pEntityMsg->m_PreviousParentGuid).GetData(),
                ezConversionUtils::ToString(pEntityMsg->m_NewParentGuid).GetData(),
                pEntityMsg->m_uiNewChildIndex);

    ezLogMsgToEditor lm;
    lm.m_sText = s;
    lm.m_uiViewID = pEntityMsg->m_uiViewID;
    lm.m_DocumentGuid = pEntityMsg->m_DocumentGuid;

    m_IPC.SendMessage(&lm);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewCameraMsgToEngine>())
  {
    ezViewCameraMsgToEngine* pCamMsg = (ezViewCameraMsgToEngine*) pMsg;

    pViewContext->SetCamera(pCamMsg);
  }
}


