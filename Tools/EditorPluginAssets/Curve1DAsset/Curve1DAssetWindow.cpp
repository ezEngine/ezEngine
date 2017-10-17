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
  pContainer->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
  pContainer->layout()->addWidget(m_pCurveEditor);
  pContainer->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

  setCentralWidget(pContainer);

  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::InsertCpAt, this, &ezQtCurve1DAssetDocumentWindow::onInsertCpAt);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::CpMoved, this, &ezQtCurve1DAssetDocumentWindow::onCurveCpMoved);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::CpDeleted, this, &ezQtCurve1DAssetDocumentWindow::onCurveCpDeleted);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::TangentMoved, this, &ezQtCurve1DAssetDocumentWindow::onCurveTangentMoved);

  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::NormalizeRangeX, this, &ezQtCurve1DAssetDocumentWindow::onCurveNormalizeX);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::NormalizeRangeY, this, &ezQtCurve1DAssetDocumentWindow::onCurveNormalizeY);

  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::BeginOperation, this, &ezQtCurve1DAssetDocumentWindow::onCurveBeginOperation);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::EndOperation, this, &ezQtCurve1DAssetDocumentWindow::onCurveEndOperation);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::BeginCpChanges, this, &ezQtCurve1DAssetDocumentWindow::onCurveBeginCpChanges);
  connect(m_pCurveEditor, &ezQtCurve1DEditorWidget::EndCpChanges, this, &ezQtCurve1DAssetDocumentWindow::onCurveEndCpChanges);

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


//void ezQtCurve1DAssetDocumentWindow::onCurveColorCpAdded(float posX, const ezColorGammaUB& color)
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

//void ezQtCurve1DAssetDocumentWindow::onCurveColorCpChanged(ezInt32 idx, const ezColorGammaUB& color)
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


void ezQtCurve1DAssetDocumentWindow::onCurveBeginOperation()
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Curve");
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


void ezQtCurve1DAssetDocumentWindow::onCurveBeginCpChanges()
{
  GetDocument()->GetCommandHistory()->StartTransaction("Modify Curve");
}

void ezQtCurve1DAssetDocumentWindow::onCurveEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdatePreview();
}


void ezQtCurve1DAssetDocumentWindow::onInsertCpAt(float clickPosX, float clickPosY, float epsilon)
{
  int curveIdx = 0, cpIdx = 0;

  // do not insert at a point where a CP already exists
  if (PickControlPointAt(clickPosX, clickPosY, epsilon, curveIdx, cpIdx))
    return;

  if (!PickCurveAt(clickPosX, clickPosY, epsilon, curveIdx, clickPosY))
    return;

  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  ezCommandHistory* history = pDoc->GetCommandHistory();
  history->StartTransaction("Insert Control Point");

  const ezVariant curveGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Curves", curveIdx);

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


void ezQtCurve1DAssetDocumentWindow::onCurveNormalizeY()
{
  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  /// \todo Active curve index
  ezUInt32 uiActiveCurve = 0;

  ezCurve1D CurveData;
  pDoc->FillCurve(uiActiveCurve, CurveData);

  const ezUInt32 numCPs = CurveData.GetNumControlPoints();

  if (numCPs < 2)
    return;

  ezCurve1D CurveData2;
  pDoc->FillCurve(uiActiveCurve, CurveData2);
  CurveData2.SortControlPoints();
  CurveData2.CreateLinearApproximation();

  float minY, maxY;
  CurveData2.QueryExtremeValues(minY, maxY);

  if (minY == 0 && maxY == 1)
    return;

  ezCommandHistory* history = GetDocument()->GetCommandHistory();

  const float rangeNorm = 1.0f / (maxY - minY);

  history->StartTransaction("Normalize Curve (Y)");

  for (ezUInt32 i = 0; i < numCPs; ++i)
  {
    const auto& cp = CurveData.GetControlPoint(i);

    ezVec2 pos = cp.m_Position;
    pos.y -= minY;
    pos.y *= rangeNorm;

    onCurveCpMoved(uiActiveCurve, i, pos.x, pos.y);

    ezVec2 lt = cp.m_LeftTangent;
    lt.y *= rangeNorm;
    onCurveTangentMoved(uiActiveCurve, i, lt.x, lt.y, false);

    ezVec2 rt = cp.m_RightTangent;
    rt.y *= rangeNorm;
    onCurveTangentMoved(uiActiveCurve, i, rt.x, rt.y, true);
  }

  history->FinishTransaction();

  m_pCurveEditor->FrameCurve();
}

void ezQtCurve1DAssetDocumentWindow::onCurveNormalizeX()
{
  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  /// \todo Active curve index
  ezUInt32 uiActiveCurve = 0;

  ezCurve1D CurveData;
  pDoc->FillCurve(uiActiveCurve, CurveData);

  const ezUInt32 numCPs = CurveData.GetNumControlPoints();

  if (numCPs < 2)
    return;

  CurveData.RecomputeExtents();

  float minX, maxX;
  CurveData.QueryExtents(minX, maxX);

  if (minX == 0 && maxX == 1)
    return;

  ezCommandHistory* history = GetDocument()->GetCommandHistory();

  const float rangeNorm = 1.0f / (maxX - minX);

  history->StartTransaction("Normalize Curve (X)");

  for (ezUInt32 i = 0; i < numCPs; ++i)
  {
    const auto& cp = CurveData.GetControlPoint(i);

    ezVec2 pos = cp.m_Position;
    pos.x -= minX;
    pos.x *= rangeNorm;

    onCurveCpMoved(uiActiveCurve, i, pos.x, pos.y);

    ezVec2 lt = cp.m_LeftTangent;
    lt.x *= rangeNorm;
    onCurveTangentMoved(uiActiveCurve, i, lt.x, lt.y, false);

    ezVec2 rt = cp.m_RightTangent;
    rt.x *= rangeNorm;
    onCurveTangentMoved(uiActiveCurve, i, rt.x, rt.y, true);
  }

  history->FinishTransaction();

  m_pCurveEditor->FrameCurve();
}

void ezQtCurve1DAssetDocumentWindow::UpdatePreview()
{
  ezHybridArray<ezCurve1D, 8> CurveData;

  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  for (ezUInt32 i = 0; i < pDoc->GetCurveCount(); ++i)
  {
    auto& data = CurveData.ExpandAndGetRef();

    pDoc->FillCurve(i, data);
  }

  m_pCurveEditor->SetCurves(CurveData);
}

void ezQtCurve1DAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  UpdatePreview();
}

void ezQtCurve1DAssetDocumentWindow::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  UpdatePreview();
}

bool ezQtCurve1DAssetDocumentWindow::PickCurveAt(float x, float y, float fMaxYDistance, ezInt32& out_iCurveIdx, float& out_ValueY) const
{
  out_iCurveIdx = -1;

  ezCurve1D CurveData;
  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  for (ezUInt32 i = 0; i < pDoc->GetCurveCount(); ++i)
  {
    pDoc->FillCurve(i, CurveData);
    CurveData.SortControlPoints();
    CurveData.CreateLinearApproximation();

    float minVal, maxVal;
    CurveData.QueryExtents(minVal, maxVal);

    if (x < minVal || x > maxVal)
      continue;

    const float val = CurveData.Evaluate(x);

    const float dist = ezMath::Abs(val - y);
    if (dist < fMaxYDistance)
    {
      fMaxYDistance = dist;
      out_iCurveIdx = i;
      out_ValueY = val;
    }
  }

  return out_iCurveIdx >= 0;
}

bool ezQtCurve1DAssetDocumentWindow::PickControlPointAt(float x, float y, float fMaxDistance, ezInt32& out_iCurveIdx, ezInt32& out_iCpIdx) const
{
  const ezVec2 at(x, y);
  float fMaxDistSqr = ezMath::Square(fMaxDistance);

  out_iCurveIdx = -1;
  out_iCpIdx = -1;

  ezCurve1D CurveData;
  ezCurve1DAssetDocument* pDoc = static_cast<ezCurve1DAssetDocument*>(GetDocument());

  for (ezUInt32 iCurve = 0; iCurve < pDoc->GetCurveCount(); ++iCurve)
  {
     pDoc->FillCurve(iCurve, CurveData);

     for (ezUInt32 iCP = 0; iCP < CurveData.GetNumControlPoints(); ++iCP)
     {
       const auto& cp = CurveData.GetControlPoint(iCP);
       const float fDistSqr = (cp.m_Position - at).GetLengthSquared();
       if (fDistSqr <= fMaxDistSqr)
       {
         fMaxDistSqr = fDistSqr;
         out_iCurveIdx = iCurve;
         out_iCpIdx = iCP;
       }
     }
  }

  return out_iCpIdx >= 0;
}


