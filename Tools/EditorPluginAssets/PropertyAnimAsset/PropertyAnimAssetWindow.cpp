#include <PCH.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAssetWindow.moc.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimModel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <Foundation/Image/ImageConversion.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>
#include <GuiFoundation/Widgets/EventTrackEditorWidget.moc.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimViewWidget.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>
#include <GuiFoundation/Widgets/TimeScrubberWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLayout>
#include <QTimer>
#include <QToolBar>
#include <QPushButton>
#include <qevent.h>
#include <QInputDialog>

ezQtPropertyAnimAssetDocumentWindow::ezQtPropertyAnimAssetDocumentWindow(ezPropertyAnimAssetDocument* pDocument) : ezQtGameObjectDocumentWindow(pDocument)
{
  auto ViewFactory = [](ezQtEngineDocumentWindow* pWindow, ezEngineViewConfig* pConfig) -> ezQtEngineViewWidget*
  {
    ezQtPropertyAnimViewWidget* pWidget = new ezQtPropertyAnimViewWidget(nullptr, static_cast<ezQtPropertyAnimAssetDocumentWindow*>(pWindow), pConfig);
    pWindow->AddViewWidget(pWidget);
    return pWidget;
  };
  m_pQuadViewWidget = new ezQtQuadViewWidget(pDocument, this, ViewFactory, "PropertyAnimAssetViewToolBar");

  pDocument->SetEditToolConfigDelegate([this](ezGameObjectEditTool* pTool)
  {
    pTool->ConfigureTool(static_cast<ezGameObjectDocument*>(GetDocument()), this, this);
  });

  pDocument->m_PropertyAnimEvents.AddEventHandler(ezMakeDelegate(&ezQtPropertyAnimAssetDocumentWindow::PropertyAnimAssetEventHandler, this));

  setCentralWidget(m_pQuadViewWidget);
  SetTargetFramerate(25);

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "PropertyAnimAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "PropertyAnimAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("PropertyAnimAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Game Object Graph
  {
    std::unique_ptr<ezQtDocumentTreeModel> ptr(new ezQtGameObjectModel(pDocument, "TempObjects"));
    ezQtDocumentPanel* pGameObjectPanel = new ezQtGameObjectPanel(this, pDocument, "PropertyAnimAsset_ScenegraphContextMenu", std::move(ptr));
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pGameObjectPanel);
  }

  // Property Grid
  {
    ezQtDocumentPanel* pPanel = new ezQtDocumentPanel(this);
    pPanel->setObjectName("PropertyAnimAssetDockWidget");
    pPanel->setWindowTitle("Properties");
    pPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPanel, pDocument);
    pPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPanel);
  }

  // Property Tree View
  {
    ezQtDocumentPanel* pPanel = new ezQtDocumentPanel(this);
    pPanel->setObjectName("PropertyAnimPropertiesDockWidget");
    pPanel->setWindowTitle("Properties");
    pPanel->show();

    m_pPropertyTreeView = new ezQtPropertyAnimAssetTreeView(pPanel);
    m_pPropertyTreeView->setHeaderHidden(true);
    m_pPropertyTreeView->setRootIsDecorated(true);
    m_pPropertyTreeView->setUniformRowHeights(true);
    m_pPropertyTreeView->setExpandsOnDoubleClick(false);
    pPanel->setWidget(m_pPropertyTreeView);

    connect(m_pPropertyTreeView, &ezQtPropertyAnimAssetTreeView::DeleteSelectedItemsEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onDeleteSelectedItems);
    connect(m_pPropertyTreeView, &ezQtPropertyAnimAssetTreeView::RebindSelectedItemsEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onRebindSelectedItems);

    connect(m_pPropertyTreeView, &QTreeView::doubleClicked, this, &ezQtPropertyAnimAssetDocumentWindow::onTreeItemDoubleClicked);
    connect(m_pPropertyTreeView, &ezQtPropertyAnimAssetTreeView::FrameSelectedItemsEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onFrameSelectedTracks);

    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanel);
  }

  // Property Model
  {
    m_pPropertiesModel = new ezQtPropertyAnimModel(GetPropertyAnimDocument(), this);
    m_pPropertyTreeView->setModel(m_pPropertiesModel);
    m_pPropertyTreeView->expandToDepth(2);
    m_pPropertyTreeView->initialize();
  }

  // Selection Model
  {
    m_pPropertyTreeView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    m_pPropertyTreeView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

    m_pSelectionModel = new QItemSelectionModel(m_pPropertiesModel, this);
    m_pPropertyTreeView->setSelectionModel(m_pSelectionModel);

    connect(m_pSelectionModel, &QItemSelectionModel::selectionChanged, this, &ezQtPropertyAnimAssetDocumentWindow::onSelectionChanged);
  }

  // Float Curve Panel
  {
    m_pCurvePanel = new ezQtDocumentPanel(this);
    m_pCurvePanel->setObjectName("PropertyAnimFloatCurveDockWidget");
    m_pCurvePanel->setWindowTitle("Curves");
    m_pCurvePanel->show();

    m_pCurveEditor = new ezQtCurve1DEditorWidget(m_pCurvePanel);
    m_pCurvePanel->setWidget(m_pCurveEditor);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_pCurvePanel);
  }

  // Color Gradient Panel
  {
    m_pColorGradientPanel = new ezQtDocumentPanel(this);
    m_pColorGradientPanel->setObjectName("PropertyAnimColorGradientDockWidget");
    m_pColorGradientPanel->setWindowTitle("Color Gradient");
    m_pColorGradientPanel->show();

    m_pGradientEditor = new ezQtColorGradientEditorWidget(m_pColorGradientPanel);
    m_pColorGradientPanel->setWidget(m_pGradientEditor);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_pColorGradientPanel);
  }

  // Event Track Panel
  {
    m_pEventTrackPanel = new ezQtDocumentPanel(this);
    m_pEventTrackPanel->setObjectName("PropertyAnimEventTrackDockWidget");
    m_pEventTrackPanel->setWindowTitle("Event Track");
    m_pEventTrackPanel->show();

    m_pEventTrackEditor = new ezQtEventTrackEditorWidget(m_pEventTrackPanel);
    m_pEventTrackPanel->setWidget(m_pEventTrackEditor);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_pEventTrackPanel);
  }

  // Time Scrubber
  {
    m_pScrubberToolbar = new ezQtTimeScrubberToolbar(this);
    connect(m_pScrubberToolbar, &ezQtTimeScrubberToolbar::ScrubberPosChangedEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onScrubberPosChanged);
    connect(m_pScrubberToolbar, &ezQtTimeScrubberToolbar::PlayPauseEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onPlayPauseClicked);
    connect(m_pScrubberToolbar, &ezQtTimeScrubberToolbar::RepeatEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onRepeatClicked);
    connect(m_pScrubberToolbar, &ezQtTimeScrubberToolbar::DurationChangedEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onDurationChangedEvent);
    connect(m_pScrubberToolbar, &ezQtTimeScrubberToolbar::AdjustDurationEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onAdjustDurationClicked);

    addToolBar(Qt::ToolBarArea::BottomToolBarArea, m_pScrubberToolbar);
  }

  // this would show the document properties
  //pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  // Curve editor events
  {
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::InsertCpEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onCurveInsertCpAt);
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::CpMovedEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onCurveCpMoved);
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::CpDeletedEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onCurveCpDeleted);
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::TangentMovedEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onCurveTangentMoved);
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::TangentLinkEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onLinkCurveTangents);
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::CpTangentModeEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onCurveTangentModeChanged);

    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::BeginOperationEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onCurveBeginOperation);
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::EndOperationEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onCurveEndOperation);
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::BeginCpChangesEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onCurveBeginCpChanges);
    connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::EndCpChangesEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onCurveEndCpChanges);
  }

  // Gradient editor events
  {
    connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::ColorCpAdded, this, &ezQtPropertyAnimAssetDocumentWindow::onGradientColorCpAdded);
    connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::ColorCpMoved, this, &ezQtPropertyAnimAssetDocumentWindow::onGradientColorCpMoved);
    connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::ColorCpDeleted, this, &ezQtPropertyAnimAssetDocumentWindow::onGradientColorCpDeleted);
    connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::ColorCpChanged, this, &ezQtPropertyAnimAssetDocumentWindow::onGradientColorCpChanged);

    connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::AlphaCpAdded, this, &ezQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpAdded);
    connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::AlphaCpMoved, this, &ezQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpMoved);
    connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::AlphaCpDeleted, this, &ezQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpDeleted);
    connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::AlphaCpChanged, this, &ezQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpChanged);

    connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::IntensityCpAdded, this, &ezQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpAdded);
    connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::IntensityCpMoved, this, &ezQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpMoved);
    connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::IntensityCpDeleted, this, &ezQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpDeleted);
    connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::IntensityCpChanged, this, &ezQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpChanged);

    connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::BeginOperation, this, &ezQtPropertyAnimAssetDocumentWindow::onGradientBeginOperation);
    connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::EndOperation, this, &ezQtPropertyAnimAssetDocumentWindow::onGradientEndOperation);

    //connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::NormalizeRange, this, &ezQtPropertyAnimAssetDocumentWindow::onGradientNormalizeRange);
  }

  // Event track editor events
  {
    connect(m_pEventTrackEditor, &ezQtEventTrackEditorWidget::InsertCpEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onEventTrackInsertCpAt);
    connect(m_pEventTrackEditor, &ezQtEventTrackEditorWidget::CpMovedEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onEventTrackCpMoved);
    connect(m_pEventTrackEditor, &ezQtEventTrackEditorWidget::CpDeletedEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onEventTrackCpDeleted);

    connect(m_pEventTrackEditor, &ezQtEventTrackEditorWidget::BeginOperationEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onEventTrackBeginOperation);
    connect(m_pEventTrackEditor, &ezQtEventTrackEditorWidget::EndOperationEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onEventTrackEndOperation);
    connect(m_pEventTrackEditor, &ezQtEventTrackEditorWidget::BeginCpChangesEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onEventTrackBeginCpChanges);
    connect(m_pEventTrackEditor, &ezQtEventTrackEditorWidget::EndCpChangesEvent, this, &ezQtPropertyAnimAssetDocumentWindow::onEventTrackEndCpChanges);
  }

  //GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtPropertyAnimAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezQtPropertyAnimAssetDocumentWindow::StructureEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtPropertyAnimAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtPropertyAnimAssetDocumentWindow::CommandHistoryEventHandler, this));

  FinishWindowCreation();

  {
    const ezUInt64 uiDuration = GetPropertyAnimDocument()->GetAnimationDurationTicks();
    m_pScrubberToolbar->SetDuration(uiDuration, GetPropertyAnimDocument()->GetProperties()->m_uiFramesPerSecond);
  }

  UpdateCurveEditor();
  UpdateGradientEditor();
  UpdateEventTrackEditor();
}

ezQtPropertyAnimAssetDocumentWindow::~ezQtPropertyAnimAssetDocumentWindow()
{
  GetPropertyAnimDocument()->m_PropertyAnimEvents.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyAnimAssetDocumentWindow::PropertyAnimAssetEventHandler, this));
  //GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyAnimAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyAnimAssetDocumentWindow::StructureEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyAnimAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyAnimAssetDocumentWindow::CommandHistoryEventHandler, this));
}

void ezQtPropertyAnimAssetDocumentWindow::ToggleViews(QWidget* pView)
{
  m_pQuadViewWidget->ToggleViews(pView);
}

ezObjectAccessorBase* ezQtPropertyAnimAssetDocumentWindow::GetObjectAccessor()
{
  return GetPropertyAnimDocument()->GetObjectAccessor();
}

bool ezQtPropertyAnimAssetDocumentWindow::CanDuplicateSelection() const
{
  return false;
}

void ezQtPropertyAnimAssetDocumentWindow::DuplicateSelection()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}


void ezQtPropertyAnimAssetDocumentWindow::InternalRedraw()
{
  ezEditorInputContext::UpdateActiveInputContext();
  {
    // do not try to redraw while the process is crashed, it is obviously futile
    if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
      return;

    {
      ezSimulationSettingsMsgToEngine msg;
      msg.m_bSimulateWorld = false;
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    {
      ezGridSettingsMsgToEngine msg = GetGridSettings();
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    {
      ezGlobalSettingsMsgToEngine msg = GetGlobalSettings();
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    {
      ezWorldSettingsMsgToEngine msg = GetWorldSettings();
      GetEditorEngineConnection()->SendMessage(&msg);
    }

    GetGameObjectDocument()->SendObjectSelection();

    auto pHoveredView = GetHoveredViewWidget();

    for (auto pView : m_ViewWidgets)
    {
      pView->SetEnablePicking(pView == pHoveredView);
      pView->UpdateCameraInterpolation();
      pView->SyncToEngine();
    }
  }
  ezQtEngineDocumentWindow::InternalRedraw();
}

void ezQtPropertyAnimAssetDocumentWindow::PropertyAnimAssetEventHandler(const ezPropertyAnimAssetDocumentEvent& e)
{
  if (e.m_Type == ezPropertyAnimAssetDocumentEvent::Type::AnimationLengthChanged)
  {
    const ezUInt64 uiDuration = e.m_pDocument->GetAnimationDurationTicks();

    m_pScrubberToolbar->SetDuration(uiDuration, e.m_pDocument->GetProperties()->m_uiFramesPerSecond);
    UpdateCurveEditor();
    UpdateGradientEditor();
    UpdateEventTrackEditor();
  }
  else if (e.m_Type == ezPropertyAnimAssetDocumentEvent::Type::ScrubberPositionChanged)
  {
    m_pScrubberToolbar->SetScrubberPosition(e.m_pDocument->GetScrubberPosition());
    m_pCurveEditor->SetScrubberPosition(e.m_pDocument->GetScrubberPosition());
    m_pGradientEditor->SetScrubberPosition(e.m_pDocument->GetScrubberPosition());
    m_pEventTrackEditor->SetScrubberPosition(e.m_pDocument->GetScrubberPosition());
  }
  else if (e.m_Type == ezPropertyAnimAssetDocumentEvent::Type::PlaybackChanged)
  {
    if (!m_bAnimTimerInFlight && GetPropertyAnimDocument()->GetPlayAnimation())
    {
      m_bAnimTimerInFlight = true;
      QTimer::singleShot(0, this, SLOT(onPlaybackTick()));
    }

    m_pScrubberToolbar->SetButtonState(GetPropertyAnimDocument()->GetPlayAnimation(), GetPropertyAnimDocument()->GetRepeatAnimation());
  }
}

void ezQtPropertyAnimAssetDocumentWindow::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  UpdateSelectionData();
}

void ezQtPropertyAnimAssetDocumentWindow::UpdateSelectionData()
{
  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  m_MapSelectionToTrack.Clear();
  m_pGradientToDisplay = nullptr;
  m_iMapGradientToTrack = -1;
  m_CurvesToDisplay.Clear();
  m_CurvesToDisplay.m_bOwnsData = false;
  m_CurvesToDisplay.m_uiFramesPerSecond = pDoc->GetProperties()->m_uiFramesPerSecond;

  ezSet<ezInt32> tracks;

  for (const QModelIndex& selIdx : m_pSelectionModel->selection().indexes())
  {
    ezQtPropertyAnimModelTreeEntry* pTreeItem = reinterpret_cast<ezQtPropertyAnimModelTreeEntry*>(m_pPropertiesModel->data(selIdx, ezQtPropertyAnimModel::UserRoles::TreeItem).value<void*>());

    ezQtPropertyAnimModel* pModel = m_pPropertiesModel;

    auto addRecursive = [&tracks, pModel](auto& self, const ezQtPropertyAnimModelTreeEntry* pTreeItem)->void
    {
      if (pTreeItem->m_pTrack != nullptr)
        tracks.Insert(pTreeItem->m_iTrackIdx);

      for (ezInt32 iChild : pTreeItem->m_Children)
      {
        // cannot use 'addRecursive' here, because the name is not yet fully defined
        self(self, &pModel->GetAllEntries()[iChild]);
      }
    };

    addRecursive(addRecursive, pTreeItem);
  }

  auto& trackArray = pDoc->GetProperties()->m_Tracks;
  for (auto it = tracks.GetIterator(); it.IsValid(); ++it)
  {
    const ezInt32 iTrackIdx = it.Key();

    // this can happen during undo/redo when the selection still names data that has just been removed
    if (iTrackIdx >= (ezInt32)trackArray.GetCount())
      continue;

    if (trackArray[iTrackIdx]->m_Target != ezPropertyAnimTarget::Color)
    {
      m_MapSelectionToTrack.PushBack(iTrackIdx);

      m_CurvesToDisplay.m_Curves.PushBack(&trackArray[iTrackIdx]->m_FloatCurve);
    }
    else
    {
      m_pGradientToDisplay = &trackArray[iTrackIdx]->m_ColorGradient;
      m_iMapGradientToTrack = iTrackIdx;
    }
  }

  m_pCurveEditor->ClearSelection();

  UpdateCurveEditor();
  UpdateGradientEditor();

  if (m_pEventTrackPanel->hasFocus() || m_pEventTrackEditor->hasFocus() || m_pEventTrackEditor->EventTrackEdit->hasFocus())
  {
  }
  else if (!m_CurvesToDisplay.m_Curves.IsEmpty())
  {
    m_pCurvePanel->raise();
  }
  else if (m_pGradientToDisplay != nullptr)
  {
    m_pColorGradientPanel->raise();
  }
}

void ezQtPropertyAnimAssetDocumentWindow::onScrubberPosChanged(ezUInt64 uiTick)
{
  GetPropertyAnimDocument()->SetScrubberPosition(uiTick);
}

void ezQtPropertyAnimAssetDocumentWindow::onDeleteSelectedItems()
{
  auto pDoc = GetPropertyAnimDocument();
  auto pHistory = pDoc->GetCommandHistory();

  pHistory->StartTransaction("Delete Tracks");

  m_pGradientToDisplay = nullptr;
  m_CurvesToDisplay.Clear();

  // delete the tracks with the highest index first, otherwise the lower indices become invalid
  // do this before modifying anything, as m_MapSelectionToTrack will change once the remove commands are executed
  ezHybridArray<ezInt32, 16> sortedTrackIDs;
  {
    for (ezInt32 iTrack : m_MapSelectionToTrack)
    {
      sortedTrackIDs.PushBack(iTrack);
    }

    if (m_iMapGradientToTrack >= 0)
    {
      sortedTrackIDs.PushBack(m_iMapGradientToTrack);
    }

    sortedTrackIDs.Sort();
  }

  for (ezUInt32 i = sortedTrackIDs.GetCount(); i > 0; --i)
  {
    const ezInt32 iTrack = sortedTrackIDs[i - 1];

    const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrack);

    if (trackGuid.IsValid())
    {
      ezRemoveObjectCommand cmd;
      cmd.m_Object = trackGuid.Get<ezUuid>();

      pHistory->AddCommand(cmd);
    }
  }

  m_MapSelectionToTrack.Clear();
  m_iMapGradientToTrack = -1;

  pHistory->FinishTransaction();
}

void ezQtPropertyAnimAssetDocumentWindow::onRebindSelectedItems()
{
  auto pDoc = GetPropertyAnimDocument();
  auto pHistory = pDoc->GetCommandHistory();

  ezHybridArray<ezUuid, 16> rebindTracks;

  for (ezInt32 iTrack : m_MapSelectionToTrack)
  {
    const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrack);

    if (trackGuid.IsValid())
      rebindTracks.PushBack(trackGuid.Get<ezUuid>());
  }

  if (m_iMapGradientToTrack >= 0)
  {
    const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);

    if (trackGuid.IsValid())
      rebindTracks.PushBack(trackGuid.Get<ezUuid>());
  }

  bool ok = false;
  QString result = QInputDialog::getText(this, "Change Animation Binding", "New Binding Path:", QLineEdit::Normal, "", &ok);

  if (!ok)
    return;

  m_pSelectionModel->clear();

  ezStringBuilder path = result.toUtf8().data();;
  path.MakeCleanPath();
  const ezVariant varRes = path.GetData();

  pHistory->StartTransaction("Rebind Tracks");

  for (const ezUuid guid : rebindTracks)
  {
    ezSetObjectPropertyCommand cmdSet;
    cmdSet.m_Object = guid;

    cmdSet.m_sProperty = "ObjectPath";
    cmdSet.m_NewValue = varRes;
    pDoc->GetCommandHistory()->AddCommand(cmdSet);
  }

  pHistory->FinishTransaction();
}

void ezQtPropertyAnimAssetDocumentWindow::onPlaybackTick()
{
  m_bAnimTimerInFlight = false;

  if (!GetPropertyAnimDocument()->GetPlayAnimation())
    return;

  GetPropertyAnimDocument()->ExecuteAnimationPlaybackStep();

  m_bAnimTimerInFlight = true;
  QTimer::singleShot(0, this, SLOT(onPlaybackTick()));
}

void ezQtPropertyAnimAssetDocumentWindow::onPlayPauseClicked()
{
  GetPropertyAnimDocument()->SetPlayAnimation(!GetPropertyAnimDocument()->GetPlayAnimation());
}

void ezQtPropertyAnimAssetDocumentWindow::onRepeatClicked()
{
  GetPropertyAnimDocument()->SetRepeatAnimation(!GetPropertyAnimDocument()->GetRepeatAnimation());
}

void ezQtPropertyAnimAssetDocumentWindow::onAdjustDurationClicked()
{
  GetPropertyAnimDocument()->AdjustDuration();
}

void ezQtPropertyAnimAssetDocumentWindow::onDurationChangedEvent(double duration)
{
  GetPropertyAnimDocument()->SetAnimationDurationTicks((ezUInt64)(duration * 4800.0));
}

void ezQtPropertyAnimAssetDocumentWindow::onTreeItemDoubleClicked(const QModelIndex& index)
{
  ezQtPropertyAnimModelTreeEntry* pTreeItem = reinterpret_cast<ezQtPropertyAnimModelTreeEntry*>(m_pPropertiesModel->data(index, ezQtPropertyAnimModel::UserRoles::TreeItem).value<void*>());

  if (pTreeItem != nullptr && pTreeItem->m_pTrack != nullptr)
  {
    if (pTreeItem->m_pTrack->m_Target == ezPropertyAnimTarget::Color)
    {
      m_pGradientEditor->FrameGradient();
      m_pColorGradientPanel->raise();
    }
    else
    {
      m_pCurveEditor->FrameCurve();
      m_pCurvePanel->raise();
    }
  }
  else
  {
    if (!m_CurvesToDisplay.m_Curves.IsEmpty())
    {
      m_pCurveEditor->FrameCurve();
      m_pCurvePanel->raise();
    }
    else if (m_pGradientToDisplay != nullptr)
    {
      m_pGradientEditor->FrameGradient();
      m_pColorGradientPanel->raise();
    }
  }
}

void ezQtPropertyAnimAssetDocumentWindow::onFrameSelectedTracks()
{
  if (!m_CurvesToDisplay.m_Curves.IsEmpty())
  {
    m_pCurveEditor->FrameCurve();
    m_pCurvePanel->raise();
  }
  else if (m_pGradientToDisplay != nullptr)
  {
    m_pGradientEditor->FrameGradient();
    m_pColorGradientPanel->raise();
  }
}

ezPropertyAnimAssetDocument* ezQtPropertyAnimAssetDocumentWindow::GetPropertyAnimDocument()
{
  return static_cast<ezPropertyAnimAssetDocument*>(GetDocument());
}

//void ezQtPropertyAnimAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
//{
//  if (static_cast<ezPropertyAnimObjectManager*>(GetDocument()->GetObjectManager())->IsTemporary(e.m_pObject, e.m_sProperty))
//    return;
//
//  // TODO: only update what needs to be updated
//
//  //m_bUpdateEventTrackEditor = true;
//  //m_bUpdateCurveEditor = true;
//  //m_bUpdateGradientEditor = true;
//}

void ezQtPropertyAnimAssetDocumentWindow::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  if (e.m_pNewParent && static_cast<ezPropertyAnimObjectManager*>(GetDocument()->GetObjectManager())->IsTemporary(e.m_pNewParent, e.m_sParentProperty))
    return;
  if (e.m_pPreviousParent && static_cast<ezPropertyAnimObjectManager*>(GetDocument()->GetObjectManager())->IsTemporary(e.m_pPreviousParent, e.m_sParentProperty))
    return;

  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
  case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
  case ezDocumentObjectStructureEvent::Type::AfterObjectMoved2:
    UpdateSelectionData();
    break;
  }
}


void ezQtPropertyAnimAssetDocumentWindow::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  // this would show the document properties
  //if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
  //{
  //  // delayed execution
  //  QTimer::singleShot(1, [this]()
  //  {
  //    GetDocument()->GetSelectionManager()->SetSelection(GetPropertyAnimDocument()->GetPropertyObject());
  //  });
  //}
}


void ezQtPropertyAnimAssetDocumentWindow::CommandHistoryEventHandler(const ezCommandHistoryEvent& e)
{
  if (e.m_Type == ezCommandHistoryEvent::Type::TransactionEnded ||
    e.m_Type == ezCommandHistoryEvent::Type::UndoEnded ||
    e.m_Type == ezCommandHistoryEvent::Type::RedoEnded)
  {
    UpdateCurveEditor();
    UpdateGradientEditor();
    UpdateEventTrackEditor();
  }
}

void ezQtPropertyAnimAssetDocumentWindow::UpdateCurveEditor()
{
  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();
  m_pCurveEditor->SetCurves(m_CurvesToDisplay, pDoc->GetAnimationDurationTime().GetSeconds(), true);
}


void ezQtPropertyAnimAssetDocumentWindow::UpdateGradientEditor()
{
  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_pGradientToDisplay == nullptr || m_iMapGradientToTrack < 0)
  {
    // TODO: clear gradient editor ?
    ezColorGradient empty;
    m_pGradientEditor->SetColorGradient(empty);
  }
  else
  {
    ezColorGradient gradient;
    m_pGradientToDisplay->FillGradientData(gradient);
    m_pGradientEditor->SetColorGradient(gradient);
  }
}


void ezQtPropertyAnimAssetDocumentWindow::UpdateEventTrackEditor()
{
  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();
  m_pEventTrackEditor->SetData(GetPropertyAnimDocument()->GetProperties()->m_EventTrack, pDoc->GetAnimationDurationTime().GetSeconds());
}

void ezQtPropertyAnimAssetDocumentWindow::onCurveBeginOperation(QString name)
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands(name.toUtf8().data());
}

void ezQtPropertyAnimAssetDocumentWindow::onCurveEndOperation(bool commit)
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}

void ezQtPropertyAnimAssetDocumentWindow::onCurveBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void ezQtPropertyAnimAssetDocumentWindow::onCurveEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdateCurveEditor();
}

void ezQtPropertyAnimAssetDocumentWindow::onCurveInsertCpAt(ezUInt32 uiCurveIdx, ezInt64 tickX, double clickPosY)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();
  const ezInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  pDoc->InsertCurveCpAt(trackGuid.Get<ezUuid>(), tickX, clickPosY);
}

void ezQtPropertyAnimAssetDocumentWindow::onCurveCpMoved(ezUInt32 uiCurveIdx, ezUInt32 cpIdx, ezInt64 iTickX, double newPosY)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  iTickX = ezMath::Max<ezInt64>(iTickX, 0);

  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  auto pProp = pDoc->GetPropertyObject();

  const ezInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const ezDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<ezUuid>());
  const ezVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const ezDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<ezUuid>());
  const ezVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<ezUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = iTickX;
  pDoc->GetCommandHistory()->AddCommand(cmdSet);

  cmdSet.m_sProperty = "Value";
  cmdSet.m_NewValue = newPosY;
  pDoc->GetCommandHistory()->AddCommand(cmdSet);
}

void ezQtPropertyAnimAssetDocumentWindow::onCurveCpDeleted(ezUInt32 uiCurveIdx, ezUInt32 cpIdx)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  auto pProp = pDoc->GetPropertyObject();

  const ezInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const ezDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<ezUuid>());
  const ezVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const ezDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<ezUuid>());
  const ezVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  ezRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<ezUuid>();
  pDoc->GetCommandHistory()->AddCommand(cmdSet);
}

void ezQtPropertyAnimAssetDocumentWindow::onCurveTangentMoved(ezUInt32 uiCurveIdx, ezUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  auto pProp = pDoc->GetPropertyObject();

  const ezInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const ezDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<ezUuid>());
  const ezVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

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
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet);
}

void ezQtPropertyAnimAssetDocumentWindow::onLinkCurveTangents(ezUInt32 uiCurveIdx, ezUInt32 cpIdx, bool bLink)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  auto pProp = pDoc->GetPropertyObject();

  const ezInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const ezDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<ezUuid>());
  const ezVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const ezDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<ezUuid>());
  const ezVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  ezSetObjectPropertyCommand cmdLink;
  cmdLink.m_Object = cpGuid.Get<ezUuid>();
  cmdLink.m_sProperty = "Linked";
  cmdLink.m_NewValue = bLink;
  GetDocument()->GetCommandHistory()->AddCommand(cmdLink);

  if (bLink)
  {
    const ezVec2 leftTangent = pDoc->GetProperties()->m_Tracks[iTrackIdx]->m_FloatCurve.m_ControlPoints[cpIdx].m_LeftTangent;
    const ezVec2 rightTangent(-leftTangent.x, -leftTangent.y);

    onCurveTangentMoved(uiCurveIdx, cpIdx, rightTangent.x, rightTangent.y, true);
  }
}

void ezQtPropertyAnimAssetDocumentWindow::onCurveTangentModeChanged(ezUInt32 uiCurveIdx, ezUInt32 cpIdx, bool rightTangent, int mode)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  auto pProp = pDoc->GetPropertyObject();

  const ezInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const ezDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<ezUuid>());
  const ezVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const ezDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<ezUuid>());
  const ezVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  ezSetObjectPropertyCommand cmd;
  cmd.m_Object = cpGuid.Get<ezUuid>();
  cmd.m_sProperty = rightTangent ? "RightTangentMode" : "LeftTangentMode";
  cmd.m_NewValue = mode;
  GetDocument()->GetCommandHistory()->AddCommand(cmd);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


void ezQtPropertyAnimAssetDocumentWindow::onGradientColorCpAdded(double posX, const ezColorGammaUB& color)
{
  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  ezInt64 tickX = ezColorGradientAssetData::TickFromTime(posX);
  pDoc->InsertGradientColorCpAt(trackGuid.Get<ezUuid>(), tickX, color);
}


void ezQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpAdded(double posX, ezUInt8 alpha)
{
  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  ezInt64 tickX = ezColorGradientAssetData::TickFromTime(posX);
  pDoc->InsertGradientAlphaCpAt(trackGuid.Get<ezUuid>(), tickX, alpha);
}


void ezQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpAdded(double posX, float intensity)
{
  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  ezInt64 tickX = ezColorGradientAssetData::TickFromTime(posX);
  pDoc->InsertGradientIntensityCpAt(trackGuid.Get<ezUuid>(), tickX, intensity);
}

void ezQtPropertyAnimAssetDocumentWindow::MoveGradientCP(ezInt32 idx, double newPosX, const char* szArrayName)
{
  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const ezDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<ezUuid>());
  const ezUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<ezUuid>();
  const ezDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  ezVariant objGuid = gradientObject->GetTypeAccessor().GetValue(szArrayName, idx);

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Move Control Point");

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<ezUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = pDoc->GetProperties()->m_Tracks[m_iMapGradientToTrack]->m_ColorGradient.TickFromTime(newPosX);
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}

void ezQtPropertyAnimAssetDocumentWindow::onGradientColorCpMoved(ezInt32 idx, double newPosX)
{
  MoveGradientCP(idx, newPosX, "ColorCPs");
}

void ezQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpMoved(ezInt32 idx, double newPosX)
{
  MoveGradientCP(idx, newPosX, "AlphaCPs");
}


void ezQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpMoved(ezInt32 idx, double newPosX)
{
  MoveGradientCP(idx, newPosX, "IntensityCPs");
}

void ezQtPropertyAnimAssetDocumentWindow::RemoveGradientCP(ezInt32 idx, const char* szArrayName)
{
  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const ezDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<ezUuid>());
  const ezUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<ezUuid>();
  const ezDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  ezVariant objGuid = gradientObject->GetTypeAccessor().GetValue(szArrayName, idx);

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Control Point");

  ezRemoveObjectCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<ezUuid>();
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}

void ezQtPropertyAnimAssetDocumentWindow::onGradientColorCpDeleted(ezInt32 idx)
{
  RemoveGradientCP(idx, "ColorCPs");
}

void ezQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpDeleted(ezInt32 idx)
{
  RemoveGradientCP(idx, "AlphaCPs");
}

void ezQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpDeleted(ezInt32 idx)
{
  RemoveGradientCP(idx, "IntensityCPs");
}

void ezQtPropertyAnimAssetDocumentWindow::onGradientColorCpChanged(ezInt32 idx, const ezColorGammaUB& color)
{
  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const ezDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<ezUuid>());
  const ezUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<ezUuid>();
  const ezDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  ezVariant objGuid = gradientObject->GetTypeAccessor().GetValue("ColorCPs", idx);

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Color");

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<ezUuid>();

  cmdSet.m_sProperty = "Red";
  cmdSet.m_NewValue = color.r;
  history->AddCommand(cmdSet);

  cmdSet.m_sProperty = "Green";
  cmdSet.m_NewValue = color.g;
  history->AddCommand(cmdSet);

  cmdSet.m_sProperty = "Blue";
  cmdSet.m_NewValue = color.b;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}


void ezQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpChanged(ezInt32 idx, ezUInt8 alpha)
{
  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const ezDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<ezUuid>());
  const ezUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<ezUuid>();
  const ezDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  ezVariant objGuid = gradientObject->GetTypeAccessor().GetValue("AlphaCPs", idx);

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Alpha");

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<ezUuid>();

  cmdSet.m_sProperty = "Alpha";
  cmdSet.m_NewValue = alpha;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}

void ezQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpChanged(ezInt32 idx, float intensity)
{
  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const ezDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<ezUuid>());
  const ezUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<ezUuid>();
  const ezDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  ezVariant objGuid = gradientObject->GetTypeAccessor().GetValue("IntensityCPs", idx);

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Intensity");

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<ezUuid>();

  cmdSet.m_sProperty = "Intensity";
  cmdSet.m_NewValue = intensity;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}

void ezQtPropertyAnimAssetDocumentWindow::onGradientBeginOperation()
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Gradient");
}

void ezQtPropertyAnimAssetDocumentWindow::onGradientEndOperation(bool commit)
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}

void ezQtPropertyAnimAssetDocumentWindow::onEventTrackInsertCpAt(ezInt64 tickX, QString value)
{
  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();
  pDoc->InsertEventTrackCpAt(tickX, value.toUtf8().data());

}

void ezQtPropertyAnimAssetDocumentWindow::onEventTrackCpMoved(ezUInt32 cpIdx, ezInt64 iTickX)
{
  iTickX = ezMath::Max<ezInt64>(iTickX, 0);

  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  ezObjectCommandAccessor accessor(pDoc->GetCommandHistory());

  const ezAbstractProperty* pTrackProp = ezGetStaticRTTI<ezPropertyAnimationTrackGroup>()->FindPropertyByName("EventTrack");
  const ezUuid trackGuid = accessor.Get<ezUuid>(pDoc->GetPropertyObject(), pTrackProp);
  const ezDocumentObject* pTrackObj = accessor.GetObject(trackGuid);

  const ezVariant cpGuid = pTrackObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<ezUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = iTickX;
  pDoc->GetCommandHistory()->AddCommand(cmdSet);
}

void ezQtPropertyAnimAssetDocumentWindow::onEventTrackCpDeleted(ezUInt32 cpIdx)
{
  ezPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  ezObjectCommandAccessor accessor(pDoc->GetCommandHistory());

  const ezAbstractProperty* pTrackProp = ezGetStaticRTTI<ezPropertyAnimationTrackGroup>()->FindPropertyByName("EventTrack");
  const ezUuid trackGuid = accessor.Get<ezUuid>(pDoc->GetPropertyObject(), pTrackProp);
  const ezDocumentObject* pTrackObj = accessor.GetObject(trackGuid);

  const ezVariant cpGuid = pTrackObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  ezRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<ezUuid>();
  pDoc->GetCommandHistory()->AddCommand(cmdSet);
}

void ezQtPropertyAnimAssetDocumentWindow::onEventTrackBeginOperation(QString name)
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Events");
}

void ezQtPropertyAnimAssetDocumentWindow::onEventTrackEndOperation(bool commit)
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}

void ezQtPropertyAnimAssetDocumentWindow::onEventTrackBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void ezQtPropertyAnimAssetDocumentWindow::onEventTrackEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdateEventTrackEditor();
}

//////////////////////////////////////////////////////////////////////////

ezQtPropertyAnimAssetTreeView::ezQtPropertyAnimAssetTreeView(QWidget* parent)
  : QTreeView(parent)
{
  setContextMenuPolicy(Qt::ContextMenuPolicy::DefaultContextMenu);
}

void ezQtPropertyAnimAssetTreeView::initialize()
{
  connect(model(), &QAbstractItemModel::modelAboutToBeReset, this, &ezQtPropertyAnimAssetTreeView::onBeforeModelReset);
  connect(model(), &QAbstractItemModel::modelReset, this, &ezQtPropertyAnimAssetTreeView::onAfterModelReset);
}

void ezQtPropertyAnimAssetTreeView::storeExpandState(const QModelIndex& parent)
{
  const QAbstractItemModel* pModel = model();

  const ezUInt32 numRows = pModel->rowCount(parent);
  for (ezUInt32 row = 0; row < numRows; ++row)
  {
    QModelIndex idx = pModel->index(row, 0, parent);

    const bool expanded = isExpanded(idx);

    QString path = pModel->data(idx, ezQtPropertyAnimModel::UserRoles::Path).toString();

    if (!expanded)
      m_notExpandedState.insert(path);

    storeExpandState(idx);
  }
}

void ezQtPropertyAnimAssetTreeView::restoreExpandState(const QModelIndex& parent, QModelIndexList& newSelection)
{
  const QAbstractItemModel* pModel = model();

  const ezUInt32 numRows = pModel->rowCount(parent);
  for (ezUInt32 row = 0; row < numRows; ++row)
  {
    QModelIndex idx = pModel->index(row, 0, parent);

    QString path = pModel->data(idx, ezQtPropertyAnimModel::UserRoles::Path).toString();

    const bool notExpanded = m_notExpandedState.contains(path);

    if (!notExpanded)
      setExpanded(idx, true);

    if (m_selectedItems.contains(path))
      newSelection.append(idx);

    restoreExpandState(idx, newSelection);
  }
}

void ezQtPropertyAnimAssetTreeView::onBeforeModelReset()
{
  m_notExpandedState.clear();
  m_selectedItems.clear();

  storeExpandState(QModelIndex());

  const QAbstractItemModel* pModel = model();

  for (QModelIndex idx : selectionModel()->selectedRows())
  {
    QString path = pModel->data(idx, ezQtPropertyAnimModel::UserRoles::Path).toString();
    m_selectedItems.insert(path);
  }
}

void ezQtPropertyAnimAssetTreeView::onAfterModelReset()
{
  QModelIndexList newSelection;
  restoreExpandState(QModelIndex(), newSelection);

  // changing the selection is not possible in onAfterModelReset, probably because the items are not yet fully valid
  // has to be done shortly after
  QTimer::singleShot(0, this, [this, newSelection]()
  {
    selectionModel()->clearSelection();
    for (const auto& idx : newSelection)
    {
      selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::Select | QItemSelectionModel::SelectionFlag::Rows);
    }
  });
}

void ezQtPropertyAnimAssetTreeView::keyPressEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key::Key_Delete)
  {
    emit DeleteSelectedItemsEvent();
  }
  else
  {
    QTreeView::keyPressEvent(e);
  }
}

void ezQtPropertyAnimAssetTreeView::contextMenuEvent(QContextMenuEvent *event)
{
  QMenu m;
  QAction* pFrameAction = m.addAction("Frame Curve");
  QAction* pRemoveAction = m.addAction("Remove Track");
  QAction* pBindingAction = m.addAction("Change Binding...");
  m.setDefaultAction(pFrameAction);

  pRemoveAction->setShortcut(Qt::Key_Delete);

  connect(pFrameAction, &QAction::triggered, this, [this](bool) { emit FrameSelectedItemsEvent(); });
  connect(pRemoveAction, &QAction::triggered, this, [this](bool) { emit DeleteSelectedItemsEvent(); });
  connect(pBindingAction, &QAction::triggered, this, [this](bool) { emit RebindSelectedItemsEvent(); });

  m.exec(QCursor::pos());
}
