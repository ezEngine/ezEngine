#include <PCH.h>
#include <GuiFoundation/Widgets/EventTrackWidget.moc.h>
#include <GuiFoundation/Widgets/GridBarWidget.moc.h>
#include <GuiFoundation/Widgets/WidgetUtils.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/StringBuilder.h>
#include <QPainter>
#include <qevent.h>
#include <QRubberBand>

ezQtEventTrackWidget::ezQtEventTrackWidget(QWidget* parent)
  : QWidget(parent)
{
  setFocusPolicy(Qt::FocusPolicy::ClickFocus);
  setMouseTracking(true);

  m_fSceneTranslationX = -2;
  m_SceneToPixelScale = QPointF(1, -1);

  m_ControlPointBrush.setColor(QColor(200, 150, 0));
  m_ControlPointBrush.setStyle(Qt::BrushStyle::SolidPattern);

  m_SelectedControlPointBrush.setColor(QColor(220, 200, 50));
  m_SelectedControlPointBrush.setStyle(Qt::BrushStyle::SolidPattern);
}


void ezQtEventTrackWidget::SetData(const ezEventTrackData* pData, double fMinCurveLength)
{
  m_pEditData = pData;
  m_fMaxCurveExtent = fMinCurveLength;

  //  // make sure the selection does not contain points that got deleted
  //  for (ezUInt32 i = 0; i < m_SelectedCPs.GetCount(); )
  //  {
  //    if (m_SelectedCPs[i].m_uiCurve >= m_Curves.GetCount() ||
  //      m_SelectedCPs[i].m_uiPoint >= m_Curves[m_SelectedCPs[i].m_uiCurve].GetNumControlPoints())
  //    {
  //      m_SelectedCPs.RemoveAt(i);
  //    }
  //    else
  //    {
  //      ++i;
  //    }
  //  }
  //  ComputeSelectionRect();

  update();
}

//void ezQtEventTrackWidget::SetCurves(ezCurveGroupData* pCurveEditData, double fMinCurveLength, bool bCurveLengthIsFixed)
//{
//  m_pCurveEditData = pCurveEditData;
//
//  m_Curves.Clear();
//  m_Curves.Reserve(pCurveEditData->m_Curves.GetCount());
//
//  for (ezUInt32 i = 0; i < pCurveEditData->m_Curves.GetCount(); ++i)
//  {
//    auto& data = m_Curves.ExpandAndGetRef();
//
//    pCurveEditData->ConvertToRuntimeData(i, data);
//  }

//
//  m_CurvesSorted = m_Curves;
//  m_CurveExtents.SetCount(m_Curves.GetCount());
//  m_fMaxCurveExtent = fMinCurveLength;
//  m_fMinValue = ezMath::BasicType<float>::MaxValue();
//  m_fMaxValue = -ezMath::BasicType<float>::MaxValue();
//
//  for (ezUInt32 i = 0; i < m_CurvesSorted.GetCount(); ++i)
//  {
//    ezCurve1D& curve = m_CurvesSorted[i];
//
//    curve.SortControlPoints();
//    curve.CreateLinearApproximation();
//
//    curve.QueryExtents(m_CurveExtents[i].x, m_CurveExtents[i].y);
//
//    double fMin, fMax;
//    curve.QueryExtremeValues(fMin, fMax);
//
//    if (!bCurveLengthIsFixed)
//      m_fMaxCurveExtent = ezMath::Max(m_fMaxCurveExtent, m_CurveExtents[i].y);
//
//    m_fMinValue = ezMath::Min(m_fMinValue, fMin);
//    m_fMaxValue = ezMath::Max(m_fMaxValue, fMax);
//  }

//}

void ezQtEventTrackWidget::SetScrubberPosition(double fPosition)
{
  m_bShowScrubber = true;
  m_fScrubberPosition = fPosition;

  update();
}

void ezQtEventTrackWidget::FrameCurve()
{
  m_bFrameBeforePaint = false;

  double fWidth = m_fMaxCurveExtent;
  double fOffsetX = 0;

  if (m_pEditData->m_ControlPoints.GetCount() == 0)
  {
    fWidth = 10.0;
  }
  //else if (m_SelectedCPs.GetCount() > 1)
  //{
  //  fWidth = m_selectionBRect.width();

  //  fOffsetX = m_selectionBRect.left();
  //}
  //else if (m_SelectedCPs.GetCount() == 1)
  //{
  //  fWidth = 0.1f;

  //  const auto& point = m_pCurveEditData->m_Curves[m_SelectedCPs[0].m_uiCurve]->m_ControlPoints[m_SelectedCPs[0].m_uiPoint];
  //  fOffsetX = point.GetTickAsTime() - 0.05;
  //}

  fWidth = ezMath::Max(fWidth, 0.1);

  const double fFinalWidth = fWidth * 1.2;

  fOffsetX -= (fFinalWidth - fWidth) * 0.5;

  m_SceneToPixelScale.setX(rect().width() / fFinalWidth);
  m_fSceneTranslationX = fOffsetX;

  ClampZoomPan();

  update();
}

QPoint ezQtEventTrackWidget::MapFromScene(const QPointF& pos) const
{
  double x = pos.x() - m_fSceneTranslationX;
  double y = pos.y();
  x *= m_SceneToPixelScale.x();
  y *= m_SceneToPixelScale.y();

  return QPoint((int)x, (int)y);
}

QPointF ezQtEventTrackWidget::MapToScene(const QPoint& pos) const
{
  double x = pos.x();
  double y = pos.y();
  x /= m_SceneToPixelScale.x();
  y /= m_SceneToPixelScale.y();

  return QPointF(x + m_fSceneTranslationX, y);
}

void ezQtEventTrackWidget::ClearSelection()
{
  //m_selectionBRect = QRectF();

  //if (!m_SelectedCPs.IsEmpty())
  //{
  //  m_SelectedCPs.Clear();
  //  update();
  //}

  emit SelectionChangedEvent();
}

//bool ezQtEventTrackWidget::IsSelected(const ezSelectedCurveCP& cp) const
//{
//  for (const auto& other : m_SelectedCPs)
//  {
//    if (other.m_uiCurve == cp.m_uiCurve && other.m_uiPoint == cp.m_uiPoint)
//      return true;
//  }
//
//  return false;
//}
//
//void ezQtEventTrackWidget::SetSelection(const ezSelectedCurveCP& cp)
//{
//  m_SelectedCPs.Clear();
//  m_SelectedCPs.PushBack(cp);
//
//  ComputeSelectionRect();
//
//  emit SelectionChangedEvent();
//}
//
//void ezQtEventTrackWidget::ToggleSelected(const ezSelectedCurveCP& cp)
//{
//  SetSelected(cp, !IsSelected(cp));
//
//  ComputeSelectionRect();
//
//  emit SelectionChangedEvent();
//}
//
//void ezQtEventTrackWidget::SetSelected(const ezSelectedCurveCP& cp, bool set)
//{
//  if (!set)
//  {
//    for (ezUInt32 i = 0; i < m_SelectedCPs.GetCount(); ++i)
//    {
//      if (m_SelectedCPs[i].m_uiCurve == cp.m_uiCurve && m_SelectedCPs[i].m_uiPoint == cp.m_uiPoint)
//      {
//        m_SelectedCPs.RemoveAt(i);
//        break;
//      }
//    }
//  }
//  else
//  {
//    if (!IsSelected(cp))
//    {
//      m_SelectedCPs.PushBack(cp);
//    }
//  }
//
//  ComputeSelectionRect();
//  emit SelectionChangedEvent();
//}

QRectF ezQtEventTrackWidget::ComputeViewportSceneRect() const
{
  const QPointF topLeft = MapToScene(rect().topLeft());
  const QPointF bottomRight = MapToScene(rect().bottomRight());

  return QRectF(topLeft, bottomRight);
}

void ezQtEventTrackWidget::paintEvent(QPaintEvent* e)
{
  if (m_bFrameBeforePaint)
    FrameCurve();

  ClampZoomPan();

  QPainter painter(this);
  painter.fillRect(rect(), palette().base());
  painter.translate(0.5, 0.5);

  painter.setRenderHint(QPainter::Antialiasing, true);

  const QRectF viewportSceneRect = ComputeViewportSceneRect();

  double fFineGridDensity = 0.01;
  double fRoughGridDensity = 0.01;
  ezWidgetUtils::AdjustGridDensity(fFineGridDensity, fRoughGridDensity, rect().width(), viewportSceneRect.width(), 20);

  RenderVerticalGrid(&painter, viewportSceneRect, fRoughGridDensity);

  if (m_pGridBar)
  {
    m_pGridBar->SetConfig(viewportSceneRect, fRoughGridDensity, fFineGridDensity, [this](const QPointF& pt) -> QPoint
    {
      return MapFromScene(pt);
    });
  }

  PaintOutsideAreaOverlay(&painter);
  PaintControlPoints(&painter);
  PaintSelectedControlPoints(&painter);
  PaintMultiSelectionSquare(&painter);
  PaintScrubber(painter);
}

void ezQtEventTrackWidget::ClampZoomPan()
{
  m_fSceneTranslationX = ezMath::Clamp(m_fSceneTranslationX, -2.0, 50000.0);
  m_SceneToPixelScale.setX(ezMath::Clamp(m_SceneToPixelScale.x(), 0.0005, 10000.0));
  m_SceneToPixelScale.setY(ezMath::Clamp(m_SceneToPixelScale.y(), -10000.0, -0.0005));
}

void ezQtEventTrackWidget::mousePressEvent(QMouseEvent* e)
{
  QWidget::mousePressEvent(e);
  m_LastMousePos = e->pos();

  if (m_State != EditState::None)
    return;

  if (e->button() == Qt::RightButton)
  {
    m_State = EditState::RightClick;
    return;
  }

  if (e->buttons() == Qt::LeftButton) // nothing else pressed
  {
    //  const ClickTarget clickedOn = DetectClickTarget(e->pos());

    //  if (clickedOn == ClickTarget::Nothing || clickedOn == ClickTarget::SelectedPoint)
    //  {
    //    if (e->modifiers() == Qt::NoModifier)
    //    {
    //      m_scaleStartPoint = MapToScene(e->pos());

    //      switch (WhereIsPoint(e->pos()))
    //      {
    //      case ezQtEventTrackWidget::SelectArea::Center:
    //        m_State = EditState::DraggingPoints;
    //        m_totalPointDrag = QPointF();
    //        break;
    //      case ezQtEventTrackWidget::SelectArea::Top:
    //        m_scaleReferencePoint = m_selectionBRect.topLeft();
    //        m_State = EditState::ScaleUpDown;
    //        break;
    //      case ezQtEventTrackWidget::SelectArea::Bottom:
    //        m_scaleReferencePoint = m_selectionBRect.bottomRight();
    //        m_State = EditState::ScaleUpDown;
    //        break;
    //      case ezQtEventTrackWidget::SelectArea::Left:
    //        m_State = EditState::ScaleLeftRight;
    //        m_scaleReferencePoint = m_selectionBRect.topRight();
    //        break;
    //      case ezQtEventTrackWidget::SelectArea::Right:
    //        m_State = EditState::ScaleLeftRight;
    //        m_scaleReferencePoint = m_selectionBRect.topLeft();
    //        break;
    //      }
    //    }

    //    if (m_State == EditState::None)
    //    {
    //      ezSelectedCurveCP cp;
    //      if (PickCpAt(e->pos(), 8, cp))
    //      {
    //        if (e->modifiers().testFlag(Qt::ControlModifier))
    //        {
    //          ToggleSelected(cp);
    //        }
    //        else if (e->modifiers().testFlag(Qt::ShiftModifier))
    //        {
    //          SetSelected(cp, true);
    //        }
    //        else if (e->modifiers().testFlag(Qt::AltModifier))
    //        {
    //          SetSelected(cp, false);
    //        }
    //        else
    //        {
    //          if (clickedOn == ClickTarget::Nothing)
    //            SetSelection(cp);

    //          m_State = EditState::DraggingPoints;
    //          m_totalPointDrag = QPointF();
    //        }
    //      }
    //    }

    //    if (m_State == EditState::None && e->modifiers() == Qt::AltModifier)
    //    {
    //      m_iDraggedCurve = PickCurveAt(e->pos());

    //      if (m_iDraggedCurve >= 0)
    //        m_State = EditState::DraggingCurve;
    //    }

    //    if (m_State == EditState::None)
    //    {
    //      m_State = EditState::MultiSelect;
    //    }

    //    EZ_ASSERT_DEBUG(!m_bBegunChanges, "Invalid State");

    //    if (m_State == EditState::DraggingCurve)
    //    {
    //      emit BeginOperationEvent("Drag Curve");
    //      m_bBegunChanges = true;
    //    }
    //    else if (m_State == EditState::DraggingPoints)
    //    {
    //      emit BeginOperationEvent("Drag Points");
    //      m_bBegunChanges = true;
    //    }
    //    else if (m_State == EditState::ScaleLeftRight)
    //    {
    //      emit BeginOperationEvent("Scale Points Left / Right");
    //      m_bBegunChanges = true;
    //    }
    //    else if (m_State == EditState::ScaleUpDown)
    //    {
    //      emit BeginOperationEvent("Scale Points Up / Down");
    //      m_bBegunChanges = true;
    //    }

    //    update();
    //  }
  }

  if (m_State == EditState::MultiSelect && m_pRubberband == nullptr)
  {
    m_multiSelectionStart = e->pos();
    m_multiSelectRect = QRect();
    m_pRubberband = new QRubberBand(QRubberBand::Shape::Rectangle, this);
    m_pRubberband->setGeometry(QRect(m_multiSelectionStart, QSize()));
    m_pRubberband->hide();
  }
}

void ezQtEventTrackWidget::mouseReleaseEvent(QMouseEvent* e)
{
  QWidget::mouseReleaseEvent(e);

  if (e->button() == Qt::RightButton)
  {
    if (m_State == EditState::Panning)
      m_State = EditState::None;

    if (m_State == EditState::RightClick)
    {
      m_State = EditState::None;

      ContextMenuEvent(mapToGlobal(e->pos()), MapToScene(e->pos()));
    }
  }

  //if (e->button() == Qt::LeftButton &&
  //  (m_State == EditState::DraggingPoints ||
  //    m_State == EditState::DraggingPointsHorz ||
  //    m_State == EditState::DraggingPointsVert ||
  //    m_State == EditState::DraggingTangents ||
  //    m_State == EditState::DraggingCurve ||
  //    m_State == EditState::ScaleLeftRight ||
  //    m_State == EditState::ScaleUpDown ||
  //    m_State == EditState::MultiSelect))
  //{
  //  m_State = EditState::None;
  //  m_iSelectedTangentCurve = -1;
  //  m_iSelectedTangentPoint = -1;
  //  m_totalPointDrag = QPointF();

  //  if (m_bBegunChanges)
  //  {
  //    m_bBegunChanges = false;
  //    emit EndOperationEvent(true);
  //  }

  //  update();
  //}

  if (m_State != EditState::MultiSelect && m_pRubberband)
  {
    delete m_pRubberband;
    m_pRubberband = nullptr;

    if (!m_multiSelectRect.isEmpty())
    {
      //    ezDynamicArray<ezSelectedCurveCP> change;
      //    ExecMultiSelection(change);
      //    m_multiSelectRect = QRect();

      //    if (e->modifiers().testFlag(Qt::AltModifier))
      //    {
      //      CombineSelection(m_SelectedCPs, change, false);
      //    }
      //    else if (e->modifiers().testFlag(Qt::ShiftModifier) || e->modifiers().testFlag(Qt::ControlModifier))
      //    {
      //      CombineSelection(m_SelectedCPs, change, true);
      //    }
      //    else
      //    {
      //      m_SelectedCPs = change;
      //    }

      //    ComputeSelectionRect();
      update();

      emit SelectionChangedEvent();
    }
  }

  if (e->buttons() == Qt::NoButton)
  {
    unsetCursor();

    m_State = EditState::None;

    //  if (m_bBegunChanges)
    //  {
    //    m_bBegunChanges = false;
    //    emit EndOperationEvent(true);
    //  }

    update();
  }
}

void ezQtEventTrackWidget::mouseMoveEvent(QMouseEvent* e)
{
  QWidget::mouseMoveEvent(e);
  Qt::CursorShape cursor = Qt::ArrowCursor;

  const QPoint diff = e->pos() - m_LastMousePos;
  double moveX = (double)diff.x() / m_SceneToPixelScale.x();
  double moveY = 0;// (double)diff.y() / m_SceneToPixelScale.y();

  if (m_State == EditState::RightClick || m_State == EditState::Panning)
  {
    m_State = EditState::Panning;
    cursor = Qt::ClosedHandCursor;

    m_fSceneTranslationX = m_fSceneTranslationX - moveX;

    ClampZoomPan();

    update();
  }

  //if (m_State == EditState::DraggingPoints)
  //{
  //  if (e->modifiers() == Qt::ShiftModifier)
  //  {
  //    if (ezMath::Abs(m_totalPointDrag.x()) > ezMath::Abs(m_totalPointDrag.y()))
  //    {
  //      moveY = -m_totalPointDrag.y();
  //      m_State = EditState::DraggingPointsHorz;
  //    }
  //    else
  //    {
  //      moveX = -m_totalPointDrag.x();
  //      m_State = EditState::DraggingPointsVert;
  //    }
  //  }

  //  MoveControlPointsEvent(moveX, moveY);
  //  m_totalPointDrag += QPointF(moveX, moveY);
  //}
  //else
  //{
  //  if (m_State == EditState::DraggingPointsHorz)
  //  {
  //    MoveControlPointsEvent(moveX, 0);
  //  }

  //  if (m_State == EditState::DraggingPointsVert)
  //  {
  //    MoveControlPointsEvent(0, moveY);
  //  }
  //}

  if (m_State == EditState::MultiSelect && m_pRubberband)
  {
    m_multiSelectRect = QRect(m_multiSelectionStart, e->pos()).normalized();
    m_pRubberband->setGeometry(m_multiSelectRect);
    m_pRubberband->show();
  }

  //if (m_State == EditState::None && !m_selectionBRect.isEmpty())
  //{
  //  switch (WhereIsPoint(e->pos()))
  //  {
  //  case ezQtEventTrackWidget::SelectArea::Center:
  //    //cursor = Qt::SizeAllCursor;
  //    break;
  //  case ezQtEventTrackWidget::SelectArea::Top:
  //  case ezQtEventTrackWidget::SelectArea::Bottom:
  //    cursor = Qt::SizeVerCursor;
  //    break;
  //  case ezQtEventTrackWidget::SelectArea::Left:
  //  case ezQtEventTrackWidget::SelectArea::Right:
  //    cursor = Qt::SizeHorCursor;
  //    break;
  //  }
  //}

  //if (m_State == EditState::ScaleLeftRight)
  //{
  //  cursor = Qt::SizeHorCursor;

  //  const QPointF wsPos = MapToScene(e->pos());
  //  const QPointF norm = m_scaleReferencePoint - m_scaleStartPoint;
  //  const QPointF wsDiff = m_scaleReferencePoint - wsPos;

  //  ScaleControlPointsEvent(m_scaleReferencePoint, wsDiff.x() / norm.x(), 1);
  //}

  setCursor(cursor);
  m_LastMousePos = e->pos();
}

void ezQtEventTrackWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
  QWidget::mouseDoubleClickEvent(e);

  if (e->button() == Qt::LeftButton)
  {
    //  ezSelectedCurveCP cp;
    //  if (PickCpAt(e->pos(), 15, cp))
    //  {
    //    SetSelection(cp);
    //  }
    //  else
    {
      const QPointF epsilon = MapToScene(QPoint(15, 15)) - MapToScene(QPoint(0, 0));
      const QPointF scenePos = MapToScene(e->pos());

      if (m_bBegunChanges)
      {
        m_bBegunChanges = false;
        emit EndOperationEvent(true);
      }

      emit DoubleClickEvent(scenePos.x(), epsilon.x());
    }
  }
}

void ezQtEventTrackWidget::wheelEvent(QWheelEvent* e)
{
  const double ptAtX = MapToScene(mapFromGlobal(e->globalPos())).x();
  double posDiff = m_fSceneTranslationX - ptAtX;

  double changeX = 1.2;
  double changeY = 1.2;

  if (e->modifiers().testFlag(Qt::ShiftModifier))
    changeX = 1;
  if (e->modifiers().testFlag(Qt::ControlModifier))
    changeY = 1;

  const double oldScaleX = m_SceneToPixelScale.x();
  const double oldScaleY = m_SceneToPixelScale.y();

  if (e->delta() > 0)
  {
    m_SceneToPixelScale.setX(m_SceneToPixelScale.x() * changeX);
    m_SceneToPixelScale.setY(m_SceneToPixelScale.y() * changeY);
  }
  else
  {
    m_SceneToPixelScale.setX(m_SceneToPixelScale.x() * (1.0 / changeX));
    m_SceneToPixelScale.setY(m_SceneToPixelScale.y() * (1.0 / changeY));
  }

  ClampZoomPan();

  changeX = m_SceneToPixelScale.x() / oldScaleX;
  changeY = m_SceneToPixelScale.y() / oldScaleY;

  posDiff = posDiff * (1.0 / changeX);

  m_fSceneTranslationX = ptAtX + posDiff;

  ClampZoomPan();

  update();
}

void ezQtEventTrackWidget::keyPressEvent(QKeyEvent* e)
{
  QWidget::keyPressEvent(e);

  if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_F)
  {
    FrameCurve();
  }

  if (e->modifiers() == Qt::NoModifier)
  {
    if (e->key() == Qt::Key_Escape)
    {
      ClearSelection();
    }

    if (e->key() == Qt::Key_Delete)
    {
      emit DeleteControlPointsEvent();
    }
  }
}

void ezQtEventTrackWidget::PaintOutsideAreaOverlay(QPainter* painter) const
{
  const int iRightEdge = MapFromScene(QPointF(ezMath::Max(0.0, m_fMaxCurveExtent), 0)).x();

  if (iRightEdge >= rect().width())
    return;

  QRect area = rect();
  area.setLeft(iRightEdge);

  QBrush b;
  b.setColor(palette().light().color());
  b.setStyle(Qt::BrushStyle::Dense6Pattern);

  painter->setPen(Qt::NoPen);
  painter->setBrush(b);
  painter->drawRect(area);
}

void ezQtEventTrackWidget::PaintControlPoints(QPainter* painter) const
{
  painter->save();
  painter->setBrush(m_ControlPointBrush);

  QPen pen(palette().light(), 1.0f);
  pen.setCosmetic(true);
  painter->setPen(pen);

  const int iy = rect().bottom();
  ezHybridArray<QLine, 100> lines;

  for (ezUInt32 ptIdx = 0; ptIdx < m_pEditData->m_ControlPoints.GetCount(); ++ptIdx)
  {
    const ezEventTrackControlPointData& point = m_pEditData->m_ControlPoints[ptIdx];

    const int ix = MapFromScene(QPointF(point.GetTickAsTime(), 0)).x();

    QLine& l = lines.ExpandAndGetRef();
    l.setLine(ix, 0, ix, iy);

    //painter->drawEllipse(ptPos, 3.5, 3.5);
  }

  painter->drawLines(lines.GetData(), lines.GetCount());

  painter->restore();
}

void ezQtEventTrackWidget::PaintSelectedControlPoints(QPainter* painter) const
{
  painter->save();
  painter->setBrush(m_SelectedControlPointBrush);
  painter->setPen(Qt::NoPen);

  //for (const auto& cpSel : m_SelectedCPs)
  //{
  //  const ezCurve1D& curve = m_Curves[cpSel.m_uiCurve];
  //  const ezCurve1D::ControlPoint& cp = curve.GetControlPoint(cpSel.m_uiPoint);

  //  const QPointF ptPos = MapFromScene(QPointF(cp.m_Position.x, cp.m_Position.y));

  //  painter->drawEllipse(ptPos, 4.5, 4.5);
  //}

  painter->restore();
}

void ezQtEventTrackWidget::PaintMultiSelectionSquare(QPainter* painter) const
{
  //if (m_selectionBRect.isEmpty())
  //  return;

  //painter->save();
  //painter->setPen(Qt::NoPen);

  //QColor col = palette().highlight().color();
  //col.setAlpha(100);
  //painter->setBrush(col);

  //const QPoint tl = MapFromScene(m_selectionBRect.topLeft());
  //const QPoint br = MapFromScene(m_selectionBRect.bottomRight());
  //QRectF r = QRect(tl, br);
  //r.adjust(-4.5, +4.5, +3.5, -5.5);

  //painter->drawRect(r);

  //col.setAlpha(255);
  //QPen pen(col);
  //pen.setStyle(Qt::PenStyle::SolidLine);
  //pen.setCosmetic(true);
  //pen.setWidth(1);
  //pen.setCapStyle(Qt::PenCapStyle::SquareCap);
  //painter->setPen(pen);

  //painter->drawLine(tl.x() - 10, tl.y(), tl.x() - 10, br.y());
  //painter->drawLine(br.x() + 10, tl.y(), br.x() + 10, br.y());
  //painter->drawLine(tl.x(), br.y() - 10, br.x(), br.y() - 10);
  //painter->drawLine(tl.x(), tl.y() + 10, br.x(), tl.y() + 10);

  //painter->restore();
}

void ezQtEventTrackWidget::PaintScrubber(QPainter& p) const
{
  if (!m_bShowScrubber)
    return;

  const QRect area = rect();

  const ezInt32 xPos = MapFromScene(QPointF(m_fScrubberPosition, 0)).x();
  if (xPos < 0 || xPos > area.width())
    return;

  p.save();

  QPen pen;
  pen.setCosmetic(true);
  pen.setColor(palette().highlight().color());
  pen.setWidth(1);

  p.setPen(pen);
  p.drawLine(QLine(xPos, area.top(), xPos, area.bottom()));

  p.restore();
}

void ezQtEventTrackWidget::RenderVerticalGrid(QPainter* painter, const QRectF& viewportSceneRect, double fRoughGridDensity)
{
  double lowX, highX;
  ezWidgetUtils::ComputeGridExtentsX(viewportSceneRect, fRoughGridDensity, lowX, highX);
  lowX = ezMath::Max(lowX, 0.0);

  const int iy = rect().bottom();

  // render grid lines
  {
    QPen pen(palette().light(), 1.0f);
    pen.setCosmetic(true);
    painter->setPen(pen);

    ezHybridArray<QLine, 100> lines;

    for (double x = lowX; x <= highX; x += fRoughGridDensity)
    {
      const int ix = MapFromScene(QPointF(x, x)).x();

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(ix, 0, ix, iy);
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }
}

//bool ezQtEventTrackWidget::PickCpAt(const QPoint& pos, float fMaxPixelDistance, ezSelectedCurveCP& out_Result) const
//{
//  const ezVec2 at((float)pos.x(), (float)pos.y());
//  float fMaxDistSqr = ezMath::Square(fMaxPixelDistance);
//
//  out_Result.m_uiCurve = 0xFFFF;
//
//  for (ezUInt32 uiCurve = 0; uiCurve < m_Curves.GetCount(); ++uiCurve)
//  {
//    const ezCurve1D& curve = m_Curves[uiCurve];
//
//    for (ezUInt32 uiCP = 0; uiCP < curve.GetNumControlPoints(); ++uiCP)
//    {
//      const auto& cp = curve.GetControlPoint(uiCP);
//
//      const QPoint diff = MapFromScene(cp.m_Position) - pos;
//      const ezVec2 fDiff(diff.x(), diff.y());
//
//      const float fDistSqr = fDiff.GetLengthSquared();
//      if (fDistSqr <= fMaxDistSqr)
//      {
//        fMaxDistSqr = fDistSqr;
//        out_Result.m_uiCurve = uiCurve;
//        out_Result.m_uiPoint = uiCP;
//      }
//    }
//  }
//
//  return out_Result.m_uiCurve != 0xFFFF;
//}

//static inline ezVec2d ToVec(const QPoint& pt)
//{
//  return ezVec2d(pt.x(), pt.y());
//}
//
//ezQtEventTrackWidget::ClickTarget ezQtEventTrackWidget::DetectClickTarget(const QPoint& pos)
//{
//  const ezVec2d vScreenPos(pos.x(), pos.y());
//  float fMinDistSQR = ezMath::Square(15);
//  ezInt32 iBestCurve = -1;
//  ezInt32 iBestCP = -1;
//  ezInt32 iBestComp = -1;
//
//  for (ezUInt32 i = 0; i < m_SelectedCPs.GetCount(); ++i)
//  {
//    const auto& cpSel = m_SelectedCPs[i];
//    const auto& cp = m_pCurveEditData->m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];
//
//    const ezVec2d point(cp.GetTickAsTime(), cp.m_fValue);
//
//    const ezVec2d ptPos = ToVec(MapFromScene(point));
//    ezVec2d ptLeft;
//    ezVec2d ptRight;
//
//    //if (cp.m_LeftTangentMode == ezCurveTangentMode::Bezier)
//    ptLeft = ToVec(MapFromScene(point + ezVec2d(cp.m_LeftTangent.x, cp.m_LeftTangent.y)));
//    //else
//      //ptLeft = ToVec(MapFromScene(cp.m_Point)) + MapDirFromScene(cp.m_LeftTangent).GetNormalized() * 50.0f;
//
//    //if (cp.m_RightTangentMode == ezCurveTangentMode::Bezier)
//    ptRight = ToVec(MapFromScene(point + ezVec2d(cp.m_RightTangent.x, cp.m_RightTangent.y)));
//    //else
//      //ptRight = ToVec(MapFromScene(cp.m_Point)) + MapDirFromScene(cp.m_RightTangent).GetNormalized() * 50.0f;
//
//    {
//      const float distSQR = (ptPos - vScreenPos).GetLengthSquared();
//      if (distSQR < fMinDistSQR)
//      {
//        fMinDistSQR = distSQR;
//        iBestCurve = cpSel.m_uiCurve;
//        iBestCP = cpSel.m_uiPoint;
//        iBestComp = 0;
//      }
//    }
//    {
//      const float distSQR = (ptLeft - vScreenPos).GetLengthSquared();
//      if (distSQR < fMinDistSQR)
//      {
//        fMinDistSQR = distSQR;
//        iBestCurve = cpSel.m_uiCurve;
//        iBestCP = cpSel.m_uiPoint;
//        iBestComp = 1;
//      }
//    }
//    {
//      const float distSQR = (ptRight - vScreenPos).GetLengthSquared();
//      if (distSQR < fMinDistSQR)
//      {
//        fMinDistSQR = distSQR;
//        iBestCurve = cpSel.m_uiCurve;
//        iBestCP = cpSel.m_uiPoint;
//        iBestComp = 2;
//      }
//    }
//  }
//
//  m_iSelectedTangentCurve = -1;
//  m_iSelectedTangentPoint = -1;
//  m_bSelectedTangentLeft = false;
//
//  if (iBestComp > 0)
//  {
//    m_iSelectedTangentCurve = iBestCurve;
//    m_iSelectedTangentPoint = iBestCP;
//    m_bSelectedTangentLeft = (iBestComp == 1);
//
//    return ClickTarget::TangentHandle;
//  }
//
//  if (iBestComp == 0)
//    return ClickTarget::SelectedPoint;
//
//  return ClickTarget::Nothing;
//}
//
//void ezQtEventTrackWidget::ExecMultiSelection(ezDynamicArray<ezSelectedCurveCP>& out_Selection)
//{
//  out_Selection.Clear();
//
//  for (ezUInt32 uiCurve = 0; uiCurve < m_Curves.GetCount(); ++uiCurve)
//  {
//    const ezCurve1D& curve = m_Curves[uiCurve];
//
//    for (ezUInt32 uiCP = 0; uiCP < curve.GetNumControlPoints(); ++uiCP)
//    {
//      const auto& cp = curve.GetControlPoint(uiCP);
//
//      const QPoint cpPos = MapFromScene(cp.m_Position);
//
//      if (m_multiSelectRect.contains(cpPos))
//      {
//        auto& sel = out_Selection.ExpandAndGetRef();
//        sel.m_uiCurve = uiCurve;
//        sel.m_uiPoint = uiCP;
//      }
//    }
//  }
//}
//
//bool ezQtEventTrackWidget::CombineSelection(ezDynamicArray<ezSelectedCurveCP>& inout_Selection, const ezDynamicArray<ezSelectedCurveCP>& change, bool add)
//{
//  bool bChange = false;
//
//  for (ezUInt32 i = 0; i < change.GetCount(); ++i)
//  {
//    const auto& cp = change[i];
//
//    if (!add)
//    {
//      bChange |= inout_Selection.Remove(cp);
//    }
//    else if (!inout_Selection.Contains(cp))
//    {
//      inout_Selection.PushBack(cp);
//      bChange = true;
//    }
//  }
//
//  return bChange;
//}
//
//void ezQtEventTrackWidget::ComputeSelectionRect()
//{
//  m_selectionBRect = QRectF();
//
//  if (m_SelectedCPs.GetCount() < 2)
//    return;
//
//  ezBoundingBox bbox;
//  bbox.SetInvalid();
//
//  for (const auto& cpSel : m_SelectedCPs)
//  {
//    const ezCurve1D& curve = m_Curves[cpSel.m_uiCurve];
//    const ezCurve1D::ControlPoint& cp = curve.GetControlPoint(cpSel.m_uiPoint);
//
//    bbox.ExpandToInclude(ezVec3(cp.m_Position.x, cp.m_Position.y, cp.m_Position.x));
//  }
//
//  if (bbox.IsValid())
//  {
//    m_selectionBRect.setCoords(bbox.m_vMin.x, bbox.m_vMin.y, bbox.m_vMax.x, bbox.m_vMax.y);
//    m_selectionBRect.normalized();
//  }
//}
//
//ezQtEventTrackWidget::SelectArea ezQtEventTrackWidget::WhereIsPoint(QPoint pos) const
//{
//  if (m_selectionBRect.isEmpty())
//    return SelectArea::None;
//
//  const QPoint tl = MapFromScene(m_selectionBRect.topLeft());
//  const QPoint br = MapFromScene(m_selectionBRect.bottomRight());
//  QRect selectionRectSS = QRect(tl, br);
//  selectionRectSS.adjust(-4.5, +4.5, +3.5, -5.5);
//
//  const QRect barTop(selectionRectSS.left(), selectionRectSS.bottom() - 10, selectionRectSS.width(), 10);
//  const QRect barBottom(selectionRectSS.left(), selectionRectSS.top(), selectionRectSS.width(), 10);
//  const QRect barLeft(selectionRectSS.left() - 10, selectionRectSS.top(), 10, selectionRectSS.height());
//  const QRect barRight(selectionRectSS.right(), selectionRectSS.top(), 10, selectionRectSS.height());
//
//  if (barTop.contains(pos))
//    return SelectArea::Top;
//
//  if (barBottom.contains(pos))
//    return SelectArea::Bottom;
//
//  if (barLeft.contains(pos))
//    return SelectArea::Left;
//
//  if (barRight.contains(pos))
//    return SelectArea::Right;
//
//  if (selectionRectSS.contains(pos))
//    return SelectArea::Center;
//
//  return SelectArea::None;
//}
