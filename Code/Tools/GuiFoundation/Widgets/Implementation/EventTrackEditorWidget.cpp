#include <PCH.h>
#include <GuiFoundation/Widgets/EventTrackEditorWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QGraphicsItem>
#include <QPainterPath>
#include <QGraphicsSceneEvent>
#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <QMenu>

ezQtEventTrackEditorWidget::ezQtEventTrackEditorWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setupUi(this);

  EventTrackEdit->SetGridBarWidget(GridBarWidget);

  //connect(EventTrackEdit, &ezQtEventTrackEditWidget::DeleteControlPointsEvent, this, &ezQtEventTrackEditorWidget::onDeleteControlPoints);
  connect(EventTrackEdit, &ezQtEventTrackWidget::DoubleClickEvent, this, &ezQtEventTrackEditorWidget::onDoubleClick);
  //connect(EventTrackEdit, &ezQtEventTrackEditWidget::MoveControlPointsEvent, this, &ezQtEventTrackEditorWidget::onMoveControlPoints);
  //connect(EventTrackEdit, &ezQtEventTrackEditWidget::MoveTangentsEvent, this, &ezQtEventTrackEditorWidget::onMoveTangents);
  //connect(EventTrackEdit, &ezQtEventTrackEditWidget::BeginOperationEvent, this, &ezQtEventTrackEditorWidget::onBeginOperation);
  //connect(EventTrackEdit, &ezQtEventTrackEditWidget::EndOperationEvent, this, &ezQtEventTrackEditorWidget::onEndOperation);
  //connect(EventTrackEdit, &ezQtEventTrackEditWidget::ScaleControlPointsEvent, this, &ezQtEventTrackEditorWidget::onScaleControlPoints);
  //connect(EventTrackEdit, &ezQtEventTrackEditWidget::ContextMenuEvent, this, &ezQtEventTrackEditorWidget::onContextMenu);
  //connect(EventTrackEdit, &ezQtEventTrackEditWidget::SelectionChangedEvent, this, &ezQtEventTrackEditorWidget::onSelectionChanged);
  //connect(EventTrackEdit, &ezQtEventTrackEditWidget::MoveCurveEvent, this, &ezQtEventTrackEditorWidget::onMoveCurve);

  LinePosition->setEnabled(false);
}

ezQtEventTrackEditorWidget::~ezQtEventTrackEditorWidget()
{
}


void ezQtEventTrackEditorWidget::SetData(const ezEventTrackData& data, double fMinCurveLength)
{
    ezQtScopedUpdatesDisabled ud(this);
    ezQtScopedBlockSignals bs(this);

    m_pData = &data;
    EventTrackEdit->SetData(&data, fMinCurveLength);

    //m_fCurveDuration = EventTrackEdit->GetMaxCurveExtent();
    //UpdateSpinBoxes();
}

void ezQtEventTrackEditorWidget::SetScrubberPosition(ezUInt64 uiTick)
{
  EventTrackEdit->SetScrubberPosition(uiTick / 4800.0);
}


void ezQtEventTrackEditorWidget::ClearSelection()
{
  EventTrackEdit->ClearSelection();
}

void ezQtEventTrackEditorWidget::FrameCurve()
{
  EventTrackEdit->FrameCurve();
}

//struct PtToDelete
//{
//  EZ_DECLARE_POD_TYPE();
//
//  ezUInt32 m_uiCurveIdx;
//  ezUInt32 m_uiPointIdx;
//
//  bool operator<(const PtToDelete& rhs) const
//  {
//    return m_uiPointIdx > rhs.m_uiPointIdx;
//  }
//};
//
//void ezQtEventTrackEditorWidget::onDeleteControlPoints()
//{
//  const auto selection = EventTrackEdit->GetSelection();
//
//  if (selection.IsEmpty())
//    return;
//
//  EventTrackEdit->ClearSelection();
//
//  emit BeginCpChangesEvent("Delete Points");
//
//  ezHybridArray<PtToDelete, 16> delOrder;
//
//  for (const auto& item : selection)
//  {
//    auto& pt = delOrder.ExpandAndGetRef();
//    pt.m_uiCurveIdx = item.m_uiCurve;
//    pt.m_uiPointIdx = item.m_uiPoint;
//  }
//
//  delOrder.Sort();
//
//  // delete sorted from back to front to prevent point indices becoming invalidated
//  for (const auto& pt : delOrder)
//  {
//    emit CpDeletedEvent(pt.m_uiCurveIdx, pt.m_uiPointIdx);
//  }
//
//  emit EndCpChangesEvent();
//}

void ezQtEventTrackEditorWidget::onDoubleClick(double scenePosX, double epsilon)
{
  InsertCpAt(scenePosX, ezMath::Abs(epsilon));
}

//void ezQtEventTrackEditorWidget::onMoveControlPoints(double x, double y)
//{
//  m_ControlPointMove += ezVec2d(x, y);
//
//  const auto selection = EventTrackEdit->GetSelection();
//
//  if (selection.IsEmpty())
//    return;
//
//  emit BeginCpChangesEvent("Move Points");
//
//  for (const auto& cpSel : selection)
//  {
//    const auto& cp = m_CurvesBackup.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];
//    ezVec2d newPos = ezVec2d(cp.GetTickAsTime(), cp.m_fValue) + m_ControlPointMove;
//    newPos.x = ezMath::Max(newPos.x, 0.0);
//    newPos.y = ezMath::Clamp(newPos.y, -100000.0, +100000.0);
//
//    emit CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, m_Curves.TickFromTime(newPos.x), newPos.y);
//  }
//
//  emit EndCpChangesEvent();
//}
//
//void ezQtEventTrackEditorWidget::onScaleControlPoints(QPointF refPt, double scaleX, double scaleY)
//{
//  const auto selection = EventTrackEdit->GetSelection();
//
//  if (selection.IsEmpty())
//    return;
//
//  const ezVec2d ref(refPt.x(), refPt.y());
//  const ezVec2d scale(scaleX, scaleY);
//
//  emit BeginCpChangesEvent("Scale Points");
//
//  for (const auto& cpSel : selection)
//  {
//    const auto& cp = m_CurvesBackup.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];
//    ezVec2d newPos = ref + (ezVec2d(cp.GetTickAsTime(), cp.m_fValue) - ref).CompMul(scale);
//    newPos.x = ezMath::Max(newPos.x, 0.0);
//    newPos.y = ezMath::Clamp(newPos.y, -100000.0, +100000.0);
//
//    emit CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, m_Curves.TickFromTime(newPos.x), newPos.y);
//  }
//
//  emit EndCpChangesEvent();
//}

//void ezQtEventTrackEditorWidget::onBeginOperation(QString name)
//{
//  m_CurvesBackup.CloneFrom(m_Curves);
//  m_TangentMove.SetZero();
//  m_ControlPointMove.SetZero();
//
//  emit BeginOperationEvent(name);
//}
//
//void ezQtEventTrackEditorWidget::onEndOperation(bool commit)
//{
//  emit EndOperationEvent(commit);
//}
//
//void ezQtEventTrackEditorWidget::onContextMenu(QPoint pos, QPointF scenePos)
//{
//  if (m_Curves.m_Curves.IsEmpty())
//    return;
//
//  m_contextMenuScenePos = scenePos;
//
//  QMenu m(this);
//  m.setDefaultAction(m.addAction("Add Point", this, SLOT(onAddPoint())));
//
//  const auto& selection = EventTrackEdit->GetSelection();
//
//  if (!selection.IsEmpty())
//  {
//    m.addAction("Delete Points", this, SLOT(onDeleteControlPoints()), QKeySequence(Qt::Key_Delete));
//    m.addSeparator();
//    m.addAction("Link Tangents", this, SLOT(onLinkTangents()));
//    m.addAction("Break Tangents", this, SLOT(onBreakTangents()));
//    m.addAction("Flatten Tangents", this, SLOT(onFlattenTangents()));
//
//    QMenu* cmLT = m.addMenu("Left Tangents");
//    QMenu* cmRT = m.addMenu("Right Tangents");
//    QMenu* cmBT = m.addMenu("Both Tangents");
//
//    cmLT->addAction("Auto", this, [this]() { SetTangentMode(ezCurveTangentMode::Auto, true, false); });
//    cmLT->addAction("Bezier", this, [this]() { SetTangentMode(ezCurveTangentMode::Bezier, true, false); });
//    cmLT->addAction("Fixed Length", this, [this]() { SetTangentMode(ezCurveTangentMode::FixedLength, true, false); });
//    cmLT->addAction("Linear", this, [this]() { SetTangentMode(ezCurveTangentMode::Linear, true, false); });
//
//    cmRT->addAction("Auto", this, [this]() { SetTangentMode(ezCurveTangentMode::Auto, false, true); });
//    cmRT->addAction("Bezier", this, [this]() { SetTangentMode(ezCurveTangentMode::Bezier, false, true); });
//    cmRT->addAction("Fixed Length", this, [this]() { SetTangentMode(ezCurveTangentMode::FixedLength, false, true); });
//    cmRT->addAction("Linear", this, [this]() { SetTangentMode(ezCurveTangentMode::Linear, false, true); });
//
//    cmBT->addAction("Auto", this, [this]() { SetTangentMode(ezCurveTangentMode::Auto, true, true); });
//    cmBT->addAction("Bezier", this, [this]() { SetTangentMode(ezCurveTangentMode::Bezier, true, true); });
//    cmBT->addAction("Fixed Length", this, [this]() { SetTangentMode(ezCurveTangentMode::FixedLength, true, true); });
//    cmBT->addAction("Linear", this, [this]() { SetTangentMode(ezCurveTangentMode::Linear, true, true); });
//  }
//
//  m.addSeparator();
//  QMenu* cm = m.addMenu("Curve");
//  cm->addSeparator();
//  cm->addAction("Normalize X", this, [this]() { NormalizeCurveX(0); });
//  cm->addAction("Normalize Y", this, [this]() { NormalizeCurveY(0); });
//  cm->addAction("Loop: Adjust Last Point", this, [this]() { MakeRepeatable(true); });
//  cm->addAction("Loop: Adjust First Point", this, [this]() { MakeRepeatable(false); });
//
//  m.addAction("Frame", this, [this]() { FrameCurve(); }, QKeySequence(Qt::ControlModifier | Qt::Key_F));
//
//  m.exec(pos);
//}
//
//void ezQtEventTrackEditorWidget::onAddPoint()
//{
//  InsertCpAt(m_contextMenuScenePos.x(), m_contextMenuScenePos.y(), ezVec2d::ZeroVector());
//}

void ezQtEventTrackEditorWidget::InsertCpAt(double posX, double epsilon)
{
  int curveIdx = 0, cpIdx = 0;
  posX = ezMath::Max(posX, 0.0);
  //value = ezMath::Clamp(value, -100000.0, +100000.0);

  // do not insert at a point where a CP already exists
  //if (PickControlPointAt(posX, value, epsilon, curveIdx, cpIdx))
    //return;

  emit InsertCpEvent(m_pData->TickFromTime(posX), "test");
}

//bool ezQtEventTrackEditorWidget::PickControlPointAt(double x, double y, ezVec2d vMaxDistance, ezInt32& out_iCurveIdx, ezInt32& out_iCpIdx) const
//{
//  const ezVec2d at(x, y);
//
//  out_iCurveIdx = -1;
//  out_iCpIdx = -1;
//
//  ezCurve1D CurveData;
//
//  for (ezUInt32 iCurve = 0; iCurve < m_Curves.m_Curves.GetCount(); ++iCurve)
//  {
//    m_Curves.ConvertToRuntimeData(iCurve, CurveData);
//
//    for (ezUInt32 iCP = 0; iCP < CurveData.GetNumControlPoints(); ++iCP)
//    {
//      const auto& cp = CurveData.GetControlPoint(iCP);
//      const ezVec2d dist = cp.m_Position - at;
//
//      if (ezMath::Abs(dist.x) <= vMaxDistance.x && ezMath::Abs(dist.y) <= vMaxDistance.y)
//      {
//        vMaxDistance.x = ezMath::Abs(dist.x);
//        vMaxDistance.y = ezMath::Abs(dist.y);
//
//        out_iCurveIdx = iCurve;
//        out_iCpIdx = iCP;
//      }
//    }
//  }
//
//  return out_iCpIdx >= 0;
//}
//
//void ezQtEventTrackEditorWidget::onSelectionChanged()
//{
//  UpdateSpinBoxes();
//}

//void ezQtEventTrackEditorWidget::UpdateSpinBoxes()
//{
//  const auto& selection = EventTrackEdit->GetSelection();
//
//  ezQtScopedBlockSignals _1(LinePosition, LineValue);
//
//  if (selection.IsEmpty())
//  {
//    LinePosition->setText(QString());
//    LineValue->setText(QString());
//
//    LinePosition->setEnabled(false);
//    LineValue->setEnabled(false);
//    return;
//  }
//
//  const auto& pt0 = m_Curves.m_Curves[selection[0].m_uiCurve]->m_ControlPoints[selection[0].m_uiPoint];
//  const double fPos = pt0.GetTickAsTime();
//  const double fVal = pt0.m_fValue;
//
//  LinePosition->setEnabled(true);
//  LineValue->setEnabled(true);
//
//  bool bMultipleTicks = false;
//  for (ezUInt32 i = 1; i < selection.GetCount(); ++i)
//  {
//    const auto& pt = m_Curves.m_Curves[selection[i].m_uiCurve]->m_ControlPoints[selection[i].m_uiPoint];
//
//    if (pt.GetTickAsTime() != fPos)
//    {
//      bMultipleTicks = true;
//      break;
//    }
//  }
//
//  bool bMultipleValues = false;
//  for (ezUInt32 i = 1; i < selection.GetCount(); ++i)
//  {
//    const auto& pt = m_Curves.m_Curves[selection[i].m_uiCurve]->m_ControlPoints[selection[i].m_uiPoint];
//
//    if (pt.m_fValue != fVal)
//    {
//      bMultipleValues = true;
//      LineValue->setText(QString());
//      break;
//    }
//  }
//
//  LinePosition->setText(bMultipleTicks ? QString() : QString::number(fPos, 'f', 2));
//  LineValue->setText(bMultipleValues ? QString() : QString::number(fVal, 'f', 3));
//}

//void ezQtEventTrackEditorWidget::on_LinePosition_editingFinished()
//{
//  QString sValue = LinePosition->text();
//
//  bool ok = false;
//  const double value = sValue.toDouble(&ok);
//  if (!ok)
//    return;
//
//  if (value < 0)
//    return;
//
//  const auto& selection = EventTrackEdit->GetSelection();
//  if (selection.IsEmpty())
//    return;
//
//  emit BeginCpChangesEvent("Set Time");
//
//  for (const auto& cpSel : selection)
//  {
//    const auto& cp = m_Curves.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];
//
//    ezInt32 iTick = m_Curves.TickFromTime(value);
//    if (cp.m_iTick != iTick)
//      emit CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, iTick, cp.m_fValue);
//  }
//
//  emit EndCpChangesEvent();
//}
//
