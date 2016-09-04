#include <PCH.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QLabel>
#include <QLayout>
#include <CoreUtils/Image/ImageConversion.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>

ezCurve1DAssetDocumentWindow::ezCurve1DAssetDocumentWindow(ezDocument* pDocument) : ezQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezCurve1DAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezCurve1DAssetDocumentWindow::StructureEventHandler, this));

  // Menu Bar
  {
    ezMenuBarActionMapView* pMenuBar = static_cast<ezMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "Curve1DAssetMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "Curve1DAssetToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("Curve1DAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  m_pCurveEditor = new QCurve1DEditorWidget(this);

  QWidget* pContainer = new QWidget(this);
  pContainer->setLayout(new QVBoxLayout(this));
  pContainer->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
  pContainer->layout()->addWidget(m_pCurveEditor);
  pContainer->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

  setCentralWidget(pContainer);

  connect(m_pCurveEditor, &QCurve1DEditorWidget::CpMoved, this, &ezCurve1DAssetDocumentWindow::onCurveCpMoved);
  connect(m_pCurveEditor, &QCurve1DEditorWidget::CpDeleted, this, &ezCurve1DAssetDocumentWindow::onCurveCpDeleted);
  connect(m_pCurveEditor, &QCurve1DEditorWidget::TangentMoved, this, &ezCurve1DAssetDocumentWindow::onCurveTangentMoved);

  connect(m_pCurveEditor, &QCurve1DEditorWidget::BeginOperation, this, &ezCurve1DAssetDocumentWindow::onCurveBeginOperation);
  connect(m_pCurveEditor, &QCurve1DEditorWidget::EndOperation, this, &ezCurve1DAssetDocumentWindow::onCurveEndOperation);
  connect(m_pCurveEditor, &QCurve1DEditorWidget::BeginCpChanges, this, &ezCurve1DAssetDocumentWindow::onCurveBeginCpChanges);
  connect(m_pCurveEditor, &QCurve1DEditorWidget::EndCpChanges, this, &ezCurve1DAssetDocumentWindow::onCurveEndCpChanges);

  {
    ezDocumentPanel* pPropertyPanel = new ezDocumentPanel(this);
    pPropertyPanel->setObjectName("Curve1DAssetDockWidget");
    pPropertyPanel->setWindowTitle("Curve1D Properties");
    pPropertyPanel->show();

    ezPropertyGridWidget* pPropertyGrid = new ezPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();

  UpdatePreview();
}

ezCurve1DAssetDocumentWindow::~ezCurve1DAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezCurve1DAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezCurve1DAssetDocumentWindow::StructureEventHandler, this));
}


//void ezCurve1DAssetDocumentWindow::onCurveColorCpAdded(float posX, const ezColorGammaUB& color)
//{
//  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());
//
//  ezCommandHistory* history = GetDocument()->GetCommandHistory();
//  history->StartTransaction();
//
//  ezAddObjectCommand cmdAdd;
//  cmdAdd.m_Parent = pDoc->GetPropertyObject()->GetGuid();
//  cmdAdd.m_NewObjectGuid.CreateNewUuid();
//  cmdAdd.m_sParentProperty = "Color CPs";
//  cmdAdd.m_pType = ezGetStaticRTTI<ezColorControlPoint>();
//  cmdAdd.m_Index = -1;
//
//  history->AddCommand(cmdAdd);
//
//  ezSetObjectPropertyCommand cmdSet;
//  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;
//
//  cmdSet.m_sPropertyPath = "Position";
//  cmdSet.m_NewValue = posX;
//  history->AddCommand(cmdSet);
//
//  cmdSet.m_sPropertyPath = "Red";
//  cmdSet.m_NewValue = color.r;
//  history->AddCommand(cmdSet);
//
//  cmdSet.m_sPropertyPath = "Green";
//  cmdSet.m_NewValue = color.g;
//  history->AddCommand(cmdSet);
//
//  cmdSet.m_sPropertyPath = "Blue";
//  cmdSet.m_NewValue = color.b;
//  history->AddCommand(cmdSet);
//
//  history->FinishTransaction();
//}

//void ezCurve1DAssetDocumentWindow::onCurveColorCpChanged(ezInt32 idx, const ezColorGammaUB& color)
//{
//  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());
//
//  auto pProp = pDoc->GetPropertyObject();
//  ezVariant objGuid = pProp->GetTypeAccessor().GetValue("Color CPs", idx);
//
//  ezCommandHistory* history = GetDocument()->GetCommandHistory();
//  history->StartTransaction();
//
//  ezSetObjectPropertyCommand cmdSet;
//  cmdSet.m_Object = objGuid.Get<ezUuid>();
//
//  cmdSet.m_sPropertyPath = "Red";
//  cmdSet.m_NewValue = color.r;
//  history->AddCommand(cmdSet);
//
//  cmdSet.m_sPropertyPath = "Green";
//  cmdSet.m_NewValue = color.g;
//  history->AddCommand(cmdSet);
//
//  cmdSet.m_sPropertyPath = "Blue";
//  cmdSet.m_NewValue = color.b;
//  history->AddCommand(cmdSet);
//
//  history->FinishTransaction();
//}


void ezCurve1DAssetDocumentWindow::onCurveBeginOperation()
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands();
}


void ezCurve1DAssetDocumentWindow::onCurveEndOperation(bool commit)
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();

  UpdatePreview();
}


void ezCurve1DAssetDocumentWindow::onCurveBeginCpChanges()
{
  GetDocument()->GetCommandHistory()->StartTransaction();
}

void ezCurve1DAssetDocumentWindow::onCurveEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdatePreview();
}

void ezCurve1DAssetDocumentWindow::onCurveCpMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY)
{
  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const ezVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const ezDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<ezUuid>());
  const ezVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("Control Points", cpIdx);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<ezUuid>();

  cmdSet.m_sPropertyPath = "Point";
  cmdSet.m_NewValue = ezVec2(newPosX, newPosY);
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet);
}


void ezCurve1DAssetDocumentWindow::onCurveCpDeleted(ezUInt32 curveIdx, ezUInt32 cpIdx)
{
  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const ezVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const ezDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<ezUuid>());
  const ezVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("Control Points", cpIdx);

  ezRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<ezUuid>();
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet);
}


void ezCurve1DAssetDocumentWindow::onCurveTangentMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent)
{
  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const ezVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const ezDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<ezUuid>());
  const ezVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("Control Points", cpIdx);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<ezUuid>();

  cmdSet.m_sPropertyPath = rightTangent ? "Right Tangent" : "Left Tangent";
  cmdSet.m_NewValue = ezVec2(newPosX, newPosY);
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet);
}

//
//void ezCurve1DAssetDocumentWindow::onCurveNormalizeRange()
//{
//  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());
//
//  ezCurve1D CurveData;
//  pDoc->FillCurveData(CurveData);
//
//  float minX, maxX;
//  if (!CurveData.GetExtents(minX, maxX))
//    return;
//
//  if (minX == 0 && maxX == 1)
//    return;
//
//  ezCommandHistory* history = GetDocument()->GetCommandHistory();
//
//  const float rangeNorm = 1.0f / (maxX - minX);
//
//  history->StartTransaction();
//
//  ezUInt32 numRgb, numAlpha, numInt;
//  CurveData.GetNumControlPoints(numRgb, numAlpha, numInt);
//
//  for (ezUInt32 i = 0; i < numRgb; ++i)
//  {
//    float x = CurveData.GetColorControlPoint(i).m_PosX;
//    x -= minX;
//    x *= rangeNorm;
//
//    MoveCP(i, x, "Color CPs");
//  }
//
//  history->FinishTransaction();
//
//  m_pCurveEditor->FrameCurve();
//}

void ezCurve1DAssetDocumentWindow::UpdatePreview()
{
  ezCurve1D CurveData;

  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  m_pCurveEditor->SetNumCurves(pDoc->GetCurveCount());

  for (ezUInt32 i = 0; i < pDoc->GetCurveCount(); ++i)
  {
    pDoc->FillCurve(i, CurveData);
    m_pCurveEditor->SetCurve1D(i, CurveData);
  }
}

void ezCurve1DAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  UpdatePreview();
}

void ezCurve1DAssetDocumentWindow::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  UpdatePreview();
}



