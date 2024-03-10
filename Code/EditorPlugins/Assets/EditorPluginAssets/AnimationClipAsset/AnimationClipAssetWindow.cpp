#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

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
    setCentralWidget(pContainer);
  }

  // Property Grid
  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("AnimationClipAssetDockWidget");
    pPropertyPanel->setWindowTitle("Animation Clip Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

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

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_pEventTrackPanel);

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

  // curve editor
  {
    m_pCurveEditPanel = new ezQtDocumentPanel(this, pDocument);
    m_pCurveEditPanel->setObjectName("AnimClipCurveEditDockWidget");
    m_pCurveEditPanel->setWindowTitle("Curves");
    m_pCurveEditPanel->show();

    m_pCurveEditor = new ezQtCurve1DEditorWidget(this);
    m_pCurveEditPanel->setWidget(m_pCurveEditor);

    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::InsertCpEvent, this, &ezQtAnimationClipAssetDocumentWindow::onCurveInsertCpAt);
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::CpMovedEvent, this, &ezQtAnimationClipAssetDocumentWindow::onCurveCpMoved);
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::CpDeletedEvent, this, &ezQtAnimationClipAssetDocumentWindow::onCurveCpDeleted);
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::TangentMovedEvent, this, &ezQtAnimationClipAssetDocumentWindow::onCurveTangentMoved);
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::TangentLinkEvent, this, &ezQtAnimationClipAssetDocumentWindow::onLinkCurveTangents);
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::CpTangentModeEvent, this, &ezQtAnimationClipAssetDocumentWindow::onCurveTangentModeChanged);

    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::BeginOperationEvent, this, &ezQtAnimationClipAssetDocumentWindow::onCurveBeginOperation);
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::EndOperationEvent, this, &ezQtAnimationClipAssetDocumentWindow::onCurveEndOperation);
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::BeginCpChangesEvent, this, &ezQtAnimationClipAssetDocumentWindow::onCurveBeginCpChanges);
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::EndCpChangesEvent, this, &ezQtAnimationClipAssetDocumentWindow::onCurveEndCpChanges);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_pCurveEditPanel);

    UpdateCurveEditor();
  }

  tabifyDockWidget(m_pEventTrackPanel, m_pCurveEditPanel);

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
    msg.m_fPayload = m_PlaybackPosition.GetSeconds() / m_ClipDuration.GetSeconds();
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
      msg.m_fPayload = 0.0;
    else
      msg.m_fPayload = GetAnimationClipDocument()->GetCommonAssetUiState(ezCommonAssetUiState::SimulationSpeed);

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

void ezQtAnimationClipAssetDocumentWindow::UpdateCurveEditor()
{
  auto* pDoc = GetAnimationClipDocument();

  m_Curves.Clear();
  m_Curves.m_bOwnsData = false;

  auto& curves = pDoc->GetProperties()->m_Curves;

  for (auto& curve : curves)
  {
    m_Curves.m_Curves.PushBack(&curve);
  }

  m_pCurveEditor->SetCurveExtents(0.0f, m_ClipDuration.GetSeconds(), true, true);
  m_pCurveEditor->SetCurveRanges(-10, 10); // TODO: make this configurable
  m_pCurveEditor->SetCurves(m_Curves);
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
  m_pCurveEditor->SetScrubberPosition(m_PlaybackPosition);

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
    if (pMsg->m_sName == "ClipDuration")
    {
      const ezTime newDuration = ezTime::MakeFromSeconds(pMsg->m_fPayload);

      if (m_ClipDuration != newDuration)
      {
        m_ClipDuration = newDuration;

        m_pTimeScrubber->SetDuration(m_ClipDuration);

        UpdateEventTrackEditor();
        UpdateCurveEditor();

        m_pEventTrackEditor->FrameCurve();
        m_pCurveEditor->FrameCurve();
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

void ezQtAnimationClipAssetDocumentWindow::onCurveInsertCpAt(ezUInt32 uiCurveIdx, ezInt64 tickX, double newPosY)
{
  auto* pDoc = static_cast<ezAnimationClipAssetDocument*>(GetDocument());

  ezCommandHistory* history = pDoc->GetCommandHistory();

  if (pDoc->GetPropertyObject()->GetTypeAccessor().GetCount("Curves") == 0)
  {
    // no curves allocated yet, add one

    ezAddObjectCommand cmdAddCurve;
    cmdAddCurve.m_Parent = pDoc->GetPropertyObject()->GetGuid();
    cmdAddCurve.m_sParentProperty = "Curves";
    cmdAddCurve.m_pType = ezGetStaticRTTI<ezSingleCurveData>();
    cmdAddCurve.m_Index = -1;

    cmdAddCurve.m_NewObjectGuid = ezUuid::MakeUuid();
    history->AddCommand(cmdAddCurve).AssertSuccess();
  }

  const ezVariant curveGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Curves", uiCurveIdx);

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = curveGuid.Get<ezUuid>();
  cmdAdd.m_NewObjectGuid = ezUuid::MakeUuid();
  cmdAdd.m_sParentProperty = "ControlPoints";
  cmdAdd.m_pType = ezGetStaticRTTI<ezCurveControlPointData>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd).AssertSuccess();

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = tickX;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Value";
  cmdSet.m_NewValue = newPosY;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "LeftTangent";
  cmdSet.m_NewValue = ezVec2(-0.1f, 0.0f);
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "RightTangent";
  cmdSet.m_NewValue = ezVec2(+0.1f, 0.0f);
  history->AddCommand(cmdSet).AssertSuccess();
}

void ezQtAnimationClipAssetDocumentWindow::onCurveCpMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, ezInt64 iTickX, double newPosY)
{
  iTickX = ezMath::Max<ezInt64>(iTickX, 0);

  auto* pDoc = static_cast<ezAnimationClipAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const ezVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const ezDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<ezUuid>());
  const ezVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<ezUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = iTickX;
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Value";
  cmdSet.m_NewValue = newPosY;
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}


void ezQtAnimationClipAssetDocumentWindow::onCurveCpDeleted(ezUInt32 curveIdx, ezUInt32 cpIdx)
{
  auto* pDoc = static_cast<ezAnimationClipAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const ezVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const ezDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<ezUuid>());
  const ezVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  ezRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<ezUuid>();
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}


void ezQtAnimationClipAssetDocumentWindow::onCurveTangentMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent)
{
  auto* pDoc = static_cast<ezAnimationClipAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const ezVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const ezDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<ezUuid>());
  const ezVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<ezUuid>();

  // clamp tangents to one side
  if (rightTangent)
    newPosX = ezMath::Max(newPosX, 0.0f);
  else
    newPosX = ezMath::Min(newPosX, 0.0f);

  cmdSet.m_sProperty = rightTangent ? "RightTangent" : "LeftTangent";
  cmdSet.m_NewValue = ezVec2(newPosX, newPosY);
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}


void ezQtAnimationClipAssetDocumentWindow::onLinkCurveTangents(ezUInt32 curveIdx, ezUInt32 cpIdx, bool bLink)
{
  auto* pDoc = static_cast<ezAnimationClipAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const ezVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const ezDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<ezUuid>());
  const ezVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  ezSetObjectPropertyCommand cmdLink;
  cmdLink.m_Object = cpGuid.Get<ezUuid>();
  cmdLink.m_sProperty = "Linked";
  cmdLink.m_NewValue = bLink;
  GetDocument()->GetCommandHistory()->AddCommand(cmdLink).AssertSuccess();

  if (bLink)
  {
    const ezVec2 leftTangent = pDoc->GetProperties()->m_Curves[curveIdx].m_ControlPoints[cpIdx].m_LeftTangent;
    const ezVec2 rightTangent(-leftTangent.x, -leftTangent.y);

    onCurveTangentMoved(curveIdx, cpIdx, rightTangent.x, rightTangent.y, true);
  }
}


void ezQtAnimationClipAssetDocumentWindow::onCurveTangentModeChanged(ezUInt32 curveIdx, ezUInt32 cpIdx, bool rightTangent, int mode)
{
  auto* pDoc = static_cast<ezAnimationClipAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const ezVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const ezDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<ezUuid>());
  const ezVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  ezSetObjectPropertyCommand cmd;
  cmd.m_Object = cpGuid.Get<ezUuid>();
  cmd.m_sProperty = rightTangent ? "RightTangentMode" : "LeftTangentMode";
  cmd.m_NewValue = mode;
  GetDocument()->GetCommandHistory()->AddCommand(cmd).AssertSuccess();

  // sync current curve back
  if (false)
  {
    // generally works, but would need some work to make it perfect

    ezCurve1D curve;
    pDoc->GetProperties()->m_Curves[curveIdx].ConvertToRuntimeData(curve);
    curve.SortControlPoints();
    curve.ApplyTangentModes();

    for (ezUInt32 i = 0; i < curve.GetNumControlPoints(); ++i)
    {
      const auto& cp = curve.GetControlPoint(i);
      if (cp.m_uiOriginalIndex == cpIdx)
      {
        if (rightTangent)
          onCurveTangentMoved(curveIdx, cpIdx, cp.m_RightTangent.x, cp.m_RightTangent.y, true);
        else
          onCurveTangentMoved(curveIdx, cpIdx, cp.m_LeftTangent.x, cp.m_LeftTangent.y, false);

        break;
      }
    }
  }
}


void ezQtAnimationClipAssetDocumentWindow::onCurveBeginOperation(QString name)
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands(name.toUtf8().data());
}

void ezQtAnimationClipAssetDocumentWindow::onCurveEndOperation(bool commit)
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();

  UpdateCurveEditor();
}

void ezQtAnimationClipAssetDocumentWindow::onCurveBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void ezQtAnimationClipAssetDocumentWindow::onCurveEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdateCurveEditor();
}

void ezQtAnimationClipAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  UpdateEventTrackEditor();
  UpdateCurveEditor();
}

void ezQtAnimationClipAssetDocumentWindow::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case ezDocumentObjectStructureEvent::Type::AfterReset:
    case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    case ezDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      UpdateEventTrackEditor();
      UpdateCurveEditor();
      break;
  }
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
    UpdateCurveEditor();
  }
}
