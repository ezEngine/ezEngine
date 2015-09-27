#include <PCH.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Timestamp.h>
#include <QPushButton>
#include <qlayout.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

ezDocumentWindow3D::ezDocumentWindow3D(ezDocumentBase* pDocument) : ezDocumentWindow(pDocument)
{
  m_pRestartButtonLayout = nullptr;
  m_pRestartButton = nullptr;
  m_pEngineConnection = nullptr;

  m_pEngineConnection = ezEditorEngineProcessConnection::GetInstance()->CreateEngineConnection(this);

  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezMakeDelegate(&ezDocumentWindow3D::EngineViewProcessEventHandler, this));
}

ezDocumentWindow3D::~ezDocumentWindow3D()
{
  // delete all view widgets, so that they can send their messages before we clean up the engine connection
  DestroyAllViews();

  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezMakeDelegate(&ezDocumentWindow3D::EngineViewProcessEventHandler, this));

  ezEditorEngineProcessConnection::GetInstance()->DestroyEngineConnection(this);
}

void ezDocumentWindow3D::SendMessageToEngine(ezEditorEngineDocumentMsg* pMessage, bool bSuperHighPriority) const
{
  m_pEngineConnection->SendMessage(pMessage, bSuperHighPriority);
}

const ezObjectPickingResult& ezDocumentWindow3D::PickObject(ezUInt16 uiScreenPosX, ezUInt16 uiScreenPosY) const
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

      SendMessageToEngine(&msg, true);

      if (ezEditorEngineProcessConnection::GetInstance()->WaitForMessage(ezGetStaticRTTI<ezViewPickingResultMsgToEditor>(), ezTime::Seconds(3.0)).Failed())
        return m_LastPickingResult;
    }
  }

  return m_LastPickingResult;
}

void ezDocumentWindow3D::SlotRestartEngineProcess()
{
  ezEditorEngineProcessConnection::GetInstance()->RestartProcess();
}

void ezDocumentWindow3D::InternalRedraw()
{
  // TODO: Move this to a better place (some kind of regular update function, not redraw)
  SyncObjectsToEngine();
}

void ezDocumentWindow3D::ShowRestartButton(bool bShow)
{
  if (m_pRestartButtonLayout == nullptr && bShow == true)
  {
    m_pRestartButtonLayout = new QHBoxLayout(this);
    m_pRestartButtonLayout->setMargin(0);

    centralWidget()->setLayout(m_pRestartButtonLayout);

    m_pRestartButton = new QPushButton(centralWidget());
    m_pRestartButton->setText("Restart Engine View Process");
    m_pRestartButton->setVisible(ezEditorEngineProcessConnection::GetInstance()->IsProcessCrashed());
    m_pRestartButton->setMaximumWidth(200);
    m_pRestartButton->setMinimumHeight(50);
    m_pRestartButton->connect(m_pRestartButton, &QPushButton::clicked, this, &ezDocumentWindow3D::SlotRestartEngineProcess);

    m_pRestartButtonLayout->addWidget(m_pRestartButton);
  }

  if (m_pRestartButton)
  {
    m_pRestartButton->setVisible(bShow);
    //centralWidget()->setAutoFillBackground(bShow); /// \todo this seems not to work

    if (bShow)
      m_pRestartButton->update();
  }
}

bool ezDocumentWindow3D::HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezDocumentOpenResponseMsgToEditor>())
  {
    m_pEngineConnection->SendDocument();
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

void ezDocumentWindow3D::EngineViewProcessEventHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
  case ezEditorEngineProcessConnection::Event::Type::ProcessCrashed:
    {
      ShowRestartButton(true);
    }
    break;

  case ezEditorEngineProcessConnection::Event::Type::ProcessStarted:
    {
      ShowRestartButton(false);
    }
    break;

  case ezEditorEngineProcessConnection::Event::Type::ProcessShutdown:
    break;
  }
}

void ezDocumentWindow3D::AddSyncObject(ezEditorEngineSyncObject* pSync)
{
  m_SyncObjects.PushBack(pSync);
  pSync->m_pOwner = this;
  m_AllSyncObjects[pSync->GetGuid()] = pSync;
}

void ezDocumentWindow3D::RemoveSyncObject(ezEditorEngineSyncObject* pSync)
{
  m_DeletedObjects.PushBack(pSync->GetGuid());
  m_AllSyncObjects.Remove(pSync->GetGuid());
  m_SyncObjects.RemoveSwap(pSync);
  pSync->m_pOwner = nullptr;
}

ezEditorEngineSyncObject* ezDocumentWindow3D::FindSyncObject(const ezUuid& guid)
{
  ezEditorEngineSyncObject* pSync = nullptr;
  m_AllSyncObjects.TryGetValue(guid, pSync);
  return pSync;
}

ezEngineViewWidget* ezDocumentWindow3D::GetHoveredViewWidget() const
{
  QWidget* pWidget = QApplication::widgetAt(QCursor::pos());

  while (pWidget != nullptr)
  {
    ezEngineViewWidget* pCandidate = qobject_cast<ezEngineViewWidget*>(pWidget);
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

ezEngineViewWidget* ezDocumentWindow3D::GetFocusedViewWidget() const
{
  QWidget* pWidget = QApplication::focusWidget();

  while (pWidget != nullptr)
  {
    ezEngineViewWidget* pCandidate = qobject_cast<ezEngineViewWidget*>(pWidget);
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

void ezDocumentWindow3D::SyncObjectsToEngine()
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

  ezStringBuilder sData;

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

    ezReflectionSerializer::WriteObjectToJSON(writer, pObject->GetDynamicRTTI(), pObject);

    sData.ReadAll(reader);
    msg.SetObjectData(sData);

    SendMessageToEngine(&msg);

    pObject->SetModified(false);
  }
}

void ezDocumentWindow3D::DestroyAllViews()
{
  while (!m_ViewWidgets.IsEmpty())
  {
    delete m_ViewWidgets[0];
  }
}


