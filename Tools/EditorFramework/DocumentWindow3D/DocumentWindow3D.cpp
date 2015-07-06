#include <PCH.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Timestamp.h>
#include <QPushButton>
#include <qlayout.h>

ezDocumentWindow3D::ezDocumentWindow3D(ezDocumentBase* pDocument) : ezDocumentWindow(pDocument)
{
  m_pRestartButtonLayout = nullptr;
  m_pRestartButton = nullptr;
  m_pEngineView = nullptr;

  m_pEngineView = ezEditorEngineProcessConnection::GetInstance()->CreateEngineConnection(this);

  m_DelegateEngineViewProcessEvents = ezMakeDelegate(&ezDocumentWindow3D::EngineViewProcessEventHandler, this);
  ezEditorEngineProcessConnection::s_Events.AddEventHandler(m_DelegateEngineViewProcessEvents);
}

ezDocumentWindow3D::~ezDocumentWindow3D()
{
  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(m_DelegateEngineViewProcessEvents);

  ezEditorEngineProcessConnection::GetInstance()->DestroyEngineConnection(this);
}

void ezDocumentWindow3D::SendMessageToEngine(ezEditorEngineDocumentMsg* pMessage, bool bSuperHighPriority) const
{
  m_pEngineView->SendMessage(pMessage, bSuperHighPriority);
}

const ezObjectPickingResult& ezDocumentWindow3D::PickObject(ezUInt16 uiScreenPosX, ezUInt16 uiScreenPosY) const
{
  ezTimestamp ts = ezTimestamp::CurrentTimestamp();
  ezStopwatch s;

  m_LastPickingResult.m_PickedComponent = ezUuid();
  m_LastPickingResult.m_PickedObject = ezUuid();
  m_LastPickingResult.m_PickedOther = ezUuid();
  m_LastPickingResult.m_uiPartIndex = 0;
  m_LastPickingResult.m_vPickedPosition.SetZero();
  m_LastPickingResult.m_vPickingRayStart.SetZero();

  // do not send picking messages while the engine process isn't fully configured yet
  if (ezEditorEngineProcessConnection::GetInstance()->IsEngineSetup())
  {
    ezViewPickingMsgToEngine msg;
    msg.m_uiPickPosX = uiScreenPosX;
    msg.m_uiPickPosY = uiScreenPosY;

    SendMessageToEngine(&msg, true);

    if (ezEditorEngineProcessConnection::GetInstance()->WaitForMessage(ezGetStaticRTTI<ezViewPickingResultMsgToEditor>(), ezTime::Seconds(3.0)).Failed())
      return m_LastPickingResult;
  }

  const ezTime tPick = s.Checkpoint();
  //ezLog::Dev("%lli: Picking: %.3fms", ts.GetInt64(ezSIUnitOfTime::Microsecond), tPick.GetMilliseconds());

  return m_LastPickingResult;
}

void ezDocumentWindow3D::SlotRestartEngineProcess()
{
  ezEditorEngineProcessConnection::GetInstance()->RestartProcess();
}

void ezDocumentWindow3D::SyncObjects()
{
  ezEditorEngineSyncObject::SyncObjectsToEngine(*m_pEngineView, false);
}

void ezDocumentWindow3D::InternalRedraw()
{
  // TODO: Move this to a better place (some kind of regular update function, not redraw)
  SyncObjects();
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
    m_pEngineView->SendDocument();
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

