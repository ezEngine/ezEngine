#include <PCH.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Timestamp.h>

#include <Foundation/Serialization/ReflectionSerializer.h>
#include <EditorFramework/Assets/AssetDocument.h>

ezQtEngineDocumentWindow::ezQtEngineDocumentWindow(ezAssetDocument* pDocument) : ezQtDocumentWindow(pDocument)
{
  pDocument->m_ProcessMessageEvent.AddEventHandler(ezMakeDelegate(&ezQtEngineDocumentWindow::ProcessMessageEventHandler, this));
}

ezQtEngineDocumentWindow::~ezQtEngineDocumentWindow()
{
  GetDocument()->m_ProcessMessageEvent.RemoveEventHandler(ezMakeDelegate(&ezQtEngineDocumentWindow::ProcessMessageEventHandler, this));

  // delete all view widgets, so that they can send their messages before we clean up the engine connection
  DestroyAllViews();
}


ezEditorEngineConnection* ezQtEngineDocumentWindow::GetEditorEngineConnection() const
{
  return GetDocument()->GetEditorEngineConnection();
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
  if (ezEditorEngineProcessConnection::GetSingleton()->IsEngineSetup())
  {
    auto pView = GetHoveredViewWidget();

    if (pView != nullptr)
    {
      ezViewPickingMsgToEngine msg;
      msg.m_uiViewID = pView->GetViewID();
      msg.m_uiPickPosX = uiScreenPosX;
      msg.m_uiPickPosY = uiScreenPosY;

      GetDocument()->SendMessageToEngine(&msg);

      if (ezEditorEngineProcessConnection::GetSingleton()->WaitForMessage(ezGetStaticRTTI<ezViewPickingResultMsgToEditor>(), ezTime::Seconds(3.0)).Failed())
        return m_LastPickingResult;
    }
  }

  return m_LastPickingResult;
}


ezAssetDocument* ezQtEngineDocumentWindow::GetDocument() const
{
  return static_cast<ezAssetDocument*>(ezQtDocumentWindow::GetDocument());
}

void ezQtEngineDocumentWindow::InternalRedraw()
{
  // TODO: Move this to a better place (some kind of regular update function, not redraw)
  GetDocument()->SyncObjectsToEngine();
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

ezQtEngineViewWidget* ezQtEngineDocumentWindow::GetViewWidgetByID(ezUInt32 uiViewID) const
{
  for (auto pView : m_ViewWidgets)
  {
    if (pView && pView->GetViewID() == uiViewID)
      return pView;
  }

  return nullptr;
}


void ezQtEngineDocumentWindow::ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg)
{
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
  }
}

void ezQtEngineDocumentWindow::DestroyAllViews()
{
  while (!m_ViewWidgets.IsEmpty())
  {
    delete m_ViewWidgets[0];
  }
}


