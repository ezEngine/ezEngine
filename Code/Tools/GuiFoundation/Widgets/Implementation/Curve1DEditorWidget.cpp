#include <PCH.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Foundation/Math/Color8UNorm.h>
#include <QGraphicsItem>
#include <QPainterPath>
#include <QGraphicsSceneEvent>
#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <QMenu>

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
  connect(CurveEdit, &ezQtCurveEditWidget::EndOperationEvent, this, [this](bool commit) { emit EndOperationEvent(commit); });
  connect(CurveEdit, &ezQtCurveEditWidget::ScaleControlPointsEvent, this, &ezQtCurve1DEditorWidget::onScaleControlPoints);
  connect(CurveEdit, &ezQtCurveEditWidget::ContextMenuEvent, this, &ezQtCurve1DEditorWidget::onContextMenu);
}

ezQtCurve1DEditorWidget::~ezQtCurve1DEditorWidget()
{
}

void ezQtCurve1DEditorWidget::SetCurves(ezCurve1DAssetData& curves)
{
  ezQtScopedUpdatesDisabled ud(this);
  ezQtScopedBlockSignals bs(this);

  m_Curves = curves;

  CurveEdit->SetCurves(&curves);
}

void ezQtCurve1DEditorWidget::FrameCurve()
{
}

void ezQtCurve1DEditorWidget::on_ButtonFrame_clicked()
{
  FrameCurve();
}

void ezQtCurve1DEditorWidget::on_SpinPosition_valueChanged(double value)
{
}

void ezQtCurve1DEditorWidget::NormalizeCurveX(ezUInt32 uiActiveCurve)
{
  ezCurve1D CurveData;
  m_Curves.ConvertToRuntimeData(uiActiveCurve, CurveData);

  const ezUInt32 numCPs = CurveData.GetNumControlPoints();

  if (numCPs < 2)
    return;

  CurveData.RecomputeExtents();

  float minX, maxX;
  CurveData.QueryExtents(minX, maxX);

  if (minX == 0 && maxX == 1)
    return;

  emit BeginOperationEvent("Normalize Curve (X)");

  const float rangeNorm = 1.0f / (maxX - minX);

  for (ezUInt32 i = 0; i < numCPs; ++i)
  {
    const auto& cp = CurveData.GetControlPoint(i);

    ezVec2 pos = cp.m_Position;
    pos.x -= minX;
    pos.x *= rangeNorm;

    emit CpMovedEvent(uiActiveCurve, i, pos.x, pos.y);

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
  ezCurve1D CurveData;
  m_Curves.ConvertToRuntimeData(uiActiveCurve, CurveData);

  const ezUInt32 numCPs = CurveData.GetNumControlPoints();

  if (numCPs < 2)
    return;

  ezCurve1D CurveDataSorted = CurveData;
  CurveDataSorted.SortControlPoints();
  CurveDataSorted.CreateLinearApproximation();

  float minY, maxY;
  CurveDataSorted.QueryExtremeValues(minY, maxY);

  if (minY == 0 && maxY == 1)
    return;

  emit BeginOperationEvent("Normalize Curve (Y)");

  const float rangeNorm = 1.0f / (maxY - minY);

  for (ezUInt32 i = 0; i < numCPs; ++i)
  {
    const auto& cp = CurveData.GetControlPoint(i);

    ezVec2 pos = cp.m_Position;
    pos.y -= minY;
    pos.y *= rangeNorm;

    emit CpMovedEvent(uiActiveCurve, i, pos.x, pos.y);

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

void ezQtCurve1DEditorWidget::on_ButtonNormalizeX_clicked()
{
  /// \todo Active curve index
  NormalizeCurveX(0);
}


void ezQtCurve1DEditorWidget::on_ButtonNormalizeY_clicked()
{
  /// \todo Active curve index
  NormalizeCurveY(0);
}

struct PtToDelete
{
  EZ_DECLARE_POD_TYPE();

  ezUInt32 m_uiCurveIdx;
  ezUInt32 m_uiPointIdx;

  bool operator<(const PtToDelete& rhs) const
  {
    return m_uiPointIdx > rhs.m_uiPointIdx;
  }
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
  InsertCpAt(scenePos.x(), scenePos.y(), epsilon.x());
}

void ezQtCurve1DEditorWidget::onMoveControlPoints(double x, double y)
{
  m_ControlPointMove += ezVec2(x, y);

  const auto selection = CurveEdit->GetSelection();

  if (selection.IsEmpty())
    return;

  emit BeginCpChangesEvent("Move Points");

  for (const auto& cpSel : selection)
  {
    const auto& cp = m_CurvesBackup.m_Curves[cpSel.m_uiCurve].m_ControlPoints[cpSel.m_uiPoint];
    const ezVec2 newPos = cp.m_Point + m_ControlPointMove;

    emit CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, newPos.x, newPos.y);
  }

  emit EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::onScaleControlPoints(QPointF refPt, double scaleX, double scaleY)
{
  const auto selection = CurveEdit->GetSelection();

  if (selection.IsEmpty())
    return;

  const ezVec2 ref(refPt.x(), refPt.y());
  const ezVec2 scale(scaleX, scaleY);

  emit BeginCpChangesEvent("Scale Points");

  for (const auto& cpSel : selection)
  {
    const auto& cp = m_CurvesBackup.m_Curves[cpSel.m_uiCurve].m_ControlPoints[cpSel.m_uiPoint];
    const ezVec2 newPos = ref + (cp.m_Point - ref).CompMul(scale);

    emit CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, newPos.x, newPos.y);
  }

  emit EndCpChangesEvent();
}

void ezQtCurve1DEditorWidget::onMoveTangents(double x, double y)
{
  m_TangentMove += ezVec2(x, y);

  ezInt32 iCurve;
  ezInt32 iPoint;
  bool bLeftTangent;

  if (!CurveEdit->GetSelectedTangent(iCurve, iPoint, bLeftTangent))
    return;

  emit BeginCpChangesEvent("Move Tangents");

  {
    const auto& cp = m_CurvesBackup.m_Curves[iCurve].m_ControlPoints[iPoint];
    ezVec2 newPos;

    if (bLeftTangent)
      newPos = cp.m_LeftTangent + m_TangentMove;
    else
      newPos = cp.m_RightTangent + m_TangentMove;

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
  m_CurvesBackup = m_Curves;
  m_TangentMove.SetZero();
  m_ControlPointMove.SetZero();

  emit BeginOperationEvent(name);
}

void ezQtCurve1DEditorWidget::onContextMenu(QPoint pos, QPointF scenePos)
{
  m_contextMenuScenePos = scenePos;

  QMenu m(this);
  m.addAction("Add Point", this, SLOT(onAddPoint()));

  const auto& selection = CurveEdit->GetSelection();

  if (!selection.IsEmpty())
  {
    m.addAction("Delete Points", this, SLOT(onDeleteControlPoints()));
    m.addSeparator();
    m.addAction("Link Tangents", this, SLOT(onLinkTangents()));
    m.addAction("Break Tangents", this, SLOT(onBreakTangents()));
    m.addAction("Flatten Tangents", this, SLOT(onFlattenTangents()));

  }

  if (!m.isEmpty())
    m.exec(pos);
}

void ezQtCurve1DEditorWidget::onAddPoint()
{
  InsertCpAt(m_contextMenuScenePos.x(), m_contextMenuScenePos.y(), 0);
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
    const auto& tL = m_Curves.m_Curves[cpSel.m_uiCurve].m_ControlPoints[cpSel.m_uiPoint].m_LeftTangent;
    const auto& tR = m_Curves.m_Curves[cpSel.m_uiCurve].m_ControlPoints[cpSel.m_uiPoint].m_RightTangent;

    emit TangentMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, tL.x, 0, false);
    emit TangentMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, tR.x, 0, true);
  }

  emit EndOperationEvent(true);
}

void ezQtCurve1DEditorWidget::InsertCpAt(float posX, float value, float epsilon)
{
  int curveIdx = 0, cpIdx = 0;

  // do not insert at a point where a CP already exists
  if (PickControlPointAt(posX, value, epsilon, curveIdx, cpIdx))
    return;

  if (!PickCurveAt(posX, value, epsilon, curveIdx, value))
  {
    // by default insert into curve 0
    curveIdx = 0;
  }

  emit InsertCpEvent(curveIdx, posX, value);
}


bool ezQtCurve1DEditorWidget::PickCurveAt(float x, float y, float fMaxYDistance, ezInt32& out_iCurveIdx, float& out_ValueY) const
{
  out_iCurveIdx = -1;
  ezCurve1D CurveData;

  for (ezUInt32 i = 0; i < m_Curves.m_Curves.GetCount(); ++i)
  {
    m_Curves.ConvertToRuntimeData(i, CurveData);

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

bool ezQtCurve1DEditorWidget::PickControlPointAt(float x, float y, float fMaxDistance, ezInt32& out_iCurveIdx, ezInt32& out_iCpIdx) const
{
  const ezVec2 at(x, y);
  float fMaxDistSqr = ezMath::Square(fMaxDistance);

  out_iCurveIdx = -1;
  out_iCpIdx = -1;

  ezCurve1D CurveData;

  for (ezUInt32 iCurve = 0; iCurve < m_Curves.m_Curves.GetCount(); ++iCurve)
  {
    m_Curves.ConvertToRuntimeData(iCurve, CurveData);

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



