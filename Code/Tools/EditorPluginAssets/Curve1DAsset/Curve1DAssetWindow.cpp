#include <EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetWindow.moc.h>
#include <Texture/Image/ImageConversion.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <QLabel>
#include <QLayout>
#include <ToolsFoundation/Command/TreeCommands.h>

ezQtCurve1DAssetDocumentWindow::ezQtCurve1DAssetDocumentWindow(ezDocument* pDocument)
    : ezQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(
      ezMakeDelegate(&ezQtCurve1DAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(
      ezMakeDelegate(&ezQtCurve1DAssetDocumentWindow::StructureEventHandler, this));

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
  pContainer->setLayout(new QVBoxLayout());
  pContainer->layout()->addWidget(m_pCurveEditor);

  setCentralWidget(pContainer);

  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::InsertCpEvent, this, &ezQtCurve1DAssetDocumentWindow::onInsertCpAt);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::CpMovedEvent, this, &ezQtCurve1DAssetDocumentWindow::onCurveCpMoved);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::CpDeletedEvent, this, &ezQtCurve1DAssetDocumentWindow::onCurveCpDeleted);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::TangentMovedEvent, this, &ezQtCurve1DAssetDocumentWindow::onCurveTangentMoved);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::TangentLinkEvent, this, &ezQtCurve1DAssetDocumentWindow::onLinkCurveTangents);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::CpTangentModeEvent, this, &ezQtCurve1DAssetDocumentWindow::onCurveTangentModeChanged);

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
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(
      ezMakeDelegate(&ezQtCurve1DAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(
      ezMakeDelegate(&ezQtCurve1DAssetDocumentWindow::StructureEventHandler, this));
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

void ezQtCurve1DAssetDocumentWindow::onInsertCpAt(ezUInt32 uiCurveIdx, ezInt64 tickX, double clickPosY)
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
    cmdAddCurve.m_pType = ezGetStaticRTTI<ezSingleCurveData>();
    cmdAddCurve.m_Index = -1;

    history->AddCommand(cmdAddCurve);
  }

  const ezVariant curveGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Curves", uiCurveIdx);

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
}

void ezQtCurve1DAssetDocumentWindow::onCurveCpMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, ezInt64 iTickX, double newPosY)
{
  iTickX = ezMath::Max<ezInt64>(iTickX, 0);

  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const ezVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
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

void ezQtCurve1DAssetDocumentWindow::onLinkCurveTangents(ezUInt32 curveIdx, ezUInt32 cpIdx, bool bLink)
{
  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const ezVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const ezDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<ezUuid>());
  const ezVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  ezSetObjectPropertyCommand cmdLink;
  cmdLink.m_Object = cpGuid.Get<ezUuid>();
  cmdLink.m_sProperty = "Linked";
  cmdLink.m_NewValue = bLink;
  GetDocument()->GetCommandHistory()->AddCommand(cmdLink);

  if (bLink)
  {
    const ezVec2 leftTangent = pDoc->GetProperties()->m_Curves[curveIdx]->m_ControlPoints[cpIdx].m_LeftTangent;
    const ezVec2 rightTangent(-leftTangent.x, -leftTangent.y);

    onCurveTangentMoved(curveIdx, cpIdx, rightTangent.x, rightTangent.y, true);
  }
}

void ezQtCurve1DAssetDocumentWindow::onCurveTangentModeChanged(ezUInt32 curveIdx, ezUInt32 cpIdx, bool rightTangent, int mode)
{
  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const ezVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const ezDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<ezUuid>());
  const ezVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  ezSetObjectPropertyCommand cmd;
  cmd.m_Object = cpGuid.Get<ezUuid>();
  cmd.m_sProperty = rightTangent ? "RightTangentMode" : "LeftTangentMode";
  cmd.m_NewValue = mode;
  GetDocument()->GetCommandHistory()->AddCommand(cmd);

  // sync current curve back
  if (false)
  {
    // generally works, but would need some work to make it perfect

    ezCurve1D curve;
    pDoc->GetProperties()->m_Curves[curveIdx]->ConvertToRuntimeData(curve);
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

void ezQtCurve1DAssetDocumentWindow::UpdatePreview()
{
  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  m_pCurveEditor->SetCurves(*pDoc->GetProperties(), 0.1f, false);
}

void ezQtCurve1DAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  UpdatePreview();
}

void ezQtCurve1DAssetDocumentWindow::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  UpdatePreview();
}
