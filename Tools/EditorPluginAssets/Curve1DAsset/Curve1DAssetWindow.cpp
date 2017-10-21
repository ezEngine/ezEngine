#include <PCH.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QLabel>
#include <QLayout>
#include <Foundation/Image/ImageConversion.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>

ezQtCurve1DAssetDocumentWindow::ezQtCurve1DAssetDocumentWindow(ezDocument* pDocument) : ezQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtCurve1DAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezQtCurve1DAssetDocumentWindow::StructureEventHandler, this));

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "Curve1DAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "Curve1DAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("Curve1DAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  m_pCurveEditor = new ezQtCurve1DEditorWidget(this);

  QWidget* pContainer = new QWidget(this);
  pContainer->setLayout(new QVBoxLayout(this));
  pContainer->layout()->addWidget(m_pCurveEditor);

  setCentralWidget(pContainer);

  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::InsertCpEvent, this, &ezQtCurve1DAssetDocumentWindow::onInsertCpAt);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::CpMovedEvent, this, &ezQtCurve1DAssetDocumentWindow::onCurveCpMoved);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::CpDeletedEvent, this, &ezQtCurve1DAssetDocumentWindow::onCurveCpDeleted);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::TangentMovedEvent, this, &ezQtCurve1DAssetDocumentWindow::onCurveTangentMoved);

  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::BeginOperationEvent, this, &ezQtCurve1DAssetDocumentWindow::onCurveBeginOperation);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::EndOperationEvent, this, &ezQtCurve1DAssetDocumentWindow::onCurveEndOperation);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::BeginCpChangesEvent, this, &ezQtCurve1DAssetDocumentWindow::onCurveBeginCpChanges);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::EndCpChangesEvent, this, &ezQtCurve1DAssetDocumentWindow::onCurveEndCpChanges);

  if (false)
  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this);
    pPropertyPanel->setObjectName("Curve1DAssetDockWidget");
    pPropertyPanel->setWindowTitle("Curve1D Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();

  UpdatePreview();
}

ezQtCurve1DAssetDocumentWindow::~ezQtCurve1DAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtCurve1DAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezQtCurve1DAssetDocumentWindow::StructureEventHandler, this));
}


void ezQtCurve1DAssetDocumentWindow::onCurveBeginOperation(QString name)
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands(name.toUtf8().data());
}

void ezQtCurve1DAssetDocumentWindow::onCurveEndOperation(bool commit)
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();

  UpdatePreview();
}

void ezQtCurve1DAssetDocumentWindow::onCurveBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void ezQtCurve1DAssetDocumentWindow::onCurveEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdatePreview();
}

void ezQtCurve1DAssetDocumentWindow::onInsertCpAt(ezUInt32 uiCurveIdx, float clickPosX, float clickPosY)
{
  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  ezCommandHistory* history = pDoc->GetCommandHistory();
  history->StartTransaction("Insert Control Point");

  if (pDoc->GetPropertyObject()->GetTypeAccessor().GetCount("Curves") == 0)
  {
    // no curves allocated yet, add one

    ezAddObjectCommand cmdAddCurve;
    cmdAddCurve.m_Parent = pDoc->GetPropertyObject()->GetGuid();
    cmdAddCurve.m_NewObjectGuid.CreateNewUuid();
    cmdAddCurve.m_sParentProperty = "Curves";
    cmdAddCurve.m_pType = ezGetStaticRTTI<ezCurve1DData>();
    cmdAddCurve.m_Index = -1;

    history->AddCommand(cmdAddCurve);
  }

  const ezVariant curveGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Curves", uiCurveIdx);

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = curveGuid.Get<ezUuid>();
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_sParentProperty = "ControlPoints";
  cmdAdd.m_pType = ezGetStaticRTTI<ezCurve1DControlPoint>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Point";
  cmdSet.m_NewValue = ezVec2(clickPosX, clickPosY);
  history->AddCommand(cmdSet);

  cmdSet.m_sProperty = "LeftTangent";
  cmdSet.m_NewValue = ezVec2(-0.1f, 0.0f);
  history->AddCommand(cmdSet);

  cmdSet.m_sProperty = "RightTangent";
  cmdSet.m_NewValue = ezVec2(+0.1f, 0.0f);
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}

void ezQtCurve1DAssetDocumentWindow::onCurveCpMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY)
{
  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const ezVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const ezDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<ezUuid>());
  const ezVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<ezUuid>();

  cmdSet.m_sProperty = "Point";
  cmdSet.m_NewValue = ezVec2(newPosX, newPosY);
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet);
}

void ezQtCurve1DAssetDocumentWindow::onCurveCpDeleted(ezUInt32 curveIdx, ezUInt32 cpIdx)
{
  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const ezVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const ezDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<ezUuid>());
  const ezVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  ezRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<ezUuid>();
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet);
}

void ezQtCurve1DAssetDocumentWindow::onCurveTangentMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent)
{
  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

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
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet);
}

void ezQtCurve1DAssetDocumentWindow::UpdatePreview()
{
  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  m_pCurveEditor->SetCurves(*pDoc->GetProperties());
}

void ezQtCurve1DAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  UpdatePreview();
}

void ezQtCurve1DAssetDocumentWindow::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  UpdatePreview();
}
