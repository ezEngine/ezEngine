#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Math.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <QGraphicsItem>
#include <QGraphicsSceneEvent>
#include <QMenu>
#include <QPainterPath>

ezQtCurve1DEditorWidget::ezQtCurve1DEditorWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setupUi(this);

  CurveEdit->SetGridBarWidget(GridBarWidget);

  connect(CurveEdit, &ezQtCurveEditWidget::DeleteControlPointsEvent, this, &ezQtCurve1DEditorWidget::onDeleteControlPoints);
  connect(CurveEdit, &ezQtCurveEditWidget::DoubleClickEvent, this, &ezQtCurve1DEditorWidget::onDoubleClick);
  connect(CurveEdit, &ezQtCurveEditWidget::MoveControlPointsEvent, this, &ezQtCurve1DEditorWidget::onMoveControlPoints);
  connect(CurveEdit, &ezQtCurveEditWidget::MoveTangentsEvent, this, &ezQtCurve1DEditorWidget::onMoveTangents);
  connect(CurveEdit, &ezQtCurveEditWidget::BeginOperationEvent, this, &ezQtCurve1DEditorWidget::onBeginOperation);
  connect(CurveEdit, &ezQtCurveEditWidget::EndOperationEvent, this, &ezQtCurve1DEditorWidget::onEndOperation);
  connect(CurveEdit, &ezQtCurveEditWidget::ScaleControlPointsEvent, this, &ezQtCurve1DEditorWidget::onScaleControlPoints);
  connect(CurveEdit, &ezQtCurveEditWidget::ContextMenuEvent, this, &ezQtCurve1DEditorWidget::onContextMenu);
  connect(CurveEdit, &ezQtCurveEditWidget::SelectionChangedEvent, this, &ezQtCurve1DEditorWidget::onSelectionChanged);
  connect(CurveEdit, &ezQtCurveEditWidget::MoveCurveEvent, this, &ezQtCurve1DEditorWidget::onMoveCurve);

  LinePosition->setEnabled(false);
  LineValue->setEnabled(false);
}

ezQtCurve1DEditorWidget::~ezQtCurve1DEditorWidget() {}

void ezQtCurve1DEditorWidget::SetCurveExtents(double fLowerBound, double fUpperBound, bool bLowerIsFixed, bool bUpperIsFixed)
{
  CurveEdit->m_fLowerExtent = fLowerBound;
  CurveEdit->m_fUpperExtent = fUpperBound;
  CurveEdit->m_bLowerExtentFixed = bLowerIsFixed;
  CurveEdit->m_bUpperExtentFixed = bUpperIsFixed;
}

void ezQtCurve1DEditorWidget::SetCurveRanges(double fLowerRange, double fUpperRange)
{
  CurveEdit->m_fLowerRange = fLowerRange;
  CurveEdit->m_fUpperRange = fUpperRange;
}

void ezQtCurve1DEditorWidget::SetCurves(const ezCurveGroupData& curves)
{
  ezQtScopedUpdatesDisabled ud(this);
  ezQtScopedBlockSignals bs(this);

  m_Curves.CloneFrom(curves);

  CurveEdit->SetCurves(&m_Curves);
  m_fCurveDuration = CurveEdit->GetMaxCurveExtent();

  UpdateSpinBoxes();
}


void ezQtCurve1DEditorWidget::SetScrubberPosition(ezUInt64 uiTick)
{
  CurveEdit->SetScrubberPosition(uiTick / 4800.0);
}


void ezQtCurve1DEditorWidget::ClearSelection()
{
  CurveEdit->ClearSelection();
}

void ezQtCurve1DEditorWidget::FrameCurve()
{
  CurveEdit->FrameCurve();
}

void ezQtCurve1DEditorWidget::FrameSelection()
{
  CurveEdit->FrameSelection();
}

void ezQtCurve1DEditorWidget::MakeRepeatable(bool bAdjustLastPoint)
{
  Q_EMIT BeginOperationEvent("Make Curve Repeatable");

  for (ezUInt32 iCurveIdx = 0; iCurveIdx < m_Curves.m_Curves.GetCount(); ++iCurveIdx)
  {
    const auto& curve = *m_Curves.m_Curves[iCurveIdx];

    const ezUInt32 uiNumCps = curve.m_ControlPoints.GetCount();
    if (uiNumCps < 2)
      continue;

    ezInt64 iMinTick = curve.m_ControlPoints[0].m_iTick;
    ezInt64 iMaxTick = curve.m_ControlPoints[0].m_iTick;
    ezUInt32 uiMinCp = 0;
    ezUInt32 uiMaxCp = 0;

    for (ezUInt32 uiCpIdx = 1; uiCpIdx < uiNumCps; ++uiCpIdx)
    {
      const ezInt64 x = curve.m_ControlPoints[uiCpIdx].m_iTick;

      if (x < iMinTick)
      {
        iMinTick = x;
        uiMinCp = uiCpIdx;
      }
      if (x > iMaxTick)
      {
        iMaxTick = x;
        uiMaxCp = uiCpIdx;
      }
    }

    if (uiMinCp == uiMaxCp)
      continue;

    // copy data, the first Q_EMIT may change the backing store
    const ezCurveControlPointData cpLeft = curve.m_ControlPoints[uiMinCp];
    const ezCurveControlPointData cpRight = curve.m_ControlPoints[uiMaxCp];

    if (bAdjustLastPoint)
    {
      Q_EMIT CpMovedEvent(iCurveIdx, uiMaxCp, (ezInt64)(m_fCurveDuration * 4800.0), cpLeft.m_fValue);
      Q_EMIT TangentMovedEvent(iCurveIdx, uiMaxCp, -cpLeft.m_RightTangent.x, -cpLeft.m_RightTangent.y, false);
    }
    else
    {
      Q_EMIT CpMovedEvent(iCurveIdx, uiMinCp, 0, cpRight.m_fValue);
      Q_EMIT TangentMovedEvent(iCurveIdx, uiMinCp, -cpRight.m_LeftTangent.x, -cpRight.m_LeftTangent.y, true);
    }
  }

  Q_EMIT EndOperationEvent(true);
}

void ezQtCurve1DEditorWidget::NormalizeCurveX(ezUInt32 uiActiveCurve)
{
  if (uiActiveCurve > m_Curves.m_Curves.GetCount())
    return;

  ezCurve1D CurveData;
  m_Curves.ConvertToRuntimeData(uiActiveCurve, CurveData);

  const ezUInt32 numCPs = CurveData.GetNumControlPoints();

  if (numCPs < 2)
    return;

  CurveData.RecomputeExtents();

  double minX, maxX;
  CurveData.QueryExtents(minX, maxX);

  if (minX == 0 && maxX == 1)
    return;

  Q_EMIT BeginOperationEvent("Normalize Curve (X)");

  const float rangeNorm = 1.0f / (maxX - minX);

  for (ezUInt32 i = 0; i < numCPs; ++i)
  {
    const auto& cp = CurveData.GetControlPoint(i);

    ezVec2d pos = cp.m_Position;
    pos.x -= minX;
    pos.x *= rangeNorm;

    Q_EMIT CpMovedEvent(uiActiveCurve, i, m_Curves.TickFromTime(ezTime::Seconds(pos.x)), pos.y);

    ezVec2 lt = cp.m_LeftTangent;
    lt.x *= rangeNorm;
    Q_EMIT TangentMovedEvent(uiActiveCurve, i, lt.x, lt.y, false);

    ezVec2 rt = cp.m_RightTangent;
    rt.x *= rangeNorm;
    Q_EMIT TangentMovedEvent(uiActiveCurve, i, rt.x, rt.y, true);
  }

  Q_EMIT EndOperationEvent(true);

  FrameCurve();
}

void ezQtCurve1DEditorWidget::NormalizeCurveY(ezUInt32 uiActiveCurve)
{
  if (uiActiveCurve > m_Curves.m_Curves.GetCount())
    return;

  ezCurve1D CurveData;
  m_Curves.ConvertToRuntimeData(uiActiveCurve, CurveData);

  const ezUInt32 numCPs = CurveData.GetNumControlPoints();

  if (numCPs < 2)
    return;

  ezCurve1D CurveDataSorted = CurveData;
  CurveDataSorted.SortControlPoints();
  CurveDataSorted.CreateLinearApproximation();

  double minY, maxY;
  CurveDataSorted.QueryExtremeValues(minY, maxY);

  if (minY == 0 && maxY == 1)
    return;

  Q_EMIT BeginOperationEvent("Normalize Curve (Y)");

  const float rangeNorm = 1.0f / (maxY - minY);

  for (ezUInt32 i = 0; i < numCPs; ++i)
  {
    const auto& cp = CurveData.GetControlPoint(i);

    ezVec2d pos = cp.m_Position;
    pos.y -= minY;
    pos.y *= rangeNorm;

    Q_EMIT CpMovedEvent(uiActiveCurve, i, m_Curves.TickFromTime(ezTime::Seconds(pos.x)), pos.y);

    ezVec2 lt = cp.m_LeftTangent;
    lt.y *= rangeNorm;
    Q_EMIT TangentMovedEvent(uiActiveCurve, i, lt.x, lt.y, false);

    ezVec2 rt = cp.m_RightTangent;
    rt.y *= rangeNorm;
    Q_EMIT TangentMovedEvent(uiActiveCurve, i, rt.x, rt.y, true);
  }

  Q_EMIT EndOperationEvent(true);

  FrameCurve();
}

struct PtToDelete
{
  EZ_DECLARE_POD_TYPE();

  ezUInt32 m_uiCurveIdx;
  ezUInt32 m_uiPointIdx;

  bool operator<(const PtToDelete& rhs) const { return m_uiPointIdx > rhs.m_uiPointIdx; }
};

void ezQtCurve1DEditorWidget::ClearAllPoints()
{
  Q_EMIT BeginCpChangesEvent("Delete Points");

  ezHybridArray<PtToDelete, 16> delOrder;

  for (ezUInt32 curveIdx = 0; curveIdx < m_Curves.m_Curves.GetCount(); ++curveIdx)
  {
    ezCurve1D curveData;
    m_Curves.m_Curves[curveIdx]->ConvertToRuntimeData(curveData);

    for (ezUInt32 i = 0; i < curveData.GetNumControlPoints(); ++i)
    {
      auto& pt = delOrder.ExpandAndGetRef();
      pt.m_uiCurveIdx = curveIdx;
      pt.m_uiPointIdx = i;
    }
  }

  delOrder.Sort();

  // Delete sorted from back to front to prevent point indices becoming invalidated
  for (const auto& pt : delOrder)
  {
    Q_EMIT CpDeletedEvent(pt.m_uiCurveIdx, pt.m_uiPointIdx);
  }

  Q_EMIT EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::onDeleteControlPoints()
{
  const auto selection = CurveEdit->GetSelection();

  if (selection.IsEmpty())
    return;

  CurveEdit->ClearSelection();

  Q_EMIT BeginCpChangesEvent("Delete Points");

  ezHybridArray<PtToDelete, 16> delOrder;

  for (const auto& item : selection)
  {
    auto& pt = delOrder.ExpandAndGetRef();
    pt.m_uiCurveIdx = item.m_uiCurve;
    pt.m_uiPointIdx = item.m_uiPoint;
  }

  delOrder.Sort();

  // delete sorted from back to front to prevent point indices becoming invalidated
  for (const auto& pt : delOrder)
  {
    Q_EMIT CpDeletedEvent(pt.m_uiCurveIdx, pt.m_uiPointIdx);
  }

  Q_EMIT EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::onDoubleClick(const QPointF& scenePos, const QPointF& epsilon)
{
  Q_EMIT BeginCpChangesEvent("Add Control Point");

  InsertCpAt(scenePos.x(), scenePos.y(), ezVec2d(ezMath::Abs(epsilon.x()), ezMath::Abs(epsilon.y())));

  Q_EMIT EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::onMoveControlPoints(double x, double y)
{
  m_ControlPointMove += ezVec2d(x, y);

  const auto selection = CurveEdit->GetSelection();

  if (selection.IsEmpty())
    return;

  Q_EMIT BeginCpChangesEvent("Move Points");

  for (const auto& cpSel : selection)
  {
    const auto& cp = m_CurvesBackup.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];
    ezVec2d newPos = ezVec2d(cp.GetTickAsTime().GetSeconds(), cp.m_fValue) + m_ControlPointMove;

    ClampPoint(newPos.x, newPos.y);

    Q_EMIT CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, m_Curves.TickFromTime(ezTime::Seconds(newPos.x)), newPos.y);
  }

  Q_EMIT EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::onScaleControlPoints(QPointF refPt, double scaleX, double scaleY)
{
  const auto selection = CurveEdit->GetSelection();

  if (selection.IsEmpty())
    return;

  const ezVec2d ref(refPt.x(), refPt.y());
  const ezVec2d scale(scaleX, scaleY);

  Q_EMIT BeginCpChangesEvent("Scale Points");

  for (const auto& cpSel : selection)
  {
    const auto& cp = m_CurvesBackup.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];
    ezVec2d newPos = ref + (ezVec2d(cp.GetTickAsTime().GetSeconds(), cp.m_fValue) - ref).CompMul(scale);

    ClampPoint(newPos.x, newPos.y);

    Q_EMIT CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, m_Curves.TickFromTime(ezTime::Seconds(newPos.x)), newPos.y);
  }

  Q_EMIT EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::onMoveTangents(float x, float y)
{
  m_TangentMove += ezVec2(x, y);

  ezInt32 iCurve;
  ezInt32 iPoint;
  bool bLeftTangent;

  if (!CurveEdit->GetSelectedTangent(iCurve, iPoint, bLeftTangent))
    return;

  Q_EMIT BeginCpChangesEvent("Move Tangents");

  {
    const auto& cp = m_CurvesBackup.m_Curves[iCurve]->m_ControlPoints[iPoint];
    ezVec2 newPos;

    if (bLeftTangent)
      newPos = cp.m_LeftTangent + m_TangentMove;
    else
      newPos = cp.m_RightTangent + m_TangentMove;

    newPos.y = ezMath::Clamp(newPos.y, -100000.0f, +100000.0f);

    Q_EMIT TangentMovedEvent(iCurve, iPoint, newPos.x, newPos.y, !bLeftTangent);

    if (cp.m_bTangentsLinked)
    {
      Q_EMIT TangentMovedEvent(iCurve, iPoint, -newPos.x, -newPos.y, bLeftTangent);
    }
  }

  Q_EMIT EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::onBeginOperation(QString name)
{
  m_CurvesBackup.CloneFrom(m_Curves);
  m_TangentMove.SetZero();
  m_ControlPointMove.SetZero();

  Q_EMIT BeginOperationEvent(name);
}

void ezQtCurve1DEditorWidget::onEndOperation(bool commit)
{
  Q_EMIT EndOperationEvent(commit);
}

void ezQtCurve1DEditorWidget::onContextMenu(QPoint pos, QPointF scenePos)
{
  if (m_Curves.m_Curves.IsEmpty())
    return;

  m_contextMenuScenePos = scenePos;

  QMenu m(this);
  m.setDefaultAction(m.addAction("Add Point\tDbl Click", this, SLOT(onAddPoint())));

  const auto& selection = CurveEdit->GetSelection();

  QMenu* cmSel = m.addMenu("Selection");
  cmSel->addAction("Select All\tCtrl+A", this, [this]()
    { CurveEdit->SelectAll(); });

  if (!selection.IsEmpty())
  {
    cmSel->addAction("Clear Selection\tESC", this, [this]()
      { CurveEdit->ClearSelection(); });

    cmSel->addAction(
      "Frame Selection\tShift+F", this, [this]()
      { FrameSelection(); });

    cmSel->addSeparator();

    cmSel->addAction("Delete Points\tDel", this, SLOT(onDeleteControlPoints()));
    cmSel->addSeparator();
    cmSel->addAction("Link Tangents", this, SLOT(onLinkTangents()));
    cmSel->addAction("Break Tangents", this, SLOT(onBreakTangents()));
    cmSel->addAction("Flatten Tangents", this, SLOT(onFlattenTangents()));

    QMenu* cmLT = cmSel->addMenu("Left Tangents");
    QMenu* cmRT = cmSel->addMenu("Right Tangents");
    QMenu* cmBT = cmSel->addMenu("Both Tangents");

    cmLT->addAction("Auto", this, [this]()
      { SetTangentMode(ezCurveTangentMode::Auto, true, false); });
    cmLT->addAction("Bezier", this, [this]()
      { SetTangentMode(ezCurveTangentMode::Bezier, true, false); });
    cmLT->addAction("Fixed Length", this, [this]()
      { SetTangentMode(ezCurveTangentMode::FixedLength, true, false); });
    cmLT->addAction("Linear", this, [this]()
      { SetTangentMode(ezCurveTangentMode::Linear, true, false); });

    cmRT->addAction("Auto", this, [this]()
      { SetTangentMode(ezCurveTangentMode::Auto, false, true); });
    cmRT->addAction("Bezier", this, [this]()
      { SetTangentMode(ezCurveTangentMode::Bezier, false, true); });
    cmRT->addAction("Fixed Length", this, [this]()
      { SetTangentMode(ezCurveTangentMode::FixedLength, false, true); });
    cmRT->addAction("Linear", this, [this]()
      { SetTangentMode(ezCurveTangentMode::Linear, false, true); });

    cmBT->addAction("Auto", this, [this]()
      { SetTangentMode(ezCurveTangentMode::Auto, true, true); });
    cmBT->addAction("Bezier", this, [this]()
      { SetTangentMode(ezCurveTangentMode::Bezier, true, true); });
    cmBT->addAction("Fixed Length", this, [this]()
      { SetTangentMode(ezCurveTangentMode::FixedLength, true, true); });
    cmBT->addAction("Linear", this, [this]()
      { SetTangentMode(ezCurveTangentMode::Linear, true, true); });
  }

  QMenu* cm = m.addMenu("Curve");
  cm->addSeparator();
  cm->addAction("Normalize X", this, [this]()
    { NormalizeCurveX(0); });
  cm->addAction("Normalize Y", this, [this]()
    { NormalizeCurveY(0); });
  cm->addAction("Loop: Adjust Last Point", this, [this]()
    { MakeRepeatable(true); });
  cm->addAction("Loop: Adjust First Point", this, [this]()
    { MakeRepeatable(false); });
  cm->addAction("Clear Curve", this, [this]()
    { ClearAllPoints(); });

  cm->addAction(
    "Frame Curve\tCtrl+F", this, [this]()
    { FrameCurve(); });

  QMenu* gm = m.addMenu("Generate Curve");
  QMenu* lm = gm->addMenu("Linear");
  QMenu* sm = gm->addMenu("Sine");
  QMenu* qm = gm->addMenu("Quad");
  QMenu* qcm = gm->addMenu("Cubic");
  QMenu* qrm = gm->addMenu("Quartic");
  QMenu* qim = gm->addMenu("Quintic");
  QMenu* qxm = gm->addMenu("Expo");
  QMenu* qcrm = gm->addMenu("Circ");
  QMenu* qbm = gm->addMenu("Back");
  QMenu* qem = gm->addMenu("Elastic");
  QMenu* qbom = gm->addMenu("Bounce");

  gm->addSeparator();
  lm->addAction("EaseInLinear", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InLinear); });
  lm->addAction("EaseOutLinear", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::OutLinear); });
  sm->addAction("EaseInSine", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InSine); });
  sm->addAction("EaseOutSine", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::OutSine); });
  sm->addAction("EaseInOutSine", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InOutSine); });
  qm->addAction("EaseInQuad", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InQuad); });
  qm->addAction("EaseOutQuad", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::OutQuad); });
  qm->addAction("EaseInOutQuad", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InOutQuad); });
  qcm->addAction("EaseInCubic", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InCubic); });
  qcm->addAction("EaseOutCubic", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::OutCubic); });
  qcm->addAction("EaseInOutCubic", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InOutCubic); });
  qrm->addAction("EaseInQuartic", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InQuartic); });
  qrm->addAction("EaseOutQuartic", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::OutQuartic); });
  qrm->addAction("EaseInOutQuartic", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InOutQuartic); });
  qim->addAction("EaseInQuintic", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InQuintic); });
  qim->addAction("EaseOutQuintic", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::OutQuintic); });
  qim->addAction("EaseInOutQuintic", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InOutQuintic); });
  qxm->addAction("EaseInExpo", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InExpo); });
  qxm->addAction("EaseOutExpo", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::OutExpo); });
  qxm->addAction("EaseInOutExpo", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InOutExpo); });
  qcrm->addAction("EaseInCirc", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InCirc); });
  qcrm->addAction("EaseOutCirc", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::OutCirc); });
  qcrm->addAction("EaseInOutCirc", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InOutCirc); });
  qbm->addAction("EaseInBack", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InBack); });
  qbm->addAction("EaseOutBack", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::OutBack); });
  qbm->addAction("EaseInOutBack", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InOutBack); });
  qem->addAction("EaseInElastic", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InElastic); });
  qem->addAction("EaseOutElastic", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::OutElastic); });
  qem->addAction("EaseInOutElastic", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InOutElastic); });
  qbom->addAction("EaseInBounce", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InBounce); });
  qbom->addAction("EaseOutBounce", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::OutBounce); });
  qbom->addAction("EaseInOutBounce", this, [this]()
    { onGenerateCurve(ezMath::ezEasingFunctions::InOutBounce); });

  m.exec(pos);
}

void ezQtCurve1DEditorWidget::onAddPoint()
{
  Q_EMIT BeginCpChangesEvent("Add Control Point");

  InsertCpAt(m_contextMenuScenePos.x(), m_contextMenuScenePos.y(), ezVec2d::ZeroVector());

  Q_EMIT EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::onLinkTangents()
{
  const auto& selection = CurveEdit->GetSelection();

  Q_EMIT BeginOperationEvent("Link Tangents");

  for (const auto& cpSel : selection)
  {
    Q_EMIT TangentLinkEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, true);
  }

  Q_EMIT EndOperationEvent(true);
}

void ezQtCurve1DEditorWidget::onBreakTangents()
{
  const auto& selection = CurveEdit->GetSelection();

  Q_EMIT BeginOperationEvent("Break Tangents");

  for (const auto& cpSel : selection)
  {
    Q_EMIT TangentLinkEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, false);
  }

  Q_EMIT EndOperationEvent(true);
}


void ezQtCurve1DEditorWidget::onFlattenTangents()
{
  const auto& selection = CurveEdit->GetSelection();

  Q_EMIT BeginOperationEvent("Flatten Tangents");

  for (const auto& cpSel : selection)
  {
    // don't use references, the signals may move the data in memory
    const ezVec2 tL = m_Curves.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint].m_LeftTangent;
    const ezVec2 tR = m_Curves.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint].m_RightTangent;

    // clamp the X position, to prevent tangents with zero length
    Q_EMIT TangentMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, ezMath::Min(tL.x, -0.02f), 0, false);
    Q_EMIT TangentMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, ezMath::Max(tR.x, +0.02f), 0, true);
  }

  Q_EMIT EndOperationEvent(true);
}

void ezQtCurve1DEditorWidget::InsertCpAt(double posX, double value, ezVec2d epsilon)
{
  int curveIdx = 0, cpIdx = 0;

  ClampPoint(posX, value);

  // do not insert at a point where a CP already exists
  if (PickControlPointAt(posX, value, epsilon, curveIdx, cpIdx))
    return;

  if (!PickCurveAt(posX, value, epsilon.y, curveIdx, value))
  {
    // by default insert into curve 0
    curveIdx = 0;
  }

  Q_EMIT InsertCpEvent(curveIdx, m_Curves.TickFromTime(ezTime::Seconds(posX)), value);
}


bool ezQtCurve1DEditorWidget::PickCurveAt(double x, double y, double fMaxDistanceY, ezInt32& out_iCurveIdx, double& out_ValueY) const
{
  out_iCurveIdx = -1;
  ezCurve1D CurveData;

  for (ezUInt32 i = 0; i < m_Curves.m_Curves.GetCount(); ++i)
  {
    m_Curves.ConvertToRuntimeData(i, CurveData);

    CurveData.SortControlPoints();
    CurveData.CreateLinearApproximation();

    double minVal, maxVal;
    CurveData.QueryExtents(minVal, maxVal);

    const double val = CurveData.Evaluate(x);

    const double dist = ezMath::Abs(val - y);
    if (dist < fMaxDistanceY)
    {
      fMaxDistanceY = dist;
      out_iCurveIdx = i;
      out_ValueY = val;
    }
  }

  return out_iCurveIdx >= 0;
}

bool ezQtCurve1DEditorWidget::PickControlPointAt(double x, double y, ezVec2d vMaxDistance, ezInt32& out_iCurveIdx, ezInt32& out_iCpIdx) const
{
  const ezVec2d at(x, y);

  out_iCurveIdx = -1;
  out_iCpIdx = -1;

  ezCurve1D CurveData;

  for (ezUInt32 iCurve = 0; iCurve < m_Curves.m_Curves.GetCount(); ++iCurve)
  {
    m_Curves.ConvertToRuntimeData(iCurve, CurveData);

    for (ezUInt32 iCP = 0; iCP < CurveData.GetNumControlPoints(); ++iCP)
    {
      const auto& cp = CurveData.GetControlPoint(iCP);
      const ezVec2d dist = cp.m_Position - at;

      if (ezMath::Abs(dist.x) <= vMaxDistance.x && ezMath::Abs(dist.y) <= vMaxDistance.y)
      {
        vMaxDistance.x = ezMath::Abs(dist.x);
        vMaxDistance.y = ezMath::Abs(dist.y);

        out_iCurveIdx = iCurve;
        out_iCpIdx = iCP;
      }
    }
  }

  return out_iCpIdx >= 0;
}

void ezQtCurve1DEditorWidget::onSelectionChanged()
{
  UpdateSpinBoxes();
}


void ezQtCurve1DEditorWidget::onMoveCurve(ezInt32 iCurve, double moveY)
{
  m_ControlPointMove.y += moveY;

  Q_EMIT BeginCpChangesEvent("Move Curve");

  const auto& curve = *m_CurvesBackup.m_Curves[iCurve];
  ezUInt32 uiNumCps = curve.m_ControlPoints.GetCount();
  for (ezUInt32 i = 0; i < uiNumCps; ++i)
  {
    const ezInt64 x = curve.m_ControlPoints[i].m_iTick;
    const float y = curve.m_ControlPoints[i].m_fValue + m_ControlPointMove.y;

    Q_EMIT CpMovedEvent(iCurve, i, x, y);
  }

  Q_EMIT EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::onGenerateCurve(ezMath::ezEasingFunctions easingFunction)
{
  Q_EMIT BeginCpChangesEvent("Generate Curve");

  // Delete all existing control points
  ClearAllPoints();

  const double fps = m_Curves.m_uiFramesPerSecond;

  for (ezUInt32 i = 0; i <= m_Curves.m_uiFramesPerSecond; i += 2)
  {
    const double x = i / fps;
    InsertCpAt(x, GetEasingValue(easingFunction, x), ezVec2d::ZeroVector());
  }

  Q_EMIT EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::UpdateSpinBoxes()
{
  const auto& selection = CurveEdit->GetSelection();

  ezQtScopedBlockSignals _1(LinePosition, LineValue);

  if (selection.IsEmpty())
  {
    LinePosition->setText(QString());
    LineValue->setText(QString());

    LinePosition->setEnabled(false);
    LineValue->setEnabled(false);
    return;
  }

  const auto& pt0 = m_Curves.m_Curves[selection[0].m_uiCurve]->m_ControlPoints[selection[0].m_uiPoint];
  const double fPos = pt0.GetTickAsTime().GetSeconds();
  const double fVal = pt0.m_fValue;

  LinePosition->setEnabled(true);
  LineValue->setEnabled(true);

  bool bMultipleTicks = false;
  for (ezUInt32 i = 1; i < selection.GetCount(); ++i)
  {
    const auto& pt = m_Curves.m_Curves[selection[i].m_uiCurve]->m_ControlPoints[selection[i].m_uiPoint];

    if (pt.GetTickAsTime().GetSeconds() != fPos)
    {
      bMultipleTicks = true;
      break;
    }
  }

  bool bMultipleValues = false;
  for (ezUInt32 i = 1; i < selection.GetCount(); ++i)
  {
    const auto& pt = m_Curves.m_Curves[selection[i].m_uiCurve]->m_ControlPoints[selection[i].m_uiPoint];

    if (pt.m_fValue != fVal)
    {
      bMultipleValues = true;
      LineValue->setText(QString());
      break;
    }
  }

  LinePosition->setText(bMultipleTicks ? QString() : QString::number(fPos, 'f', 2));
  LineValue->setText(bMultipleValues ? QString() : QString::number(fVal, 'f', 3));
}

void ezQtCurve1DEditorWidget::on_LinePosition_editingFinished()
{
  QString sValue = LinePosition->text();

  bool ok = false;
  const double value = sValue.toDouble(&ok);
  if (!ok)
    return;

  if (value < 0)
    return;

  const auto& selection = CurveEdit->GetSelection();
  if (selection.IsEmpty())
    return;

  Q_EMIT BeginCpChangesEvent("Set Time");

  for (const auto& cpSel : selection)
  {
    const auto& cp = m_Curves.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];

    ezInt32 iTick = m_Curves.TickFromTime(ezTime::Seconds(value));
    if (cp.m_iTick != iTick)
      Q_EMIT CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, iTick, cp.m_fValue);
  }

  Q_EMIT EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::on_LineValue_editingFinished()
{
  QString sValue = LineValue->text();

  bool ok = false;
  const double value = sValue.toDouble(&ok);
  if (!ok)
    return;

  const auto& selection = CurveEdit->GetSelection();
  if (selection.IsEmpty())
    return;

  Q_EMIT BeginCpChangesEvent("Set Value");

  for (const auto& cpSel : selection)
  {
    const auto& cp = m_Curves.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];

    if (cp.m_fValue != value)
      Q_EMIT CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, cp.m_iTick, value);
  }

  Q_EMIT EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::SetTangentMode(ezCurveTangentMode::Enum mode, bool bLeft, bool bRight)
{
  const auto& selection = CurveEdit->GetSelection();
  if (selection.IsEmpty())
    return;

  Q_EMIT BeginCpChangesEvent("Set Tangent Mode");

  for (const auto& cpSel : selection)
  {
    if (bLeft)
      Q_EMIT CpTangentModeEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, false, (int)mode);

    if (bRight)
      Q_EMIT CpTangentModeEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, true, (int)mode);
  }

  Q_EMIT EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::ClampPoint(double& x, double& y) const
{
  if (CurveEdit->m_bLowerExtentFixed)
    x = ezMath::Max(x, CurveEdit->m_fLowerExtent);
  if (CurveEdit->m_bUpperExtentFixed)
    x = ezMath::Min(x, CurveEdit->m_fUpperExtent);

  y = ezMath::Clamp(y, CurveEdit->m_fLowerRange, CurveEdit->m_fUpperRange);
}
