#include <PCH.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimModel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QLabel>
#include <QLayout>
#include <Foundation/Image/ImageConversion.h>
#include <QTreeView>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <QItemSelectionModel>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>
#include <QTimer>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>

ezQtPropertyAnimAssetDocumentWindow::ezQtPropertyAnimAssetDocumentWindow(ezDocument* pDocument) : ezQtDocumentWindow(pDocument)
{
  auto pDoc = static_cast<ezGameObjectDocument*>(pDocument);
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
    std::unique_ptr<ezQtDocumentTreeModel> ptr(new ezQtGameObjectModel(pDoc, "TempObjects"));
    ezQtDocumentPanel* pGameObjectPanel = new ezQtGameObjectPanel(this, pDoc, std::move(ptr), "ScenegraphContextMenu");
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

    m_pPropertyTreeView = new QTreeView(pPanel);
    m_pPropertyTreeView->setHeaderHidden(true);
    m_pPropertyTreeView->setRootIsDecorated(true);
    m_pPropertyTreeView->setUniformRowHeights(true);
    pPanel->setWidget(m_pPropertyTreeView);

    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanel);
  }

  // Property Model
  {
    m_pPropertiesModel = new ezQtPropertyAnimModel(static_cast<ezPropertyAnimAssetDocument*>(pDocument), this);
    m_pPropertyTreeView->setModel(m_pPropertiesModel);
    m_pPropertyTreeView->expandToDepth(1);
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
    ezQtDocumentPanel* pPanel = new ezQtDocumentPanel(this);
    pPanel->setObjectName("PropertyAnimFloatCurveDockWidget");
    pPanel->setWindowTitle("Curves");
    pPanel->show();

    m_pCurveEditor = new ezQtCurve1DEditorWidget(pPanel);
    pPanel->setWidget(m_pCurveEditor);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, pPanel);
  }

  // Color Gradient Panel
  {
    ezQtDocumentPanel* pPanel = new ezQtDocumentPanel(this);
    pPanel->setObjectName("PropertyAnimColorGradientDockWidget");
    pPanel->setWindowTitle("Color Gradient");
    pPanel->show();

    m_pGradientEditor = new ezQtColorGradientEditorWidget(pPanel);
    pPanel->setWidget(m_pGradientEditor);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, pPanel);
  }

  pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

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

  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtPropertyAnimAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezQtPropertyAnimAssetDocumentWindow::StructureEventHandler, this));
  }
  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtPropertyAnimAssetDocumentWindow::SelectionEventHandler, this));

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

  FinishWindowCreation();
}

ezQtPropertyAnimAssetDocumentWindow::~ezQtPropertyAnimAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyAnimAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyAnimAssetDocumentWindow::StructureEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyAnimAssetDocumentWindow::SelectionEventHandler, this));
}

void ezQtPropertyAnimAssetDocumentWindow::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

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

  UpdateCurveEditor();
  UpdateGradientEditor();
}


ezPropertyAnimAssetDocument* ezQtPropertyAnimAssetDocumentWindow::GetPropertyAnimDocument()
{
  return static_cast<ezPropertyAnimAssetDocument*>(GetDocument());
}

void ezQtPropertyAnimAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  UpdateCurveEditor();
  UpdateGradientEditor();
}

void ezQtPropertyAnimAssetDocumentWindow::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
  case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
  case ezDocumentObjectStructureEvent::Type::AfterObjectMoved2:
    UpdateCurveEditor();
    UpdateGradientEditor();
    break;
  }
}


void ezQtPropertyAnimAssetDocumentWindow::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
  {
    // delayed execution
    QTimer::singleShot(1, [this]()
    {
      GetDocument()->GetSelectionManager()->SetSelection(GetPropertyAnimDocument()->GetPropertyObject());
    });
  }
}

void ezQtPropertyAnimAssetDocumentWindow::UpdateCurveEditor()
{
  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());
  m_pCurveEditor->SetCurves(m_CurvesToDisplay, pDoc->GetAnimationDuration().GetSeconds());
}


void ezQtPropertyAnimAssetDocumentWindow::UpdateGradientEditor()
{
  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

  if (m_pGradientToDisplay == nullptr)
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

  UpdateCurveEditor();
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

  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

  ezCommandHistory* history = pDoc->GetCommandHistory();
  history->StartTransaction("Insert Control Point");

  const ezInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const ezDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<ezUuid>());
  const ezVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = curveGuid.Get<ezUuid>();
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_sParentProperty = "ControlPoints";
  cmdAdd.m_pType = ezGetStaticRTTI<ezCurveControlPointData>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = tickX;
  history->AddCommand(cmdSet);

  cmdSet.m_sProperty = "Value";
  cmdSet.m_NewValue = clickPosY;
  history->AddCommand(cmdSet);

  cmdSet.m_sProperty = "LeftTangent";
  cmdSet.m_NewValue = ezVec2(-0.1f, 0.0f);
  history->AddCommand(cmdSet);

  cmdSet.m_sProperty = "RightTangent";
  cmdSet.m_NewValue = ezVec2(+0.1f, 0.0f);
  history->AddCommand(cmdSet);

  history->FinishTransaction();

  UpdateCurveEditor();
}

void ezQtPropertyAnimAssetDocumentWindow::onCurveCpMoved(ezUInt32 uiCurveIdx, ezUInt32 cpIdx, ezInt64 iTickX, double newPosY)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  iTickX = ezMath::Max<ezInt64>(iTickX, 0);

  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

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
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet);

  cmdSet.m_sProperty = "Value";
  cmdSet.m_NewValue = newPosY;
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet);
}

void ezQtPropertyAnimAssetDocumentWindow::onCurveCpDeleted(ezUInt32 uiCurveIdx, ezUInt32 cpIdx)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

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
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet);
}

void ezQtPropertyAnimAssetDocumentWindow::onCurveTangentMoved(ezUInt32 uiCurveIdx, ezUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

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

  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

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

  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

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


void ezQtPropertyAnimAssetDocumentWindow::onGradientColorCpAdded(float posX, const ezColorGammaUB& color)
{
  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

  if (m_iMapGradientToTrack < 0)
    return;

  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const ezDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<ezUuid>());
  const ezUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<ezUuid>();

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Color Control Point");

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = gradientGuid;
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_sParentProperty = "ColorCPs";
  cmdAdd.m_pType = ezGetStaticRTTI<ezColorControlPoint>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Position";
  cmdSet.m_NewValue = posX;
  history->AddCommand(cmdSet);

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


void ezQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpAdded(float posX, ezUInt8 alpha)
{
  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

  if (m_iMapGradientToTrack < 0)
    return;

  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const ezDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<ezUuid>());
  const ezUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<ezUuid>();

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Alpha Control Point");

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = gradientGuid;
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_sParentProperty = "AlphaCPs";
  cmdAdd.m_pType = ezGetStaticRTTI<ezAlphaControlPoint>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Position";
  cmdSet.m_NewValue = posX;
  history->AddCommand(cmdSet);

  cmdSet.m_sProperty = "Alpha";
  cmdSet.m_NewValue = alpha;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}


void ezQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpAdded(float posX, float intensity)
{
  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

  if (m_iMapGradientToTrack < 0)
    return;

  const ezVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const ezDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<ezUuid>());
  const ezUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<ezUuid>();

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Intensity Control Point");

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = gradientGuid;
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_sParentProperty = "IntensityCPs";
  cmdAdd.m_pType = ezGetStaticRTTI<ezIntensityControlPoint>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Position";
  cmdSet.m_NewValue = posX;
  history->AddCommand(cmdSet);

  cmdSet.m_sProperty = "Intensity";
  cmdSet.m_NewValue = intensity;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}

void ezQtPropertyAnimAssetDocumentWindow::MoveGradientCP(ezInt32 idx, float newPosX, const char* szArrayName)
{
  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

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

  cmdSet.m_sProperty = "Position";
  cmdSet.m_NewValue = newPosX;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}

void ezQtPropertyAnimAssetDocumentWindow::onGradientColorCpMoved(ezInt32 idx, float newPosX)
{
  MoveGradientCP(idx, newPosX, "ColorCPs");
}

void ezQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpMoved(ezInt32 idx, float newPosX)
{
  MoveGradientCP(idx, newPosX, "AlphaCPs");
}


void ezQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpMoved(ezInt32 idx, float newPosX)
{
  MoveGradientCP(idx, newPosX, "IntensityCPs");
}

void ezQtPropertyAnimAssetDocumentWindow::RemoveGradientCP(ezInt32 idx, const char* szArrayName)
{
  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

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
  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

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
  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

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
  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

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

/*
void ezQtPropertyAnimAssetDocumentWindow::onGradientNormalizeRange()
{
  if (ezQtUiServices::GetSingleton()->MessageBoxQuestion("This will adjust the positions of all control points, such that the minimum is at 0 and the maximum at 1.\n\nContinue?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) != QMessageBox::StandardButton::Yes)
    return;

  ezPropertyAnimAssetDocument* pDoc = static_cast<ezPropertyAnimAssetDocument*>(GetDocument());

  ezColorGradient GradientData;
  pDoc->GetProperties()->FillGradientData(GradientData);

  float minX, maxX;
  if (!GradientData.GetExtents(minX, maxX))
    return;

  if ((minX == 0 && maxX == 1) || (minX >= maxX))
    return;

  ezCommandHistory* history = GetDocument()->GetCommandHistory();

  const float rangeNorm = 1.0f / (maxX - minX);

  history->StartTransaction("Normalize Gradient Range");

  ezUInt32 numRgb, numAlpha, numInt;
  GradientData.GetNumControlPoints(numRgb, numAlpha, numInt);

  for (ezUInt32 i = 0; i < numRgb; ++i)
  {
    float x = GradientData.GetColorControlPoint(i).m_PosX;
    x -= minX;
    x *= rangeNorm;

    MoveGradientCP(i, x, "ColorCPs");
  }

  for (ezUInt32 i = 0; i < numAlpha; ++i)
  {
    float x = GradientData.GetAlphaControlPoint(i).m_PosX;
    x -= minX;
    x *= rangeNorm;

    MoveGradientCP(i, x, "AlphaCPs");
  }

  for (ezUInt32 i = 0; i < numInt; ++i)
  {
    float x = GradientData.GetIntensityControlPoint(i).m_PosX;
    x -= minX;
    x *= rangeNorm;

    MoveGradientCP(i, x, "IntensityCPs");
  }

  history->FinishTransaction();

  m_pGradientEditor->FrameGradient();
}
*/
