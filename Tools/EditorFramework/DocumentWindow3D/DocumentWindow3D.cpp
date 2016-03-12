#include <PCH.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Timestamp.h>

#include <Foundation/Serialization/ReflectionSerializer.h>

ezQtEngineDocumentWindow::ezQtEngineDocumentWindow(ezDocument* pDocument) : ezQtDocumentWindow(pDocument)
{
  m_pEngineConnection = nullptr;

  m_pEngineConnection = ezEditorEngineProcessConnection::GetInstance()->CreateEngineConnection(this);

  m_Mirror.SetIPC(m_pEngineConnection);
  m_Mirror.InitSender(pDocument->GetObjectManager());
}

ezQtEngineDocumentWindow::~ezQtEngineDocumentWindow()
{
  m_Mirror.DeInit();
  // delete all view widgets, so that they can send their messages before we clean up the engine connection
  DestroyAllViews();

  ezEditorEngineProcessConnection::GetInstance()->DestroyEngineConnection(this);
}

void ezQtEngineDocumentWindow::SendMessageToEngine(ezEditorEngineDocumentMsg* pMessage) const
{
  m_pEngineConnection->SendMessage(pMessage);
}

const ezObjectPickingResult& ezQtEngineDocumentWindow::PickObject(ezUInt16 uiScreenPosX, ezUInt16 uiScreenPosY) const
{
  m_LastPickingResult.m_PickedComponent = ezUuid();
  m_LastPickingResult.m_PickedObject = ezUuid();
  m_LastPickingResult.m_PickedOther = ezUuid();
  m_LastPickingResult.m_uiPartIndex = 0;
  m_LastPickingResult.m_vPickedPosition.SetZero();
  m_LastPickingResult.m_vPickedNormal.SetZero();
  m_LastPickingResult.m_vPickingRayStart.SetZero();

  // do not send picking messages while the engine process isn't fully configured yet
  if (ezEditorEngineProcessConnection::GetInstance()->IsEngineSetup())
  {
    auto pView = GetHoveredViewWidget();

    if (pView != nullptr)
    {
      ezViewPickingMsgToEngine msg;
      msg.m_uiViewID = pView->GetViewID();
      msg.m_uiPickPosX = uiScreenPosX;
      msg.m_uiPickPosY = uiScreenPosY;

      SendMessageToEngine(&msg);

      if (ezEditorEngineProcessConnection::GetInstance()->WaitForMessage(ezGetStaticRTTI<ezViewPickingResultMsgToEditor>(), ezTime::Seconds(3.0)).Failed())
        return m_LastPickingResult;
    }
  }

  return m_LastPickingResult;
}

void ezQtEngineDocumentWindow::InternalRedraw()
{
  // TODO: Move this to a better place (some kind of regular update function, not redraw)
  SyncObjectsToEngine();
}

bool ezQtEngineDocumentWindow::HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezDocumentOpenResponseMsgToEditor>())
  {
    m_Mirror.SendDocument();
    return true;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewPickingResultMsgToEditor>())
  {
    const ezViewPickingResultMsgToEditor* pFullMsg = static_cast<const ezViewPickingResultMsgToEditor*>(pMsg);

    m_LastPickingResult.m_PickedObject = pFullMsg->m_ObjectGuid;
    m_LastPickingResult.m_PickedComponent = pFullMsg->m_ComponentGuid;
    m_LastPickingResult.m_PickedOther = pFullMsg->m_OtherGuid;
    m_LastPickingResult.m_uiPartIndex = pFullMsg->m_uiPartIndex;
    m_LastPickingResult.m_vPickedPosition = pFullMsg->m_vPickedPosition;
    m_LastPickingResult.m_vPickedNormal = pFullMsg->m_vPickedNormal;
    m_LastPickingResult.m_vPickingRayStart = pFullMsg->m_vPickingRayStartPosition;

    return true;
  }

  return false;
}

void ezQtEngineDocumentWindow::AddSyncObject(ezEditorEngineSyncObject* pSync)
{
  m_SyncObjects.PushBack(pSync);
  pSync->m_pOwner = this;
  m_AllSyncObjects[pSync->GetGuid()] = pSync;
}

void ezQtEngineDocumentWindow::RemoveSyncObject(ezEditorEngineSyncObject* pSync)
{
  m_DeletedObjects.PushBack(pSync->GetGuid());
  m_AllSyncObjects.Remove(pSync->GetGuid());
  m_SyncObjects.RemoveSwap(pSync);
  pSync->m_pOwner = nullptr;
}

ezEditorEngineSyncObject* ezQtEngineDocumentWindow::FindSyncObject(const ezUuid& guid)
{
  ezEditorEngineSyncObject* pSync = nullptr;
  m_AllSyncObjects.TryGetValue(guid, pSync);
  return pSync;
}

ezQtEngineViewWidget* ezQtEngineDocumentWindow::GetHoveredViewWidget() const
{
  QWidget* pWidget = QApplication::widgetAt(QCursor::pos());

  while (pWidget != nullptr)
  {
    ezQtEngineViewWidget* pCandidate = qobject_cast<ezQtEngineViewWidget*>(pWidget);
    if (pCandidate != nullptr)
    {
      if (m_ViewWidgets.Contains(pCandidate))
        return pCandidate;

      return nullptr;
    }

    pWidget = pWidget->parentWidget();
  }

  return nullptr;
}

ezQtEngineViewWidget* ezQtEngineDocumentWindow::GetFocusedViewWidget() const
{
  QWidget* pWidget = QApplication::focusWidget();

  while (pWidget != nullptr)
  {
    ezQtEngineViewWidget* pCandidate = qobject_cast<ezQtEngineViewWidget*>(pWidget);
    if (pCandidate != nullptr)
    {
      if (m_ViewWidgets.Contains(pCandidate))
          return pCandidate;

      return nullptr;
    }

    pWidget = pWidget->parentWidget();
  }

  return nullptr;
}

ezQtEngineViewWidget * ezQtEngineDocumentWindow::GetViewWidgetByID(ezUInt32 uiViewID) const
{
  for (auto pView : m_ViewWidgets)
  {
    if (pView && pView->GetViewID() == uiViewID)
      return pView;
  }

  return nullptr;
}

void ezQtEngineDocumentWindow::SyncObjectsToEngine()
{
  // Tell the engine which sync objects have been removed recently
  {
    for (const auto& guid : m_DeletedObjects)
    {
      ezEditorEngineSyncObjectMsg msg;
      msg.m_ObjectGuid = guid;
      SendMessageToEngine(&msg);
    }

    m_DeletedObjects.Clear();
  }

  for (auto* pObject : m_SyncObjects)
  {
    if (!pObject->GetModified())
      continue;

    ezEditorEngineSyncObjectMsg msg;
    msg.m_ObjectGuid = pObject->m_SyncObjectGuid;
    msg.m_sObjectType = pObject->GetDynamicRTTI()->GetTypeName();

    ezMemoryStreamStorage storage;
    ezMemoryStreamWriter writer(&storage);
    ezMemoryStreamReader reader(&storage);

    ezReflectionSerializer::WriteObjectToBinary(writer, pObject->GetDynamicRTTI(), pObject);
    msg.m_ObjectData = ezArrayPtr<const ezUInt8>(storage.GetData(), storage.GetStorageSize());

    SendMessageToEngine(&msg);

    pObject->SetModified(false);
  }
}

void ezQtEngineDocumentWindow::DestroyAllViews()
{
  while (!m_ViewWidgets.IsEmpty())
  {
    delete m_ViewWidgets[0];
  }
}


