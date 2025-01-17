#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/EventTrackEditorWidget.moc.h>
#include <GuiFoundation/Widgets/TimeScrubberWidget.moc.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

ezQtAnimationClipAssetDocumentWindow::ezQtAnimationClipAssetDocumentWindow(ezAnimationClipAssetDocument* pDocument)
  : ezQtEngineDocumentWindow(pDocument)
  , m_Clock("AssetClip")
{
  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "AnimationClipAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "AnimationClipAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("AnimationClipAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  ezQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(ezVec3(-1.6f, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new ezQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureRelative(ezVec3(0, 0, 1), ezVec3(5.0f), ezVec3(5, -2, 3), 2.0f);
    AddViewWidget(m_pViewWidget);
    pContainer = new ezQtViewWidgetContainer(this, m_pViewWidget, "AnimationClipAssetViewToolBar");
    m_pDockManager->setCentralWidget(pContainer);
  }

  // Property Grid
  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("AnimationClipAssetDockWidget");
    pPropertyPanel->setWindowTitle("Animation Clip Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new ezQtAssetStatusIndicator(GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pPropertyPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  // Time Scrubber
  {
    m_pTimeScrubber = new ezQtTimeScrubberWidget(pContainer);
    m_pTimeScrubber->SetDuration(ezTime::MakeFromSeconds(1));

    pContainer->GetLayout()->addWidget(m_pTimeScrubber);

    connect(m_pTimeScrubber, &ezQtTimeScrubberWidget::ScrubberPosChangedEvent, this, &ezQtAnimationClipAssetDocumentWindow::OnScrubberPosChangedEvent);
  }

  // Event Track Panel
  {
    m_pEventTrackPanel = new ezQtDocumentPanel(this, pDocument);
    m_pEventTrackPanel->setObjectName("AnimClipEventTrackDockWidget");
    m_pEventTrackPanel->setWindowTitle("Event Track");
    m_pEventTrackPanel->show();

    m_pEventTrackEditor = new ezQtEventTrackEditorWidget(m_pEventTrackPanel);
    m_pEventTrackPanel->setWidget(m_pEventTrackEditor);

    m_pDockManager->addDockWidgetTab(ads::BottomDockWidgetArea, m_pEventTrackPanel);

    UpdateEventTrackEditor();
  }

  // Event track editor events
  {
    connect(m_pEventTrackEditor, &ezQtEventTrackEditorWidget::InsertCpEvent, this, &ezQtAnimationClipAssetDocumentWindow::onEventTrackInsertCpAt);
    connect(m_pEventTrackEditor, &ezQtEventTrackEditorWidget::CpMovedEvent, this, &ezQtAnimationClipAssetDocumentWindow::onEventTrackCpMoved);
    connect(m_pEventTrackEditor, &ezQtEventTrackEditorWidget::CpDeletedEvent, this, &ezQtAnimationClipAssetDocumentWindow::onEventTrackCpDeleted);

    connect(m_pEventTrackEditor, &ezQtEventTrackEditorWidget::BeginOperationEvent, this, &ezQtAnimationClipAssetDocumentWindow::onEventTrackBeginOperation);
    connect(m_pEventTrackEditor, &ezQtEventTrackEditorWidget::EndOperationEvent, this, &ezQtAnimationClipAssetDocumentWindow::onEventTrackEndOperation);
    connect(m_pEventTrackEditor, &ezQtEventTrackEditorWidget::BeginCpChangesEvent, this, &ezQtAnimationClipAssetDocumentWindow::onEventTrackBeginCpChanges);
    connect(m_pEventTrackEditor, &ezQtEventTrackEditorWidget::EndCpChangesEvent, this, &ezQtAnimationClipAssetDocumentWindow::onEventTrackEndCpChanges);
  }

  FinishWindowCreation();

  GetAnimationClipDocument()->m_CommonAssetUiChangeEvent.AddEventHandler(ezMakeDelegate(&ezQtAnimationClipAssetDocumentWindow::CommonAssetUiEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtAnimationClipAssetDocumentWindow::CommandHistoryEventHandler, this));
}

ezQtAnimationClipAssetDocumentWindow::~ezQtAnimationClipAssetDocumentWindow()
{
  GetAnimationClipDocument()->m_CommonAssetUiChangeEvent.RemoveEventHandler(ezMakeDelegate(&ezQtAnimationClipAssetDocumentWindow::CommonAssetUiEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtAnimationClipAssetDocumentWindow::CommandHistoryEventHandler, this));
}

ezAnimationClipAssetDocument* ezQtAnimationClipAssetDocumentWindow::GetAnimationClipDocument()
{
  return static_cast<ezAnimationClipAssetDocument*>(GetDocument());
}

void ezQtAnimationClipAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    ezSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PlaybackPos";
    msg.m_PayloadValue = (double)(m_PlaybackPosition.GetSeconds() / m_ClipDuration.GetSeconds());
    GetDocument()->SendMessageToEngine(&msg);
  }

  {
    ezSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewMesh";
    msg.m_sPayload = GetAnimationClipDocument()->GetProperties()->m_sPreviewMesh;
    GetDocument()->SendMessageToEngine(&msg);
  }

  {
    ezSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "SimulationSpeed";

    if (GetAnimationClipDocument()->GetCommonAssetUiState(ezCommonAssetUiState::Pause) != 0.0f)
      msg.m_PayloadValue = 0.0;
    else
      msg.m_PayloadValue = GetAnimationClipDocument()->GetCommonAssetUiState(ezCommonAssetUiState::SimulationSpeed);

    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  QueryObjectBBox();
}

void ezQtAnimationClipAssetDocumentWindow::QueryObjectBBox(ezInt32 iPurpose /*= 0*/)
{
  ezQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}

void ezQtAnimationClipAssetDocumentWindow::UpdateEventTrackEditor()
{
  auto* pDoc = GetAnimationClipDocument();

  m_pEventTrackEditor->SetData(pDoc->GetProperties()->m_EventTrack, m_ClipDuration.GetSeconds());
}

void ezQtAnimationClipAssetDocumentWindow::InternalRedraw()
{
  if (m_pTimeScrubber == nullptr)
    return;

  if (m_ClipDuration.IsPositive())
  {
    m_Clock.Update();

    const double fSpeed = GetAnimationClipDocument()->GetCommonAssetUiState(ezCommonAssetUiState::SimulationSpeed);

    if (GetAnimationClipDocument()->GetCommonAssetUiState(ezCommonAssetUiState::Pause) == 0)
    {
      m_PlaybackPosition += m_Clock.GetTimeDiff() * fSpeed;
    }

    if (m_PlaybackPosition > m_ClipDuration)
    {
      if (GetAnimationClipDocument()->GetCommonAssetUiState(ezCommonAssetUiState::Loop) != 0)
      {
        m_PlaybackPosition -= m_ClipDuration;
      }
      else
      {
        m_PlaybackPosition = m_ClipDuration;
      }
    }
  }

  m_PlaybackPosition = ezMath::Clamp(m_PlaybackPosition, ezTime::MakeZero(), m_ClipDuration);
  m_pTimeScrubber->SetScrubberPosition(m_PlaybackPosition);
  m_pEventTrackEditor->SetScrubberPosition(m_PlaybackPosition);

  ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  ezQtEngineDocumentWindow::InternalRedraw();
}

void ezQtAnimationClipAssetDocumentWindow::ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg0)
{
  if (auto pMsg = ezDynamicCast<const ezQuerySelectionBBoxResultMsgToEditor*>(pMsg0))
  {
    const ezQuerySelectionBBoxResultMsgToEditor* pMessage = static_cast<const ezQuerySelectionBBoxResultMsgToEditor*>(pMsg);

    if (pMessage->m_vCenter.IsValid() && pMessage->m_vHalfExtents.IsValid())
    {
      m_pViewWidget->SetOrbitVolume(pMessage->m_vCenter, pMessage->m_vHalfExtents.CompMax(ezVec3(0.1f)));
    }
    else
    {
      // try again
      QueryObjectBBox(pMessage->m_iPurpose);
    }

    return;
  }

  if (auto pMsg = ezDynamicCast<const ezSimpleDocumentConfigMsgToEditor*>(pMsg0))
  {
    if (pMsg->m_sWhatToDo == "ClipDuration")
    {
      const ezTime newDuration = pMsg->m_PayloadValue.Get<ezTime>();

      if (m_ClipDuration != newDuration)
      {
        m_ClipDuration = newDuration;

        m_pTimeScrubber->SetDuration(m_ClipDuration);

        UpdateEventTrackEditor();
      }
    }
  }

  ezQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg0);
}

void ezQtAnimationClipAssetDocumentWindow::CommonAssetUiEventHandler(const ezCommonAssetUiState& e)
{
  ezQtEngineDocumentWindow::CommonAssetUiEventHandler(e);

  if (e.m_State == ezCommonAssetUiState::Restart)
  {
    m_PlaybackPosition = ezTime::MakeFromSeconds(-1);
  }
}

void ezQtAnimationClipAssetDocumentWindow::OnScrubberPosChangedEvent(ezUInt64 uiNewScrubberTickPos)
{
  if (m_pTimeScrubber == nullptr || m_ClipDuration.IsZeroOrNegative())
    return;

  m_PlaybackPosition = ezTime::MakeFromSeconds(uiNewScrubberTickPos / 4800.0);
}

void ezQtAnimationClipAssetDocumentWindow::onEventTrackInsertCpAt(ezInt64 tickX, QString value)
{
  auto* pDoc = GetAnimationClipDocument();
  pDoc->InsertEventTrackCpAt(tickX, value.toUtf8().data());
}

void ezQtAnimationClipAssetDocumentWindow::onEventTrackCpMoved(ezUInt32 cpIdx, ezInt64 iTickX)
{
  iTickX = ezMath::Max<ezInt64>(iTickX, 0);

  auto* pDoc = GetAnimationClipDocument();

  ezObjectCommandAccessor accessor(pDoc->GetCommandHistory());

  const ezAbstractProperty* pTrackProp = ezGetStaticRTTI<ezAnimationClipAssetProperties>()->FindPropertyByName("EventTrack");
  const ezUuid trackGuid = accessor.Get<ezUuid>(pDoc->GetPropertyObject(), pTrackProp);
  const ezDocumentObject* pTrackObj = accessor.GetObject(trackGuid);

  const ezVariant cpGuid = pTrackObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<ezUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = iTickX;
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void ezQtAnimationClipAssetDocumentWindow::onEventTrackCpDeleted(ezUInt32 cpIdx)
{
  auto* pDoc = GetAnimationClipDocument();

  ezObjectCommandAccessor accessor(pDoc->GetCommandHistory());

  const ezAbstractProperty* pTrackProp = ezGetStaticRTTI<ezAnimationClipAssetProperties>()->FindPropertyByName("EventTrack");
  const ezUuid trackGuid = accessor.Get<ezUuid>(pDoc->GetPropertyObject(), pTrackProp);
  const ezDocumentObject* pTrackObj = accessor.GetObject(trackGuid);

  const ezVariant cpGuid = pTrackObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  ezRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<ezUuid>();
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void ezQtAnimationClipAssetDocumentWindow::onEventTrackBeginOperation(QString name)
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Events");
}

void ezQtAnimationClipAssetDocumentWindow::onEventTrackEndOperation(bool commit)
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}

void ezQtAnimationClipAssetDocumentWindow::onEventTrackBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void ezQtAnimationClipAssetDocumentWindow::onEventTrackEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdateEventTrackEditor();
}

void ezQtAnimationClipAssetDocumentWindow::CommandHistoryEventHandler(const ezCommandHistoryEvent& e)
{
  // also listen to TransactionCanceled, which is sent when a no-op happens (e.g. asset transform with no change)
  // because the event track data object may still get replaced, and we have to get the new pointer
  if (e.m_Type == ezCommandHistoryEvent::Type::TransactionEnded || e.m_Type == ezCommandHistoryEvent::Type::UndoEnded ||
      e.m_Type == ezCommandHistoryEvent::Type::RedoEnded ||
      e.m_Type == ezCommandHistoryEvent::Type::TransactionCanceled)
  {
    UpdateEventTrackEditor();
  }
}
