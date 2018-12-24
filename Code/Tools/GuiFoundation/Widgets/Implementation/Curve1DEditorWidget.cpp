#include <PCH.h>

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

void ezQtCurve1DEditorWidget::SetCurves(ezCurveGroupData& curves, double fMinCurveLength, bool bCurveLengthIsFixed)
{
  ezQtScopedUpdatesDisabled ud(this);
  ezQtScopedBlockSignals bs(this);

  m_Curves.CloneFrom(curves);

  CurveEdit->SetCurves(&curves, fMinCurveLength, bCurveLengthIsFixed);
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

void ezQtCurve1DEditorWidget::MakeRepeatable(bool bAdjustLastPoint)
{
  emit BeginOperationEvent("Make Curve Repeatable");

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

    // copy data, the first emit may change the backing store
    const ezCurveControlPointData cpLeft = curve.m_ControlPoints[uiMinCp];
    const ezCurveControlPointData cpRight = curve.m_ControlPoints[uiMaxCp];

    if (bAdjustLastPoint)
    {
      emit CpMovedEvent(iCurveIdx, uiMaxCp, (ezInt64)(m_fCurveDuration * 4800.0), cpLeft.m_fValue);
      emit TangentMovedEvent(iCurveIdx, uiMaxCp, -cpLeft.m_RightTangent.x, -cpLeft.m_RightTangent.y, false);
    }
    else
    {
      emit CpMovedEvent(iCurveIdx, uiMinCp, 0, cpRight.m_fValue);
      emit TangentMovedEvent(iCurveIdx, uiMinCp, -cpRight.m_LeftTangent.x, -cpRight.m_LeftTangent.y, true);
    }
  }

  emit EndOperationEvent(true);
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

  emit BeginOperationEvent("Normalize Curve (X)");

  const float rangeNorm = 1.0f / (maxX - minX);

  for (ezUInt32 i = 0; i < numCPs; ++i)
  {
    const auto& cp = CurveData.GetControlPoint(i);

    ezVec2d pos = cp.m_Position;
    pos.x -= minX;
    pos.x *= rangeNorm;

    emit CpMovedEvent(uiActiveCurve, i, m_Curves.TickFromTime(ezTime::Seconds(pos.x)), pos.y);

    ezVec2 lt = cp.m_LeftTangent;
    lt.x *= rangeNorm;
    emit TangentMovedEvent(uiActiveCurve, i, lt.x, lt.y, false);

    ezVec2 rt = cp.m_RightTangent;
    rt.x *= rangeNorm;
    emit TangentMovedEvent(uiActiveCurve, i, rt.x, rt.y, true);
  }

  emit EndOperationEvent(true);

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

  emit BeginOperationEvent("Normalize Curve (Y)");

  const float rangeNorm = 1.0f / (maxY - minY);

  for (ezUInt32 i = 0; i < numCPs; ++i)
  {
    const auto& cp = CurveData.GetControlPoint(i);

    ezVec2d pos = cp.m_Position;
    pos.y -= minY;
    pos.y *= rangeNorm;

    emit CpMovedEvent(uiActiveCurve, i, m_Curves.TickFromTime(ezTime::Seconds(pos.x)), pos.y);

    ezVec2 lt = cp.m_LeftTangent;
    lt.y *= rangeNorm;
    emit TangentMovedEvent(uiActiveCurve, i, lt.x, lt.y, false);

    ezVec2 rt = cp.m_RightTangent;
    rt.y *= rangeNorm;
    emit TangentMovedEvent(uiActiveCurve, i, rt.x, rt.y, true);
  }

  emit EndOperationEvent(true);

  FrameCurve();
}

struct PtToDelete
{
  EZ_DECLARE_POD_TYPE();

  ezUInt32 m_uiCurveIdx;
  ezUInt32 m_uiPointIdx;

  bool operator<(const PtToDelete& rhs) const { return m_uiPointIdx > rhs.m_uiPointIdx; }
};

void ezQtCurve1DEditorWidget::onDeleteControlPoints()
{
  const auto selection = CurveEdit->GetSelection();

  if (selection.IsEmpty())
    return;

  CurveEdit->ClearSelection();

  emit BeginCpChangesEvent("Delete Points");

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
    emit CpDeletedEvent(pt.m_uiCurveIdx, pt.m_uiPointIdx);
  }

  emit EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::onDoubleClick(const QPointF& scenePos, const QPointF& epsilon)
{
  InsertCpAt(scenePos.x(), scenePos.y(), ezVec2d(ezMath::Abs(epsilon.x()), ezMath::Abs(epsilon.y())));
}

void ezQtCurve1DEditorWidget::onMoveControlPoints(double x, double y)
{
  m_ControlPointMove += ezVec2d(x, y);

  const auto selection = CurveEdit->GetSelection();

  if (selection.IsEmpty())
    return;

  emit BeginCpChangesEvent("Move Points");

  for (const auto& cpSel : selection)
  {
    const auto& cp = m_CurvesBackup.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];
    ezVec2d newPos = ezVec2d(cp.GetTickAsTime().GetSeconds(), cp.m_fValue) + m_ControlPointMove;
    newPos.x = ezMath::Max(newPos.x, 0.0);
    newPos.y = ezMath::Clamp(newPos.y, -100000.0, +100000.0);

    emit CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, m_Curves.TickFromTime(ezTime::Seconds(newPos.x)), newPos.y);
  }

  emit EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::onScaleControlPoints(QPointF refPt, double scaleX, double scaleY)
{
  const auto selection = CurveEdit->GetSelection();

  if (selection.IsEmpty())
    return;

  const ezVec2d ref(refPt.x(), refPt.y());
  const ezVec2d scale(scaleX, scaleY);

  emit BeginCpChangesEvent("Scale Points");

  for (const auto& cpSel : selection)
  {
    const auto& cp = m_CurvesBackup.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];
    ezVec2d newPos = ref + (ezVec2d(cp.GetTickAsTime().GetSeconds(), cp.m_fValue) - ref).CompMul(scale);
    newPos.x = ezMath::Max(newPos.x, 0.0);
    newPos.y = ezMath::Clamp(newPos.y, -100000.0, +100000.0);

    emit CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, m_Curves.TickFromTime(ezTime::Seconds(newPos.x)), newPos.y);
  }

  emit EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::onMoveTangents(float x, float y)
{
  m_TangentMove += ezVec2(x, y);

  ezInt32 iCurve;
  ezInt32 iPoint;
  bool bLeftTangent;

  if (!CurveEdit->GetSelectedTangent(iCurve, iPoint, bLeftTangent))
    return;

  emit BeginCpChangesEvent("Move Tangents");

  {
    const auto& cp = m_CurvesBackup.m_Curves[iCurve]->m_ControlPoints[iPoint];
    ezVec2 newPos;

    if (bLeftTangent)
      newPos = cp.m_LeftTangent + m_TangentMove;
    else
      newPos = cp.m_RightTangent + m_TangentMove;

    newPos.y = ezMath::Clamp(newPos.y, -100000.0f, +100000.0f);

    emit TangentMovedEvent(iCurve, iPoint, newPos.x, newPos.y, !bLeftTangent);

    if (cp.m_bTangentsLinked)
    {
      emit TangentMovedEvent(iCurve, iPoint, -newPos.x, -newPos.y, bLeftTangent);
    }
  }

  emit EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::onBeginOperation(QString name)
{
  m_CurvesBackup.CloneFrom(m_Curves);
  m_TangentMove.SetZero();
  m_ControlPointMove.SetZero();

  emit BeginOperationEvent(name);
}

void ezQtCurve1DEditorWidget::onEndOperation(bool commit)
{
  emit EndOperationEvent(commit);
}

void ezQtCurve1DEditorWidget::onContextMenu(QPoint pos, QPointF scenePos)
{
  if (m_Curves.m_Curves.IsEmpty())
    return;

  m_contextMenuScenePos = scenePos;

  QMenu m(this);
  m.setDefaultAction(m.addAction("Add Point", this, SLOT(onAddPoint())));

  const auto& selection = CurveEdit->GetSelection();

  if (!selection.IsEmpty())
  {
    m.addAction("Delete Points", this, SLOT(onDeleteControlPoints()), QKeySequence(Qt::Key_Delete));
    m.addSeparator();
    m.addAction("Link Tangents", this, SLOT(onLinkTangents()));
    m.addAction("Break Tangents", this, SLOT(onBreakTangents()));
    m.addAction("Flatten Tangents", this, SLOT(onFlattenTangents()));

    QMenu* cmLT = m.addMenu("Left Tangents");
    QMenu* cmRT = m.addMenu("Right Tangents");
    QMenu* cmBT = m.addMenu("Both Tangents");

    cmLT->addAction("Auto", this, [this]() { SetTangentMode(ezCurveTangentMode::Auto, true, false); });
    cmLT->addAction("Bezier", this, [this]() { SetTangentMode(ezCurveTangentMode::Bezier, true, false); });
    cmLT->addAction("Fixed Length", this, [this]() { SetTangentMode(ezCurveTangentMode::FixedLength, true, false); });
    cmLT->addAction("Linear", this, [this]() { SetTangentMode(ezCurveTangentMode::Linear, true, false); });

    cmRT->addAction("Auto", this, [this]() { SetTangentMode(ezCurveTangentMode::Auto, false, true); });
    cmRT->addAction("Bezier", this, [this]() { SetTangentMode(ezCurveTangentMode::Bezier, false, true); });
    cmRT->addAction("Fixed Length", this, [this]() { SetTangentMode(ezCurveTangentMode::FixedLength, false, true); });
    cmRT->addAction("Linear", this, [this]() { SetTangentMode(ezCurveTangentMode::Linear, false, true); });

    cmBT->addAction("Auto", this, [this]() { SetTangentMode(ezCurveTangentMode::Auto, true, true); });
    cmBT->addAction("Bezier", this, [this]() { SetTangentMode(ezCurveTangentMode::Bezier, true, true); });
    cmBT->addAction("Fixed Length", this, [this]() { SetTangentMode(ezCurveTangentMode::FixedLength, true, true); });
    cmBT->addAction("Linear", this, [this]() { SetTangentMode(ezCurveTangentMode::Linear, true, true); });
  }

  m.addSeparator();
  QMenu* cm = m.addMenu("Curve");
  cm->addSeparator();
  cm->addAction("Normalize X", this, [this]() { NormalizeCurveX(0); });
  cm->addAction("Normalize Y", this, [this]() { NormalizeCurveY(0); });
  cm->addAction("Loop: Adjust Last Point", this, [this]() { MakeRepeatable(true); });
  cm->addAction("Loop: Adjust First Point", this, [this]() { MakeRepeatable(false); });

  m.addAction("Frame", this, [this]() { FrameCurve(); }, QKeySequence(Qt::ControlModifier | Qt::Key_F));

  m.exec(pos);
}

void ezQtCurve1DEditorWidget::onAddPoint()
{
  InsertCpAt(m_contextMenuScenePos.x(), m_contextMenuScenePos.y(), ezVec2d::ZeroVector());
}

void ezQtCurve1DEditorWidget::onLinkTangents()
{
  const auto& selection = CurveEdit->GetSelection();

  emit BeginOperationEvent("Link Tangents");

  for (const auto& cpSel : selection)
  {
    emit TangentLinkEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, true);
  }

  emit EndOperationEvent(true);
}

void ezQtCurve1DEditorWidget::onBreakTangents()
{
  const auto& selection = CurveEdit->GetSelection();

  emit BeginOperationEvent("Break Tangents");

  for (const auto& cpSel : selection)
  {
    emit TangentLinkEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, false);
  }

  emit EndOperationEvent(true);
}


void ezQtCurve1DEditorWidget::onFlattenTangents()
{
  const auto& selection = CurveEdit->GetSelection();

  emit BeginOperationEvent("Flatten Tangents");

  for (const auto& cpSel : selection)
  {
    const auto& tL = m_Curves.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint].m_LeftTangent;
    const auto& tR = m_Curves.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint].m_RightTangent;

    emit TangentMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, tL.x, 0, false);
    emit TangentMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, tR.x, 0, true);
  }

  emit EndOperationEvent(true);
}

void ezQtCurve1DEditorWidget::InsertCpAt(double posX, double value, ezVec2d epsilon)
{
  int curveIdx = 0, cpIdx = 0;
  posX = ezMath::Max(posX, 0.0);
  value = ezMath::Clamp(value, -100000.0, +100000.0);

  // do not insert at a point where a CP already exists
  if (PickControlPointAt(posX, value, epsilon, curveIdx, cpIdx))
    return;

  if (!PickCurveAt(posX, value, epsilon.y, curveIdx, value))
  {
    // by default insert into curve 0
    curveIdx = 0;
  }

  emit InsertCpEvent(curveIdx, m_Curves.TickFromTime(ezTime::Seconds(posX)), value);
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

bool ezQtCurve1DEditorWidget::PickControlPointAt(double x, double y, ezVec2d vMaxDistance, ezInt32& out_iCurveIdx,
                                                 ezInt32& out_iCpIdx) const
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

  emit BeginCpChangesEvent("Move Curve");

  const auto& curve = *m_CurvesBackup.m_Curves[iCurve];
  ezUInt32 uiNumCps = curve.m_ControlPoints.GetCount();
  for (ezUInt32 i = 0; i < uiNumCps; ++i)
  {
    const ezInt64 x = curve.m_ControlPoints[i].m_iTick;
    const float y = curve.m_ControlPoints[i].m_fValue + m_ControlPointMove.y;

    emit CpMovedEvent(iCurve, i, x, y);
  }

  emit EndCpChangesEvent();
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

  emit BeginCpChangesEvent("Set Time");

  for (const auto& cpSel : selection)
  {
    const auto& cp = m_Curves.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];

    ezInt32 iTick = m_Curves.TickFromTime(ezTime::Seconds(value));
    if (cp.m_iTick != iTick)
      emit CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, iTick, cp.m_fValue);
  }

  emit EndCpChangesEvent();
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

  emit BeginCpChangesEvent("Set Value");

  for (const auto& cpSel : selection)
  {
    const auto& cp = m_Curves.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];

    if (cp.m_fValue != value)
      emit CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, cp.m_iTick, value);
  }

  emit EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::SetTangentMode(ezCurveTangentMode::Enum mode, bool bLeft, bool bRight)
{
  const auto& selection = CurveEdit->GetSelection();
  if (selection.IsEmpty())
    return;

  emit BeginCpChangesEvent("Set Tangent Mode");

  for (const auto& cpSel : selection)
  {
    if (bLeft)
      emit CpTangentModeEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, false, (int)mode);

    if (bRight)
      emit CpTangentModeEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, true, (int)mode);
  }

  emit EndCpChangesEvent();
}
