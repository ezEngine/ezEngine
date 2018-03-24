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

  m_ControlPointPen.setStyle(Qt::PenStyle::DashLine);
  m_ControlPointPen.setCosmetic(true);

  m_SelectedControlPointBrush.setColor(QColor(220, 200, 50));
  m_SelectedControlPointBrush.setStyle(Qt::BrushStyle::SolidPattern);
}


void ezQtEventTrackWidget::SetData(const ezEventTrackData* pData, double fMinCurveLength)
{
  m_pEditData = pData;
  m_fMaxCurveExtent = fMinCurveLength;

  RecreateSortedData();

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

void ezQtEventTrackWidget::RecreateSortedData()
{
  for (auto& cat : m_Categories)
  {
    cat.m_Points.Clear();
  }

  for (ezUInt32 idx = 0; idx < m_pEditData->m_ControlPoints.GetCount(); ++idx)
  {
    const auto& pt = m_pEditData->m_ControlPoints[idx];

    ezUInt32 uiCategory = 0;
    if (!m_NameToCategory.TryGetValue(pt.m_sEvent, uiCategory))
    {
      uiCategory = m_Categories.GetCount();
      m_NameToCategory[pt.m_sEvent] = uiCategory;
      m_Categories.ExpandAndGetRef().m_sName = pt.m_sEvent;
    }

    auto& npt = m_Categories[uiCategory].m_Points.ExpandAndGetRef();

    npt.m_uiOrgIndex = idx;
    npt.m_fPosX = pt.GetTickAsTime();
  }

  // sort points by X position
  for (auto& cat : m_Categories)
  {
    cat.m_Points.Sort([](const ezQtEventTrackWidget::Point& lhs, const ezQtEventTrackWidget::Point& rhs) -> bool
    {
      return lhs.m_fPosX < rhs.m_fPosX;
    });
  }
}

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

  if (!m_SelectedPoints.IsEmpty())
  {
    for (const auto& cpSel : m_SelectedPoints)
    {
      m_Categories[cpSel.m_uiCategory].m_Points[cpSel.m_uiSortedIdx].m_bSelected = false;
    }

    m_SelectedPoints.Clear();
    update();
  }

  emit SelectionChangedEvent();
}

//bool ezQtEventTrackWidget::IsSelected(ezUInt32 cp) const
//{
//  return m_SelectedCPs.IndexOf(cp) != ezInvalidIndex;
//}

void ezQtEventTrackWidget::SetSelection(SelectedPoint cp)
{
  ClearSelection();

  m_SelectedPoints.PushBack(cp);

  m_Categories[cp.m_uiCategory].m_Points[cp.m_uiSortedIdx].m_bSelected = true;

  //ComputeSelectionRect();

  emit SelectionChangedEvent();
}

//void ezQtEventTrackWidget::ToggleSelected(ezUInt32 cp)
//{
//  SetSelected(cp, !IsSelected(cp));
//
//  //ComputeSelectionRect();
//
//  emit SelectionChangedEvent();
//}
//
//void ezQtEventTrackWidget::SetSelected(ezUInt32 cp, bool set)
//{
//  if (!set)
//  {
//    m_SelectedCPs.Remove(cp);
//  }
//  else
//  {
//    if (!IsSelected(cp))
//    {
//      m_SelectedCPs.PushBack(cp);
//    }
//  }
//
//  //ComputeSelectionRect();
//  emit SelectionChangedEvent();
//}

QRectF ezQtEventTrackWidget::ComputeViewportSceneRect() const
{
  const QPointF topLeft = MapToScene(rect().topLeft());
  const QPointF bottomRight = MapToScene(rect().bottomRight());

  return QRectF(topLeft, bottomRight);
}

static ezColorGammaUB g_EventColors[10 * 3] =
{
  ezColorGammaUB(255, 102, 0), ezColorGammaUB(76, 255, 0), ezColorGammaUB(0, 255, 255),
  ezColorGammaUB(239, 35, 0), ezColorGammaUB(127, 255, 0), ezColorGammaUB(0, 0 ,255),
  ezColorGammaUB(205, 92, 92), ezColorGammaUB(120, 158, 39), ezColorGammaUB(81, 120, 188),
  ezColorGammaUB(255, 105, 180), ezColorGammaUB(0, 250, 154), ezColorGammaUB(0, 191, 255),
  ezColorGammaUB(220, 20, 60), ezColorGammaUB(0, 255, 127), ezColorGammaUB(30, 144, 255),
  ezColorGammaUB(240, 128, 128), ezColorGammaUB(60, 179, 113), ezColorGammaUB(135, 206, 250),
  ezColorGammaUB(178, 34, 34), ezColorGammaUB(46, 139, 87), ezColorGammaUB(65, 105, 225),
  ezColorGammaUB(211, 122, 122), ezColorGammaUB(144, 238, 144), ezColorGammaUB(135, 206, 235),
  ezColorGammaUB(219, 112, 147), ezColorGammaUB(0, 128, 0), ezColorGammaUB(70, 130, 180),
  ezColorGammaUB(255, 182, 193), ezColorGammaUB(102, 205, 170), ezColorGammaUB(100, 149, 237),
};

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
      //const ClickTarget clickedOn = DetectClickTarget(e->pos());

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
          SelectedPoint selectedCP;
          if (PickCpAt(e->pos(), 8, selectedCP))
          {
            if (e->modifiers().testFlag(Qt::ControlModifier))
            {
              //ToggleSelected(uiSelectedCP);
            }
            else if (e->modifiers().testFlag(Qt::ShiftModifier))
            {
              //SetSelected(uiSelectedCP, true);
            }
            else if (e->modifiers().testFlag(Qt::AltModifier))
            {
              //SetSelected(uiSelectedCP, false);
            }
            else
            {
              //if (clickedOn == ClickTarget::Nothing)
                SetSelection(selectedCP);

              //m_State = EditState::DraggingPoints;
              //m_totalPointDrag = QPointF();
            }
          }
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

        update();
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
  if (m_Categories.IsEmpty())
    return;

  painter->save();

  ezHybridArray<QLineF, 100> lines;
  ezHybridArray<QRectF, 100> rects;
  ezHybridArray<QLineF, 100> linesSelected;
  ezHybridArray<QRectF, 100> rectsSelected;

  const double fRectLeft = rect().left() - 10;
  const double fRectRight = rect().right() + 10;
  const double fHeight = rect().bottom() - rect().top();
  const double fStepY = fHeight / m_Categories.GetCount();
  const double fLineStepY = fStepY / 3.5;
  double fOffsetY = rect().top() + fStepY * 0.5;

  QBrush brush = m_ControlPointBrush;
  QPen pen = m_ControlPointPen;

  for (ezUInt32 catIdx = 0; catIdx < m_Categories.GetCount(); ++catIdx)
  {
    const PointCategory& cat = m_Categories[catIdx];

    const auto& allPoints = cat.m_Points;

    for (const auto& point : allPoints)
    {
      double ptPosX = MapFromScene(QPointF(point.m_fPosX, 0)).x();

      if (ptPosX < fRectLeft || ptPosX > fRectRight)
        continue;

      QLineF& l = point.m_bSelected ? linesSelected.ExpandAndGetRef() : lines.ExpandAndGetRef();
      l.setLine(ptPosX, fOffsetY - fLineStepY, ptPosX, fOffsetY + fLineStepY);

      QRectF& r = point.m_bSelected ? rectsSelected.ExpandAndGetRef() : rects.ExpandAndGetRef();
      r.setX(ptPosX - 5.0);
      r.setY(fOffsetY - 5.0);
      r.setWidth(10.0);
      r.setHeight(10.0);
    }

    if (!rects.IsEmpty())
    {
      const ezColorGammaUB& col = g_EventColors[catIdx % EZ_ARRAY_SIZE(g_EventColors)];

      brush.setColor(qRgb(col.r, col.g, col.b));
      pen.setColor(qRgb(col.r, col.g, col.b));

      painter->setBrush(brush);
      painter->setPen(pen);

      painter->drawLines(lines.GetData(), lines.GetCount());

      painter->setPen(Qt::NoPen);
      painter->drawRects(rects.GetData(), rects.GetCount());

      lines.Clear();
      rects.Clear();
    }

    if (!rectsSelected.IsEmpty())
    {
      const QColor hlCol = palette().highlight().color();

      brush.setColor(hlCol);
      pen.setColor(hlCol);

      painter->setBrush(brush);
      painter->setPen(pen);

      painter->drawLines(linesSelected.GetData(), linesSelected.GetCount());
      painter->drawRects(rectsSelected.GetData(), rectsSelected.GetCount());

      linesSelected.Clear();
      rectsSelected.Clear();
    }

    fOffsetY += fStepY;
  }

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

bool ezQtEventTrackWidget::PickCpAt(const QPoint& pos, float fMaxPixelDistance, SelectedPoint& out_Result) const
{
  const ezVec2 at((float)pos.x(), (float)pos.y());
  float fMaxDistSqr = ezMath::Square(fMaxPixelDistance);

  out_Result.m_uiCategory = 0xFFFFFFFF;
  out_Result.m_uiSortedIdx = 0xFFFFFFFF;

  const double fHeight = rect().bottom() - rect().top();
  const double fStepY = fHeight / m_Categories.GetCount();
  double fOffsetY = rect().top() + fStepY * 0.5;

  for (ezUInt32 catIdx = 0; catIdx < m_Categories.GetCount(); ++catIdx)
  {
    const auto& cat = m_Categories[catIdx];

    const double diffY = fOffsetY - pos.y();

    if (ezMath::Abs(diffY) < 20)
    {
      const auto& allPoints = cat.m_Points;

      for (ezUInt32 ptIdx = 0; ptIdx < allPoints.GetCount(); ++ptIdx)
      {
        const auto& point = allPoints[ptIdx];

        double ptPosX = MapFromScene(QPointF(point.m_fPosX, 0)).x();
        const ezVec2 fDiff(ptPosX - pos.x(), diffY);

        const float fDistSqr = fDiff.GetLengthSquared();
        if (fDistSqr <= fMaxDistSqr)
        {
          fMaxDistSqr = fDistSqr;
          out_Result.m_uiCategory = catIdx;
          out_Result.m_uiSortedIdx = ptIdx;
        }
      }
    }

    fOffsetY += fStepY;
  }

  return out_Result.m_uiSortedIdx != 0xFFFFFFFF;
}

ezQtEventTrackWidget::ClickTarget ezQtEventTrackWidget::DetectClickTarget(const QPoint& pos)
{
  //const ezVec2d vScreenPos(pos.x(), pos.y());
  //float fMinDistSQR = ezMath::Square(15);
  //ezInt32 iBestCP = -1;

  //for (ezUInt32 i = 0; i < m_SelectedCPs.GetCount(); ++i)
  //{
  //  const auto& cpSel = m_pEditData->m_ControlPoints[m_SelectedCPs[i]];

  //  const ezVec2d point(cpSel.GetTickAsTime(), cp.m_fValue);

  //  const ezVec2d ptPos = ToVec(MapFromScene(point));

  //    const float distSQR = (ptPos - vScreenPos).GetLengthSquared();
  //    if (distSQR < fMinDistSQR)
  //    {
  //      fMinDistSQR = distSQR;
  //      iBestCP = cpSel.m_uiPoint;
  //    }
  //}

  //if (iBestCP != 0)
  //  return ClickTarget::SelectedPoint;

  return ClickTarget::Nothing;
}
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
