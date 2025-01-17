#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

#include <EditorFramework/Assets/AssetDocument.h>

ezQtEngineDocumentWindow::ezQtEngineDocumentWindow(ezAssetDocument* pDocument)
  : ezQtDocumentWindow(pDocument)
{
  pDocument->m_ProcessMessageEvent.AddEventHandler(ezMakeDelegate(&ezQtEngineDocumentWindow::ProcessMessageEventHandler, this));
  pDocument->m_CommonAssetUiChangeEvent.AddEventHandler(ezMakeDelegate(&ezQtEngineDocumentWindow::CommonAssetUiEventHandler, this));
}

ezQtEngineDocumentWindow::~ezQtEngineDocumentWindow()
{
  // make sure the selection gets cleared before the views are destroyed, so that dependent code can clean up first
  GetDocument()->GetSelectionManager()->Clear();

  GetDocument()->m_ProcessMessageEvent.RemoveEventHandler(ezMakeDelegate(&ezQtEngineDocumentWindow::ProcessMessageEventHandler, this));
  GetDocument()->m_CommonAssetUiChangeEvent.RemoveEventHandler(ezMakeDelegate(&ezQtEngineDocumentWindow::CommonAssetUiEventHandler, this));

  // delete all view widgets, so that they can send their messages before we clean up the engine connection
  DestroyAllViews();
}


ezEditorEngineConnection* ezQtEngineDocumentWindow::GetEditorEngineConnection() const
{
  return GetDocument()->GetEditorEngineConnection();
}

static ezObjectPickingResult s_DummyResult;

const ezObjectPickingResult& ezQtEngineDocumentWindow::PickObject(ezUInt16 uiScreenPosX, ezUInt16 uiScreenPosY, ezQtEngineViewWidget* pView) const
{
  if (pView == nullptr)
    pView = GetHoveredViewWidget();

  if (pView != nullptr)
    return pView->PickObject(uiScreenPosX, uiScreenPosY);

  return s_DummyResult;
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

ezArrayPtr<ezQtEngineViewWidget* const> ezQtEngineDocumentWindow::GetViewWidgets() const
{
  return m_ViewWidgets;
}

void ezQtEngineDocumentWindow::AddViewWidget(ezQtEngineViewWidget* pView)
{
  m_ViewWidgets.PushBack(pView);
  ezEngineWindowEvent e;
  e.m_Type = ezEngineWindowEvent::Type::ViewCreated;
  e.m_pView = pView;
  m_EngineWindowEvent.Broadcast(e);
}

void ezQtEngineDocumentWindow::RemoveViewWidget(ezQtEngineViewWidget* pView)
{
  m_ViewWidgets.RemoveAndSwap(pView);
  ezEngineWindowEvent e;
  e.m_Type = ezEngineWindowEvent::Type::ViewDestroyed;
  e.m_pView = pView;
  m_EngineWindowEvent.Broadcast(e);
}

void ezQtEngineDocumentWindow::CommonAssetUiEventHandler(const ezCommonAssetUiState& e)
{
  ezSimpleDocumentConfigMsgToEngine msg;
  msg.m_sWhatToDo = "CommonAssetUiState";
  msg.m_PayloadValue = e.m_fValue;

  switch (e.m_State)
  {
    case ezCommonAssetUiState::Restart:
      msg.m_sPayload = "Restart";
      break;

    case ezCommonAssetUiState::Loop:
      msg.m_sPayload = "Loop";
      break;

    case ezCommonAssetUiState::Pause:
      msg.m_sPayload = "Pause";
      break;

    case ezCommonAssetUiState::Grid:
      msg.m_sPayload = "Grid";
      break;

    case ezCommonAssetUiState::SimulationSpeed:
      msg.m_sPayload = "SimulationSpeed";
      break;

    case ezCommonAssetUiState::Visualizers:
      msg.m_sPayload = "Visualizers";
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (!msg.m_sPayload.IsEmpty())
  {
    GetEditorEngineConnection()->SendMessage(&msg);
  }
}

void ezQtEngineDocumentWindow::ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineViewMsg>())
  {
    const ezEditorEngineViewMsg* pViewMsg = static_cast<const ezEditorEngineViewMsg*>(pMsg);

    ezQtEngineViewWidget* pView = GetViewWidgetByID(pViewMsg->m_uiViewID);

    if (pView != nullptr)
      pView->HandleViewMessage(pViewMsg);
  }
}

void ezQtEngineDocumentWindow::DestroyAllViews()
{
  while (!m_ViewWidgets.IsEmpty())
  {
    delete m_ViewWidgets[0];
  }
}
