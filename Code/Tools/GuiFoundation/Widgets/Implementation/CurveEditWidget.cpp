#include <PCH.h>
#include <GuiFoundation/Widgets/CurveEditWidget.moc.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <GuiFoundation/Widgets/GridBarWidget.moc.h>
#include <QPainter>
#include <qevent.h>
#include <QRubberBand>

//////////////////////////////////////////////////////////////////////////

static void AdjustGridDensity(double& fFinestDensity, double& fRoughDensity, ezUInt32 uiWindowWidth, double fOrthoDimX, ezUInt32 uiMinPixelsForStep)
{
  const double fMaxStepsFitInWindow = (double)uiWindowWidth / (double)uiMinPixelsForStep;

  const double fStartDensity = fFinestDensity;

  ezInt32 iFactor = 1;
  double fNewDensity = fFinestDensity;
  ezInt32 iFactors[2] = { 5, 2 };
  ezInt32 iLastFactor = 0;

  while (true)
  {
    const double fStepsAtDensity = fOrthoDimX / fNewDensity;

    if (fStepsAtDensity < fMaxStepsFitInWindow)
      break;

    iFactor *= iFactors[iLastFactor];
    fNewDensity = fStartDensity * iFactor;

    iLastFactor = (iLastFactor + 1) % 2;
  }

  fFinestDensity = fStartDensity * iFactor;

  iFactor *= iFactors[iLastFactor];
  fRoughDensity = fStartDensity * iFactor;
}

static void ComputeGridExtentsX2(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinX, double& out_fMaxX)
{
  out_fMinX = ezMath::Floor((double)viewportSceneRect.left(), fGridStops);
  out_fMaxX = ezMath::Ceil((double)viewportSceneRect.right(), fGridStops);
}

static void ComputeGridExtentsY2(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinY, double& out_fMaxY)
{
  out_fMinY = ezMath::Floor((double)viewportSceneRect.top(), fGridStops);
  out_fMaxY = ezMath::Ceil((double)viewportSceneRect.bottom(), fGridStops);
}

//////////////////////////////////////////////////////////////////////////

ezQtCurveEditWidget::ezQtCurveEditWidget(QWidget* parent)
  : QWidget(parent)
{
  setFocusPolicy(Qt::FocusPolicy::ClickFocus);
  setMouseTracking(true);

  m_SceneTranslation = QPointF(-2, 0);
  m_SceneToPixelScale = QPointF(1, -1);

  m_ControlPointBrush.setColor(QColor(200, 150, 0));
  m_ControlPointBrush.setStyle(Qt::BrushStyle::SolidPattern);

  m_SelectedControlPointBrush.setColor(QColor(220, 200, 50));
  m_SelectedControlPointBrush.setStyle(Qt::BrushStyle::SolidPattern);

  m_TangentLinePen.setCosmetic(true);
  m_TangentLinePen.setColor(QColor(100, 100, 255));
  m_TangentLinePen.setStyle(Qt::PenStyle::DashLine);

  m_TangentHandleBrush.setColor(QColor(100, 100, 255));
  m_TangentHandleBrush.setStyle(Qt::BrushStyle::SolidPattern);
}

void ezQtCurveEditWidget::SetCurves(ezCurveGroupData* pCurveEditData, double fMinCurveLength)
{
  m_pCurveEditData = pCurveEditData;

  m_Curves.Clear();
  m_Curves.Reserve(pCurveEditData->m_Curves.GetCount());

  for (ezUInt32 i = 0; i < pCurveEditData->m_Curves.GetCount(); ++i)
  {
    auto& data = m_Curves.ExpandAndGetRef();

    pCurveEditData->ConvertToRuntimeData(i, data);
  }

  // make sure the selection does not contain points that got deleted
  for (ezUInt32 i = 0; i < m_SelectedCPs.GetCount(); )
  {
    if (m_SelectedCPs[i].m_uiCurve >= m_Curves.GetCount() ||
      m_SelectedCPs[i].m_uiPoint >= m_Curves[m_SelectedCPs[i].m_uiCurve].GetNumControlPoints())
    {
      m_SelectedCPs.RemoveAt(i);
    }
    else
    {
      ++i;
    }
  }

  m_CurvesSorted = m_Curves;
  m_CurveExtents.SetCount(m_Curves.GetCount());
  m_fMaxCurveExtent = fMinCurveLength;
  m_fMinValue = ezMath::BasicType<float>::MaxValue();
  m_fMaxValue = -ezMath::BasicType<float>::MaxValue();

  for (ezUInt32 i = 0; i < m_CurvesSorted.GetCount(); ++i)
  {
    ezCurve1D& curve = m_CurvesSorted[i];

    curve.SortControlPoints();
    curve.CreateLinearApproximation();

    curve.QueryExtents(m_CurveExtents[i].x, m_CurveExtents[i].y);

    double fMin, fMax;
    curve.QueryExtremeValues(fMin, fMax);

    m_fMaxCurveExtent = ezMath::Max(m_fMaxCurveExtent, m_CurveExtents[i].y);
    m_fMinValue = ezMath::Min(m_fMinValue, fMin);
    m_fMaxValue = ezMath::Max(m_fMaxValue, fMax);
  }

  ComputeSelectionRect();

  update();
}

void ezQtCurveEditWidget::FrameCurve()
{
  m_bFrameBeforePaint = false;

  double fWidth = m_fMaxCurveExtent;
  double fHeight = m_fMaxValue - m_fMinValue;
  double fOffsetX = 0;
  double fOffsetY = m_fMinValue;

  if (m_Curves.GetCount() == 0)
  {
    fWidth = 10.0;
    fHeight = 10.0;
    fOffsetY = -5.0;
  }
  else if (m_SelectedCPs.GetCount() > 1)
  {
    fWidth = m_selectionBRect.width();
    fHeight = m_selectionBRect.height();

    fOffsetX = m_selectionBRect.left();
    fOffsetY = m_selectionBRect.top();
  }
  else if (m_SelectedCPs.GetCount() == 1)
  {
    fWidth = 0.1f;
    fHeight = 0.1f;

    const auto& point = m_pCurveEditData->m_Curves[m_SelectedCPs[0].m_uiCurve]->m_ControlPoints[m_SelectedCPs[0].m_uiPoint];
    fOffsetX = point.GetTickAsTime() - 0.05;
    fOffsetY = point.m_fValue - 0.05;
  }

  fWidth = ezMath::Max(fWidth, 0.1);
  fHeight = ezMath::Max(fHeight, 0.1);

  const double fFinalWidth = fWidth * 1.2;
  const double fFinalHeight = fHeight * 1.2;

  fOffsetX -= (fFinalWidth - fWidth) * 0.5;
  fOffsetY -= (fFinalHeight - fHeight) * 0.5;

  m_SceneToPixelScale.setX(rect().width() / fFinalWidth);
  m_SceneToPixelScale.setY(-rect().height() / fFinalHeight);
  m_SceneTranslation.setX(fOffsetX);
  m_SceneTranslation.setY((-rect().height() / m_SceneToPixelScale.y()) + fOffsetY);

  ClampZoomPan();

  update();
}

QPoint ezQtCurveEditWidget::MapFromScene(const QPointF& pos) const
{
  double x = pos.x() - m_SceneTranslation.x();
  double y = pos.y() - m_SceneTranslation.y();
  x *= m_SceneToPixelScale.x();
  y *= m_SceneToPixelScale.y();

  return QPoint((int)x, (int)y);
}

QPointF ezQtCurveEditWidget::MapToScene(const QPoint& pos) const
{
  double x = pos.x();
  double y = pos.y();
  x /= m_SceneToPixelScale.x();
  y /= m_SceneToPixelScale.y();

  return QPointF(x, y) + m_SceneTranslation;
}

ezVec2 ezQtCurveEditWidget::MapDirFromScene(const ezVec2& pos) const
{
  const float x = pos.x * m_SceneToPixelScale.x();
  const float y = pos.y * m_SceneToPixelScale.y();

  return ezVec2(x, y);
}

void ezQtCurveEditWidget::ClearSelection()
{
  m_selectionBRect = QRectF();

  if (!m_SelectedCPs.IsEmpty())
  {
    m_SelectedCPs.Clear();
    update();
  }

  emit SelectionChangedEvent();
}

bool ezQtCurveEditWidget::IsSelected(const ezSelectedCurveCP& cp) const
{
  for (const auto& other : m_SelectedCPs)
  {
    if (other.m_uiCurve == cp.m_uiCurve && other.m_uiPoint == cp.m_uiPoint)
      return true;
  }

  return false;
}

void ezQtCurveEditWidget::SetSelection(const ezSelectedCurveCP& cp)
{
  m_SelectedCPs.Clear();
  m_SelectedCPs.PushBack(cp);

  ComputeSelectionRect();

  emit SelectionChangedEvent();
}

void ezQtCurveEditWidget::ToggleSelected(const ezSelectedCurveCP& cp)
{
  SetSelected(cp, !IsSelected(cp));

  ComputeSelectionRect();

  emit SelectionChangedEvent();
}

void ezQtCurveEditWidget::SetSelected(const ezSelectedCurveCP& cp, bool set)
{
  if (!set)
  {
    for (ezUInt32 i = 0; i < m_SelectedCPs.GetCount(); ++i)
    {
      if (m_SelectedCPs[i].m_uiCurve == cp.m_uiCurve && m_SelectedCPs[i].m_uiPoint == cp.m_uiPoint)
      {
        m_SelectedCPs.RemoveAt(i);
        break;
      }
    }
  }
  else
  {
    if (!IsSelected(cp))
    {
      m_SelectedCPs.PushBack(cp);
    }
  }

  ComputeSelectionRect();
  emit SelectionChangedEvent();
}

bool ezQtCurveEditWidget::GetSelectedTangent(ezInt32& out_iCurve, ezInt32& out_iPoint, bool& out_bLeftTangent) const
{
  out_iCurve = m_iSelectedTangentCurve;
  out_iPoint = m_iSelectedTangentPoint;
  out_bLeftTangent = m_bSelectedTangentLeft;
  return (out_iCurve >= 0);
}

QRectF ezQtCurveEditWidget::ComputeViewportSceneRect() const
{
  const QPointF topLeft = MapToScene(rect().topLeft());
  const QPointF bottomRight = MapToScene(rect().bottomRight());

  return QRectF(topLeft, bottomRight);
}

void ezQtCurveEditWidget::paintEvent(QPaintEvent* e)
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
  AdjustGridDensity(fFineGridDensity, fRoughGridDensity, rect().width(), viewportSceneRect.width(), 20);

  RenderVerticalGrid(&painter, viewportSceneRect, fRoughGridDensity);

  if (m_pGridBar)
  {
    m_pGridBar->SetConfig(viewportSceneRect, fRoughGridDensity, fFineGridDensity, [this](const QPointF& pt) -> QPoint
    {
      return MapFromScene(pt);
    });
  }

  RenderSideLinesAndText(&painter, viewportSceneRect);

  PaintCurveSegments(&painter, 0, 255);
  PaintCurveSegments(&painter, m_fMaxCurveExtent, 200);
  PaintCurveSegments(&painter, 2.0f * m_fMaxCurveExtent, 150);
  PaintCurveSegments(&painter, 3.0f * m_fMaxCurveExtent, 100);
  PaintCurveSegments(&painter, 4.0f * m_fMaxCurveExtent, 50);
  PaintOutsideAreaOverlay(&painter);
  PaintSelectedTangentLines(&painter);
  PaintControlPoints(&painter);
  PaintSelectedTangentHandles(&painter);
  PaintSelectedControlPoints(&painter);
  PaintMultiSelectionSquare(&painter);
}

void ezQtCurveEditWidget::ClampZoomPan()
{
  m_SceneTranslation.setX(ezMath::Clamp(m_SceneTranslation.x(), -2.0, 50000.0));
  m_SceneTranslation.setY(ezMath::Clamp(m_SceneTranslation.y(), -200000.0, 200000.0));
  m_SceneToPixelScale.setX(ezMath::Clamp(m_SceneToPixelScale.x(), 0.0005, 10000.0));
  m_SceneToPixelScale.setY(ezMath::Clamp(m_SceneToPixelScale.y(), -10000.0, -0.0005));
}

void ezQtCurveEditWidget::mousePressEvent(QMouseEvent* e)
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
    const ClickTarget clickedOn = DetectClickTarget(e->pos());

    if (clickedOn == ClickTarget::Nothing || clickedOn == ClickTarget::SelectedPoint)
    {
      if (e->modifiers() == Qt::NoModifier)
      {
        m_scaleStartPoint = MapToScene(e->pos());

        switch (WhereIsPoint(e->pos()))
        {
        case ezQtCurveEditWidget::SelectArea::Center:
          m_State = EditState::DraggingPoints;
          m_totalPointDrag = QPointF();
          break;
        case ezQtCurveEditWidget::SelectArea::Top:
          m_scaleReferencePoint = m_selectionBRect.topLeft();
          m_State = EditState::ScaleUpDown;
          break;
        case ezQtCurveEditWidget::SelectArea::Bottom:
          m_scaleReferencePoint = m_selectionBRect.bottomRight();
          m_State = EditState::ScaleUpDown;
          break;
        case ezQtCurveEditWidget::SelectArea::Left:
          m_State = EditState::ScaleLeftRight;
          m_scaleReferencePoint = m_selectionBRect.topRight();
          break;
        case ezQtCurveEditWidget::SelectArea::Right:
          m_State = EditState::ScaleLeftRight;
          m_scaleReferencePoint = m_selectionBRect.topLeft();
          break;
        }
      }

      if (m_State == EditState::None)
      {
        ezSelectedCurveCP cp;
        if (PickCpAt(e->pos(), 8, cp))
        {
          if (e->modifiers().testFlag(Qt::ControlModifier))
          {
            ToggleSelected(cp);
          }
          else if (e->modifiers().testFlag(Qt::ShiftModifier))
          {
            SetSelected(cp, true);
          }
          else if (e->modifiers().testFlag(Qt::AltModifier))
          {
            SetSelected(cp, false);
          }
          else
          {
            if (clickedOn == ClickTarget::Nothing)
              SetSelection(cp);

            m_State = EditState::DraggingPoints;
            m_totalPointDrag = QPointF();
          }
        }
      }

      if (m_State == EditState::None && e->modifiers() == Qt::AltModifier)
      {
        m_iDraggedCurve = PickCurveAt(e->pos());

        if (m_iDraggedCurve >= 0)
          m_State = EditState::DraggingCurve;
      }

      if (m_State == EditState::None)
      {
        m_State = EditState::MultiSelect;
      }

      EZ_ASSERT_DEBUG(!m_bBegunChanges, "Invalid State");

      if (m_State == EditState::DraggingCurve)
      {
        emit BeginOperationEvent("Drag Curve");
        m_bBegunChanges = true;
      }
      else if (m_State == EditState::DraggingPoints)
      {
        emit BeginOperationEvent("Drag Points");
        m_bBegunChanges = true;
      }
      else if (m_State == EditState::ScaleLeftRight)
      {
        emit BeginOperationEvent("Scale Points Left / Right");
        m_bBegunChanges = true;
      }
      else if (m_State == EditState::ScaleUpDown)
      {
        emit BeginOperationEvent("Scale Points Up / Down");
        m_bBegunChanges = true;
      }

      update();
    }
    else if (clickedOn == ClickTarget::TangentHandle)
    {
      m_State = EditState::DraggingTangents;
      emit BeginOperationEvent("Drag Tangents");
      EZ_ASSERT_DEBUG(!m_bBegunChanges, "Invalid State");
      m_bBegunChanges = true;
    }
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

void ezQtCurveEditWidget::mouseReleaseEvent(QMouseEvent* e)
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

  if (e->button() == Qt::LeftButton &&
    (m_State == EditState::DraggingPoints ||
      m_State == EditState::DraggingPointsHorz ||
      m_State == EditState::DraggingPointsVert ||
      m_State == EditState::DraggingTangents ||
      m_State == EditState::DraggingCurve ||
      m_State == EditState::ScaleLeftRight ||
      m_State == EditState::ScaleUpDown ||
      m_State == EditState::MultiSelect))
  {
    m_State = EditState::None;
    m_iSelectedTangentCurve = -1;
    m_iSelectedTangentPoint = -1;
    m_totalPointDrag = QPointF();

    if (m_bBegunChanges)
    {
      m_bBegunChanges = false;
      emit EndOperationEvent(true);
    }

    update();
  }

  if (m_State != EditState::MultiSelect && m_pRubberband)
  {
    delete m_pRubberband;
    m_pRubberband = nullptr;

    if (!m_multiSelectRect.isEmpty())
    {
      ezDynamicArray<ezSelectedCurveCP> change;
      ExecMultiSelection(change);
      m_multiSelectRect = QRect();

      if (e->modifiers().testFlag(Qt::AltModifier))
      {
        CombineSelection(m_SelectedCPs, change, false);
      }
      else if (e->modifiers().testFlag(Qt::ShiftModifier) || e->modifiers().testFlag(Qt::ControlModifier))
      {
        CombineSelection(m_SelectedCPs, change, true);
      }
      else
      {
        m_SelectedCPs = change;
      }

      ComputeSelectionRect();
      update();

      emit SelectionChangedEvent();
    }
  }

  if (e->buttons() == Qt::NoButton)
  {
    unsetCursor();

    m_State = EditState::None;
    m_iSelectedTangentCurve = -1;
    m_iSelectedTangentPoint = -1;

    if (m_bBegunChanges)
    {
      m_bBegunChanges = false;
      emit EndOperationEvent(true);
    }

    update();
  }
}

void ezQtCurveEditWidget::mouseMoveEvent(QMouseEvent* e)
{
  QWidget::mouseMoveEvent(e);
  Qt::CursorShape cursor = Qt::ArrowCursor;

  const QPoint diff = e->pos() - m_LastMousePos;
  double moveX = (double)diff.x() / m_SceneToPixelScale.x();
  double moveY = (double)diff.y() / m_SceneToPixelScale.y();

  if (m_State == EditState::RightClick || m_State == EditState::Panning)
  {
    m_State = EditState::Panning;
    cursor = Qt::ClosedHandCursor;

    m_SceneTranslation.setX(m_SceneTranslation.x() - moveX);
    m_SceneTranslation.setY(m_SceneTranslation.y() - moveY);

    ClampZoomPan();

    update();
  }

  if (m_State == EditState::DraggingPoints)
  {
    if (e->modifiers() == Qt::ShiftModifier)
    {
      if (ezMath::Abs(m_totalPointDrag.x()) > ezMath::Abs(m_totalPointDrag.y()))
      {
        moveY = -m_totalPointDrag.y();
        m_State = EditState::DraggingPointsHorz;
      }
      else
      {
        moveX = -m_totalPointDrag.x();
        m_State = EditState::DraggingPointsVert;
      }
    }

    MoveControlPointsEvent(moveX, moveY);
    m_totalPointDrag += QPointF(moveX, moveY);
  }
  else
  {
    if (m_State == EditState::DraggingPointsHorz)
    {
      MoveControlPointsEvent(moveX, 0);
    }

    if (m_State == EditState::DraggingPointsVert)
    {
      MoveControlPointsEvent(0, moveY);
    }
  }

  if (m_State == EditState::DraggingTangents)
  {
    MoveTangentsEvent(moveX, moveY);
  }

  if (m_State == EditState::MultiSelect && m_pRubberband)
  {
    m_multiSelectRect = QRect(m_multiSelectionStart, e->pos()).normalized();
    m_pRubberband->setGeometry(m_multiSelectRect);
    m_pRubberband->show();
  }

  if (m_State == EditState::None && !m_selectionBRect.isEmpty())
  {
    switch (WhereIsPoint(e->pos()))
    {
    case ezQtCurveEditWidget::SelectArea::Center:
      //cursor = Qt::SizeAllCursor;
      break;
    case ezQtCurveEditWidget::SelectArea::Top:
    case ezQtCurveEditWidget::SelectArea::Bottom:
      cursor = Qt::SizeVerCursor;
      break;
    case ezQtCurveEditWidget::SelectArea::Left:
    case ezQtCurveEditWidget::SelectArea::Right:
      cursor = Qt::SizeHorCursor;
      break;
    }
  }

  if (m_State == EditState::ScaleLeftRight)
  {
    cursor = Qt::SizeHorCursor;

    const QPointF wsPos = MapToScene(e->pos());
    const QPointF norm = m_scaleReferencePoint - m_scaleStartPoint;
    const QPointF wsDiff = m_scaleReferencePoint - wsPos;

    ScaleControlPointsEvent(m_scaleReferencePoint, wsDiff.x() / norm.x(), 1);
  }

  if (m_State == EditState::ScaleUpDown)
  {
    cursor = Qt::SizeVerCursor;

    const QPointF wsPos = MapToScene(e->pos());
    const QPointF norm = m_scaleReferencePoint - m_scaleStartPoint;
    const QPointF wsDiff = m_scaleReferencePoint - wsPos;

    ScaleControlPointsEvent(m_scaleReferencePoint, 1, wsDiff.y() / norm.y());
  }

  if (m_State == EditState::DraggingCurve)
  {
    cursor = Qt::SizeVerCursor;
    emit MoveCurveEvent(m_iDraggedCurve, moveY);
  }

  setCursor(cursor);
  m_LastMousePos = e->pos();
}

void ezQtCurveEditWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
  QWidget::mouseDoubleClickEvent(e);

  if (e->button() == Qt::LeftButton)
  {
    ezSelectedCurveCP cp;
    if (PickCpAt(e->pos(), 15, cp))
    {
      SetSelection(cp);
    }
    else
    {
      const QPointF epsilon = MapToScene(QPoint(15, 15)) - MapToScene(QPoint(0, 0));
      const QPointF scenePos = MapToScene(e->pos());

      if (m_bBegunChanges)
      {
        m_bBegunChanges = false;
        emit EndOperationEvent(true);
      }

      emit DoubleClickEvent(scenePos, epsilon);
    }
  }
}

void ezQtCurveEditWidget::wheelEvent(QWheelEvent* e)
{
  const QPointF ptAt = MapToScene(mapFromGlobal(e->globalPos()));
  QPointF posDiff = m_SceneTranslation - ptAt;

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

  posDiff.setX(posDiff.x() * (1.0 / changeX));
  posDiff.setY(posDiff.y() * (1.0 / changeY));

  m_SceneTranslation = ptAt + posDiff;

  ClampZoomPan();

  update();
}

void ezQtCurveEditWidget::keyPressEvent(QKeyEvent* e)
{
  QWidget::keyPressEvent(e);

  if (e->modifiers() == Qt::NoModifier)
  {
    if (e->key() == Qt::Key_F)
    {
      FrameCurve();
    }

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

void ezQtCurveEditWidget::PaintCurveSegments(QPainter* painter, float fOffsetX, ezUInt8 alpha) const
{
  if (MapFromScene(QPointF(fOffsetX, 0)).x() >= rect().width())
    return;

  painter->save();
  painter->setBrush(Qt::NoBrush);

  QPen pen;
  pen.setCosmetic(true);
  pen.setStyle(Qt::PenStyle::SolidLine);

  for (ezUInt32 curveIdx = 0; curveIdx < m_CurvesSorted.GetCount(); ++curveIdx)
  {
    const ezCurve1D& curve = m_CurvesSorted[curveIdx];
    const ezUInt32 numCps = curve.GetNumControlPoints();

    if (numCps == 0)
      continue;

    const ezColorGammaUB curveColor = m_pCurveEditData->m_Curves[curveIdx]->m_CurveColor;

    pen.setColor(QColor(curveColor.r, curveColor.g, curveColor.b, alpha));
    painter->setPen(pen);

    QPainterPath path;

    {
      const ezCurve1D::ControlPoint& cp = curve.GetControlPoint(0);
      path.moveTo(MapFromScene(QPointF(fOffsetX, cp.m_Position.y)));
      path.lineTo(MapFromScene(QPointF(fOffsetX + cp.m_Position.x, cp.m_Position.y)));
    }

    for (ezUInt32 cpIdx = 1; cpIdx < numCps; ++cpIdx)
    {
      const ezCurve1D::ControlPoint& cpPrev = curve.GetControlPoint(cpIdx - 1);
      const ezCurve1D::ControlPoint& cpThis = curve.GetControlPoint(cpIdx);

      const QPointF startPt = QPointF(fOffsetX + cpPrev.m_Position.x, cpPrev.m_Position.y);
      const QPointF endPt = QPointF(fOffsetX + cpThis.m_Position.x, cpThis.m_Position.y);
      const QPointF tangent1 = QPointF(cpPrev.m_RightTangent.x, cpPrev.m_RightTangent.y);
      const QPointF tangent2 = QPointF(cpThis.m_LeftTangent.x, cpThis.m_LeftTangent.y);
      const QPointF ctrlPt1 = startPt + tangent1;
      const QPointF ctrlPt2 = endPt + tangent2;

      path.moveTo(MapFromScene(startPt));
      path.cubicTo(MapFromScene(ctrlPt1), MapFromScene(ctrlPt2), MapFromScene(endPt));
    }

    {
      const ezCurve1D::ControlPoint& cp = curve.GetControlPoint(numCps - 1);
      path.lineTo(MapFromScene(QPointF(fOffsetX + m_fMaxCurveExtent, cp.m_Position.y)));
    }

    painter->drawPath(path);
  }

  painter->restore();
}

void ezQtCurveEditWidget::PaintOutsideAreaOverlay(QPainter* painter) const
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

void ezQtCurveEditWidget::PaintControlPoints(QPainter* painter) const
{
  painter->save();
  painter->setBrush(m_ControlPointBrush);
  painter->setPen(Qt::NoPen);

  for (ezUInt32 curveIdx = 0; curveIdx < m_Curves.GetCount(); ++curveIdx)
  {
    const ezCurve1D& curve = m_Curves[curveIdx];

    const ezUInt32 numCps = curve.GetNumControlPoints();
    for (ezUInt32 cpIdx = 0; cpIdx < numCps; ++cpIdx)
    {
      const ezCurve1D::ControlPoint& cp = curve.GetControlPoint(cpIdx);

      const QPointF ptPos = MapFromScene(QPointF(cp.m_Position.x, cp.m_Position.y));

      painter->drawEllipse(ptPos, 3.5, 3.5);
    }
  }

  painter->restore();
}

void ezQtCurveEditWidget::PaintSelectedControlPoints(QPainter* painter) const
{
  painter->save();
  painter->setBrush(m_SelectedControlPointBrush);
  painter->setPen(Qt::NoPen);

  for (const auto& cpSel : m_SelectedCPs)
  {
    const ezCurve1D& curve = m_Curves[cpSel.m_uiCurve];
    const ezCurve1D::ControlPoint& cp = curve.GetControlPoint(cpSel.m_uiPoint);

    const QPointF ptPos = MapFromScene(QPointF(cp.m_Position.x, cp.m_Position.y));

    painter->drawEllipse(ptPos, 4.5, 4.5);
  }

  painter->restore();
}

void ezQtCurveEditWidget::PaintSelectedTangentLines(QPainter* painter) const
{
  painter->save();
  painter->setBrush(Qt::NoBrush);
  painter->setPen(m_TangentLinePen);

  ezHybridArray<QLine, 50> lines;

  for (const auto& cpSel : m_SelectedCPs)
  {
    const ezCurve1D& curve = m_Curves[cpSel.m_uiCurve];
    const ezCurve1D::ControlPoint& cp = curve.GetControlPoint(cpSel.m_uiPoint);

    ezVec2d leftHandlePos = cp.m_Position + ezVec2d(cp.m_LeftTangent.x, cp.m_LeftTangent.y);
    ezVec2d rightHandlePos = cp.m_Position + ezVec2d(cp.m_RightTangent.x, cp.m_RightTangent.y);

    const QPoint ptPos = MapFromScene(QPointF(cp.m_Position.x, cp.m_Position.y));
    const QPoint ptPosLeft = MapFromScene(QPointF(leftHandlePos.x, leftHandlePos.y));
    const QPoint ptPosRight = MapFromScene(QPointF(rightHandlePos.x, rightHandlePos.y));

    bool bDrawLeft = m_CurveExtents[cpSel.m_uiCurve].x != cp.m_Position.x;
    bool bDrawRight = m_CurveExtents[cpSel.m_uiCurve].y != cp.m_Position.x;

    const ezCurveTangentMode::Enum tmLeft = m_pCurveEditData->m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint].m_LeftTangentMode;
    const ezCurveTangentMode::Enum tmRight = m_pCurveEditData->m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint].m_RightTangentMode;

    if (bDrawLeft && tmLeft != ezCurveTangentMode::Linear && tmLeft != ezCurveTangentMode::Auto)
    {
      QLine& l1 = lines.ExpandAndGetRef();
      l1.setLine(ptPos.x(), ptPos.y(), ptPosLeft.x(), ptPosLeft.y());
    }

    if (bDrawRight && tmRight != ezCurveTangentMode::Linear && tmRight != ezCurveTangentMode::Auto)
    {
      QLine& l2 = lines.ExpandAndGetRef();
      l2.setLine(ptPos.x(), ptPos.y(), ptPosRight.x(), ptPosRight.y());
    }
  }

  painter->drawLines(lines.GetData(), lines.GetCount());

  painter->restore();
}

void ezQtCurveEditWidget::PaintSelectedTangentHandles(QPainter* painter) const
{
  painter->save();
  painter->setBrush(m_TangentHandleBrush);
  painter->setPen(Qt::NoPen);

  for (const auto& cpSel : m_SelectedCPs)
  {
    const ezCurve1D& curve = m_Curves[cpSel.m_uiCurve];
    const ezCurve1D::ControlPoint& cp = curve.GetControlPoint(cpSel.m_uiPoint);

    const ezCurveTangentMode::Enum tmLeft = m_pCurveEditData->m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint].m_LeftTangentMode;
    const ezCurveTangentMode::Enum tmRight = m_pCurveEditData->m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint].m_RightTangentMode;

    const bool bDrawLeft = m_CurveExtents[cpSel.m_uiCurve].x != cp.m_Position.x;
    const bool bDrawRight = m_CurveExtents[cpSel.m_uiCurve].y != cp.m_Position.x;

    if (bDrawLeft && tmLeft != ezCurveTangentMode::Linear && tmLeft != ezCurveTangentMode::Auto)
    {
      if (tmLeft == ezCurveTangentMode::Bezier)
      {
        const ezVec2d leftHandlePos = cp.m_Position + ezVec2d(cp.m_LeftTangent.x, cp.m_LeftTangent.y);
        const QPointF ptPosLeft = MapFromScene(QPointF(leftHandlePos.x, leftHandlePos.y));
        painter->drawRect(QRectF(ptPosLeft.x() - 4.5, ptPosLeft.y() - 4.5, 9, 9));
      }
      else
      {
        const ezVec2d leftHandlePos = cp.m_Position + ezVec2d(cp.m_LeftTangent.x, cp.m_LeftTangent.y);
        const QPointF ptPosLeft = MapFromScene(QPointF(leftHandlePos.x, leftHandlePos.y));
        //const ezVec2 dir = MapDirFromScene(cp.m_LeftTangent).GetNormalized() * 50.0f;
        //const QPointF ptPosLeft = MapFromScene(QPointF(cp.m_Position.x, cp.m_Position.y)) + QPointF(dir.x, dir.y);
        painter->drawEllipse(QPointF(ptPosLeft.x(), ptPosLeft.y()), 3.5, 3.5);
      }

    }

    if (bDrawRight && tmRight != ezCurveTangentMode::Linear && tmRight != ezCurveTangentMode::Auto)
    {
      if (tmRight == ezCurveTangentMode::Bezier)
      {
        const ezVec2d rightHandlePos = cp.m_Position + ezVec2d(cp.m_RightTangent.x, cp.m_RightTangent.y);
        const QPointF ptPosRight = MapFromScene(QPointF(rightHandlePos.x, rightHandlePos.y));
        painter->drawRect(QRectF(ptPosRight.x() - 4.5, ptPosRight.y() - 4.5, 9, 9));
      }
      else
      {
        const ezVec2d rightHandlePos = cp.m_Position + ezVec2d(cp.m_RightTangent.x, cp.m_RightTangent.y);
        const QPointF ptPosRight = MapFromScene(QPointF(rightHandlePos.x, rightHandlePos.y));
        //const ezVec2 dir = MapDirFromScene(cp.m_RightTangent).GetNormalized() * 50.0f;
        //ptPosRight = MapFromScene(QPointF(cp.m_Position.x, cp.m_Position.y)) + QPointF(dir.x, dir.y);
        painter->drawEllipse(QPointF(ptPosRight.x(), ptPosRight.y()), 3.5, 3.5);
      }
    }
  }

  painter->restore();
}

void ezQtCurveEditWidget::PaintMultiSelectionSquare(QPainter* painter) const
{
  if (m_selectionBRect.isEmpty())
    return;

  painter->save();
  painter->setPen(Qt::NoPen);

  QColor col = palette().highlight().color();
  col.setAlpha(100);
  painter->setBrush(col);

  const QPoint tl = MapFromScene(m_selectionBRect.topLeft());
  const QPoint br = MapFromScene(m_selectionBRect.bottomRight());
  QRectF r = QRect(tl, br);
  r.adjust(-4.5, +4.5, +3.5, -5.5);

  painter->drawRect(r);

  col.setAlpha(255);
  QPen pen(col);
  pen.setStyle(Qt::PenStyle::SolidLine);
  pen.setCosmetic(true);
  pen.setWidth(1);
  pen.setCapStyle(Qt::PenCapStyle::SquareCap);
  painter->setPen(pen);

  painter->drawLine(tl.x() - 10, tl.y(), tl.x() - 10, br.y());
  painter->drawLine(br.x() + 10, tl.y(), br.x() + 10, br.y());
  painter->drawLine(tl.x(), br.y() - 10, br.x(), br.y() - 10);
  painter->drawLine(tl.x(), tl.y() + 10, br.x(), tl.y() + 10);

  painter->restore();
}

void ezQtCurveEditWidget::RenderVerticalGrid(QPainter* painter, const QRectF& viewportSceneRect, double fRoughGridDensity)
{
  double lowX, highX;
  ComputeGridExtentsX2(viewportSceneRect, fRoughGridDensity, lowX, highX);
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

    // zero line
    {
      const QPoint x0 = MapFromScene(QPointF(lowX, 0));
      const QPoint x1 = MapFromScene(QPointF(highX, 0));

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(0, x0.y(), x1.x(), x1.y());
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }
}

void ezQtCurveEditWidget::RenderSideLinesAndText(QPainter* painter, const QRectF& viewportSceneRect)
{
  double fFineGridDensity = 0.01;
  double fRoughGridDensity = 0.01;
  AdjustGridDensity(fFineGridDensity, fRoughGridDensity, rect().height(), ezMath::Abs(viewportSceneRect.height()), 20);

  painter->save();

  const ezInt32 iFineLineLength = 10;
  const ezInt32 iRoughLineLength = 20;

  QRect areaRect = rect();
  areaRect.setRight(areaRect.left() + 30);

  // render fine grid stop lines
  {
    double lowY, highY;
    ComputeGridExtentsY2(viewportSceneRect, fFineGridDensity, lowY, highY);

    if (lowY > highY)
      ezMath::Swap(lowY, highY);

    QPen pen(palette().light(), 1.0f);
    pen.setCosmetic(true);
    painter->setPen(pen);

    ezHybridArray<QLine, 100> lines;

    for (double y = lowY; y <= highY; y += fFineGridDensity)
    {
      const QPoint pos = MapFromScene(QPointF(0, y));

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(0, pos.y(), iFineLineLength, pos.y());
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }

  // render rough grid stop lines
  {
    double lowY, highY;
    ComputeGridExtentsY2(viewportSceneRect, fRoughGridDensity, lowY, highY);

    if (lowY > highY)
      ezMath::Swap(lowY, highY);

    QPen pen(palette().light(), 1.0f);
    pen.setCosmetic(true);
    painter->setPen(pen);

    ezHybridArray<QLine, 100> lines;

    for (double y = lowY; y <= highY; y += fRoughGridDensity)
    {
      const QPoint pos = MapFromScene(QPointF(0, y));

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(0, pos.y(), iRoughLineLength, pos.y());
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }

  // Grid Stop Value Text
  {
    double lowY, highY;
    ComputeGridExtentsY2(viewportSceneRect, fRoughGridDensity, lowY, highY);

    if (lowY > highY)
      ezMath::Swap(lowY, highY);

    QTextOption textOpt(Qt::AlignCenter);
    QRectF textRect;

    painter->setPen(palette().buttonText().color());

    ezStringBuilder tmp;

    for (double y = lowY; y <= highY; y += fRoughGridDensity)
    {
      const QPoint pos = MapFromScene(QPointF(0, y));

      textRect.setRect(0, pos.y() - 15, areaRect.width(), 15);
      tmp.Format("{0}", ezArgF(y));

      painter->drawText(textRect, tmp.GetData(), textOpt);
    }
  }

  painter->restore();
}

bool ezQtCurveEditWidget::PickCpAt(const QPoint& pos, float fMaxPixelDistance, ezSelectedCurveCP& out_Result) const
{
  const ezVec2 at((float)pos.x(), (float)pos.y());
  float fMaxDistSqr = ezMath::Square(fMaxPixelDistance);

  out_Result.m_uiCurve = 0xFFFF;

  for (ezUInt32 uiCurve = 0; uiCurve < m_Curves.GetCount(); ++uiCurve)
  {
    const ezCurve1D& curve = m_Curves[uiCurve];

    for (ezUInt32 uiCP = 0; uiCP < curve.GetNumControlPoints(); ++uiCP)
    {
      const auto& cp = curve.GetControlPoint(uiCP);

      const QPoint diff = MapFromScene(cp.m_Position) - pos;
      const ezVec2 fDiff(diff.x(), diff.y());

      const float fDistSqr = fDiff.GetLengthSquared();
      if (fDistSqr <= fMaxDistSqr)
      {
        fMaxDistSqr = fDistSqr;
        out_Result.m_uiCurve = uiCurve;
        out_Result.m_uiPoint = uiCP;
      }
    }
  }

  return out_Result.m_uiCurve != 0xFFFF;
}

static inline ezVec2d ToVec(const QPoint& pt)
{
  return ezVec2d(pt.x(), pt.y());
}

ezQtCurveEditWidget::ClickTarget ezQtCurveEditWidget::DetectClickTarget(const QPoint& pos)
{
  const ezVec2d vScreenPos(pos.x(), pos.y());
  float fMinDistSQR = ezMath::Square(15);
  ezInt32 iBestCurve = -1;
  ezInt32 iBestCP = -1;
  ezInt32 iBestComp = -1;

  for (ezUInt32 i = 0; i < m_SelectedCPs.GetCount(); ++i)
  {
    const auto& cpSel = m_SelectedCPs[i];
    const auto& cp = m_pCurveEditData->m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];

    const ezVec2d point(cp.GetTickAsTime(), cp.m_fValue);

    const ezVec2d ptPos = ToVec(MapFromScene(point));
    ezVec2d ptLeft;
    ezVec2d ptRight;

    //if (cp.m_LeftTangentMode == ezCurveTangentMode::Bezier)
    ptLeft = ToVec(MapFromScene(point + ezVec2d(cp.m_LeftTangent.x, cp.m_LeftTangent.y)));
    //else
      //ptLeft = ToVec(MapFromScene(cp.m_Point)) + MapDirFromScene(cp.m_LeftTangent).GetNormalized() * 50.0f;

    //if (cp.m_RightTangentMode == ezCurveTangentMode::Bezier)
    ptRight = ToVec(MapFromScene(point + ezVec2d(cp.m_RightTangent.x, cp.m_RightTangent.y)));
    //else
      //ptRight = ToVec(MapFromScene(cp.m_Point)) + MapDirFromScene(cp.m_RightTangent).GetNormalized() * 50.0f;

    {
      const float distSQR = (ptPos - vScreenPos).GetLengthSquared();
      if (distSQR < fMinDistSQR)
      {
        fMinDistSQR = distSQR;
        iBestCurve = cpSel.m_uiCurve;
        iBestCP = cpSel.m_uiPoint;
        iBestComp = 0;
      }
    }
    {
      const float distSQR = (ptLeft - vScreenPos).GetLengthSquared();
      if (distSQR < fMinDistSQR)
      {
        fMinDistSQR = distSQR;
        iBestCurve = cpSel.m_uiCurve;
        iBestCP = cpSel.m_uiPoint;
        iBestComp = 1;
      }
    }
    {
      const float distSQR = (ptRight - vScreenPos).GetLengthSquared();
      if (distSQR < fMinDistSQR)
      {
        fMinDistSQR = distSQR;
        iBestCurve = cpSel.m_uiCurve;
        iBestCP = cpSel.m_uiPoint;
        iBestComp = 2;
      }
    }
  }

  m_iSelectedTangentCurve = -1;
  m_iSelectedTangentPoint = -1;
  m_bSelectedTangentLeft = false;

  if (iBestComp > 0)
  {
    m_iSelectedTangentCurve = iBestCurve;
    m_iSelectedTangentPoint = iBestCP;
    m_bSelectedTangentLeft = (iBestComp == 1);

    return ClickTarget::TangentHandle;
  }

  if (iBestComp == 0)
    return ClickTarget::SelectedPoint;

  return ClickTarget::Nothing;
}

void ezQtCurveEditWidget::ExecMultiSelection(ezDynamicArray<ezSelectedCurveCP>& out_Selection)
{
  out_Selection.Clear();

  for (ezUInt32 uiCurve = 0; uiCurve < m_Curves.GetCount(); ++uiCurve)
  {
    const ezCurve1D& curve = m_Curves[uiCurve];

    for (ezUInt32 uiCP = 0; uiCP < curve.GetNumControlPoints(); ++uiCP)
    {
      const auto& cp = curve.GetControlPoint(uiCP);

      const QPoint cpPos = MapFromScene(cp.m_Position);

      if (m_multiSelectRect.contains(cpPos))
      {
        auto& sel = out_Selection.ExpandAndGetRef();
        sel.m_uiCurve = uiCurve;
        sel.m_uiPoint = uiCP;
      }
    }
  }
}

bool ezQtCurveEditWidget::CombineSelection(ezDynamicArray<ezSelectedCurveCP>& inout_Selection, const ezDynamicArray<ezSelectedCurveCP>& change, bool add)
{
  bool bChange = false;

  for (ezUInt32 i = 0; i < change.GetCount(); ++i)
  {
    const auto& cp = change[i];

    if (!add)
    {
      bChange |= inout_Selection.Remove(cp);
    }
    else if (!inout_Selection.Contains(cp))
    {
      inout_Selection.PushBack(cp);
      bChange = true;
    }
  }

  return bChange;
}

void ezQtCurveEditWidget::ComputeSelectionRect()
{
  m_selectionBRect = QRectF();

  if (m_SelectedCPs.GetCount() < 2)
    return;

  ezBoundingBox bbox;
  bbox.SetInvalid();

  for (const auto& cpSel : m_SelectedCPs)
  {
    const ezCurve1D& curve = m_Curves[cpSel.m_uiCurve];
    const ezCurve1D::ControlPoint& cp = curve.GetControlPoint(cpSel.m_uiPoint);

    bbox.ExpandToInclude(ezVec3(cp.m_Position.x, cp.m_Position.y, cp.m_Position.x));
  }

  if (bbox.IsValid())
  {
    m_selectionBRect.setCoords(bbox.m_vMin.x, bbox.m_vMin.y, bbox.m_vMax.x, bbox.m_vMax.y);
    m_selectionBRect.normalized();
  }
}

ezQtCurveEditWidget::SelectArea ezQtCurveEditWidget::WhereIsPoint(QPoint pos) const
{
  if (m_selectionBRect.isEmpty())
    return SelectArea::None;

  const QPoint tl = MapFromScene(m_selectionBRect.topLeft());
  const QPoint br = MapFromScene(m_selectionBRect.bottomRight());
  QRect selectionRectSS = QRect(tl, br);
  selectionRectSS.adjust(-4.5, +4.5, +3.5, -5.5);

  const QRect barTop(selectionRectSS.left(), selectionRectSS.bottom() - 10, selectionRectSS.width(), 10);
  const QRect barBottom(selectionRectSS.left(), selectionRectSS.top(), selectionRectSS.width(), 10);
  const QRect barLeft(selectionRectSS.left() - 10, selectionRectSS.top(), 10, selectionRectSS.height());
  const QRect barRight(selectionRectSS.right(), selectionRectSS.top(), 10, selectionRectSS.height());

  if (barTop.contains(pos))
    return SelectArea::Top;

  if (barBottom.contains(pos))
    return SelectArea::Bottom;

  if (barLeft.contains(pos))
    return SelectArea::Left;

  if (barRight.contains(pos))
    return SelectArea::Right;

  if (selectionRectSS.contains(pos))
    return SelectArea::Center;

  return SelectArea::None;
}

ezInt32 ezQtCurveEditWidget::PickCurveAt(QPoint pos) const
{
  const QPointF scenePos = MapToScene(pos);
  const float x = scenePos.x();

  ezInt32 iCurveIdx = -1;
  ezInt32 iMinDistance = 15;

  for (ezUInt32 i = 0; i < m_CurvesSorted.GetCount(); ++i)
  {
    double minVal, maxVal;
    m_CurvesSorted[i].QueryExtents(minVal, maxVal);

    if (x < minVal || x > maxVal)
      continue;

    const float val = m_CurvesSorted[i].Evaluate(x);
    const QPoint pixelPos = MapFromScene(QPointF(x, val));

    const ezInt32 dist = ezMath::Abs(pixelPos.y() - pos.y());
    if (dist < iMinDistance)
    {
      iMinDistance = dist;
      iCurveIdx = i;
    }
  }

  return iCurveIdx;
}
