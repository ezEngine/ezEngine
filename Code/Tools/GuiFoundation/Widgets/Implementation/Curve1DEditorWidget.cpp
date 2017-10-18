#include <PCH.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Foundation/Math/Color8UNorm.h>
#include <QGraphicsItem>
#include <QPainterPath>
#include <QGraphicsSceneEvent>
#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>

ezQtCurve1DEditorWidget::ezQtCurve1DEditorWidget(QWidget* pParent)
  : QWidget(pParent)
  , m_Scene(this)
{
  setupUi(this);

  m_Scene.setItemIndexMethod(QGraphicsScene::NoIndex);
  GraphicsView->setScene(&m_Scene);

  // this is broken in Qt
  GraphicsView->setCacheMode(QGraphicsView::CacheNone);
  //GraphicsView->SetGridBarWidget(GridBarWidget);
  CurveEdit->SetGridBarWidget(GridBarWidget);

  connect(GraphicsView, &ezQtGraphicsView::BeginDrag, this, [this]() { emit BeginOperation(); });
  connect(GraphicsView, &ezQtGraphicsView::EndDrag, this, [this]() { emit EndOperation(true); });
  connect(GraphicsView, &ezQtGraphicsView::DeleteCPs, this, &ezQtCurve1DEditorWidget::onDeleteCPs);
}


ezQtCurve1DEditorWidget::~ezQtCurve1DEditorWidget()
{

}

void ezQtCurve1DEditorWidget::SetCurves(const ezArrayPtr<ezCurve1D>& curves)
{
  if (curves.GetCount() < m_Curves.GetCount())
  {
    m_Scene.blockSignals(true);
    m_Scene.clear();
    m_Curves.Clear();
    m_Scene.blockSignals(false);
  }

  m_Curves.SetCount(curves.GetCount());

  ezQtScopedUpdatesDisabled ud(this);
  ezQtScopedBlockSignals bs(this);

  for (ezUInt32 i = 0; i < curves.GetCount(); ++i)
  {
    m_Curves[i].m_Curve = curves[i];
  }

  UpdateCpUi();

  CurveEdit->SetCurves(curves);
}

void ezQtCurve1DEditorWidget::FrameCurve()
{

  //curveWidget->update();
}


void ezQtCurve1DEditorWidget::SetControlPoints(const ezSet<ControlPointMove>& moves)
{
  if (m_Scene.signalsBlocked())
    return;

  m_Scene.blockSignals(true);
  emit BeginCpChanges();

  for (const auto& m : moves)
  {
    if (m.tangentIdx == 0)
      emit CpMoved(m.curveIdx, m.cpIdx, m.x, m.y);
    else
      emit TangentMoved(m.curveIdx, m.cpIdx, m.x, m.y, m.tangentIdx == 2);
  }

  emit EndCpChanges();
  m_Scene.blockSignals(false);

  UpdateCpUi();
}

void ezQtCurve1DEditorWidget::InsertControlPointAt(float x, float y, float epsilon)
{
  InsertCpAt(x, y, epsilon);
}

void ezQtCurve1DEditorWidget::on_ButtonFrame_clicked()
{
  FrameCurve();
}

//void ezQtCurve1DEditorWidget::on_curveWidget_selectionChanged(ezInt32 colorCP, ezInt32 alphaCP, ezInt32 intensityCP)
//{
//  m_iSelectedColorCP = colorCP;
//  m_iSelectedAlphaCP = alphaCP;
//  m_iSelectedIntensityCP = intensityCP;
//
//  SpinPosition->setEnabled((m_iSelectedColorCP != -1) || (m_iSelectedAlphaCP != -1) || (m_iSelectedIntensityCP != -1));
//
//  LabelColor->setVisible(m_iSelectedColorCP != -1);
//  ButtonColor->setVisible(m_iSelectedColorCP != -1);
//
//  LabelAlpha->setVisible(m_iSelectedAlphaCP != -1);
//  SpinAlpha->setVisible(m_iSelectedAlphaCP != -1);
//  SliderAlpha->setVisible(m_iSelectedAlphaCP != -1);
//
//  LabelIntensity->setVisible(m_iSelectedIntensityCP != -1);
//  SpinIntensity->setVisible(m_iSelectedIntensityCP != -1);
//
//  UpdateCpUi();
//}


void ezQtCurve1DEditorWidget::on_SpinPosition_valueChanged(double value)
{
  //if (m_iSelectedColorCP != -1)
  //{
  //  emit ColorCpMoved(m_iSelectedColorCP, value);
  //}
}

void ezQtCurve1DEditorWidget::on_ButtonNormalizeX_clicked()
{
  emit NormalizeRangeX();
}


void ezQtCurve1DEditorWidget::on_ButtonNormalizeY_clicked()
{
  emit NormalizeRangeY();
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

void ezQtCurve1DEditorWidget::onDeleteCPs()
{
  QList<QGraphicsItem*> selection = m_Scene.selectedItems();

  if (!selection.isEmpty())
  {
    m_Scene.blockSignals(true);
    emit BeginCpChanges();

    ezHybridArray<PtToDelete, 16> delOrder;

    for (QGraphicsItem* pItem : selection)
    {
      ezQCurveControlPoint* pCP = static_cast<ezQCurveControlPoint*>(pItem);

      if (pCP != nullptr)
      {
        auto& pt = delOrder.ExpandAndGetRef();
        pt.m_uiCurveIdx = pCP->m_uiCurveIdx;
        pt.m_uiPointIdx = pCP->m_uiControlPoint;
      }
    }

    delOrder.Sort();

    // delete sorted from back to front to prevent point indices becoming invalidated
    for (const auto& pt : delOrder)
    {
      emit CpDeleted(pt.m_uiCurveIdx, pt.m_uiPointIdx);
    }

    emit EndCpChanges();
    m_Scene.clearSelection();
    m_Scene.blockSignals(false);

    UpdateCpUi();
  }
}

void ezQtCurve1DEditorWidget::UpdateCpUi()
{
  if (m_Scene.signalsBlocked())
    return;

  m_Scene.blockSignals(true);

  ezQtScopedBlockSignals bs(this);
  ezQtScopedUpdatesDisabled ud(this);

  bool bClearSelection = false;

  const QPen penCP(QColor(200, 200, 0), 0);
  const QBrush brushCP(QColor(150, 150, 0));

  const QPen penPath(QColor(0, 200, 0), 0);
  const QBrush brushPath(Qt::NoBrush);

  for (ezUInt32 curveIdx = 0; curveIdx < m_Curves.GetCount(); ++curveIdx)
  {
    auto& data = m_Curves[curveIdx];
    auto& curve = data.m_Curve;

    curve.RecomputeExtents();

    for (ezUInt32 cpIdx = 0; cpIdx < curve.GetNumControlPoints(); ++cpIdx)
    {
      const auto& cp = curve.GetControlPoint(cpIdx);

      ezQCurveControlPoint* point = nullptr;
      ezQCurveTangentHandle* pTangentHandleLeft = nullptr;
      ezQCurveTangentHandle* pTangentHandleRight = nullptr;
      ezQCurveTangentLine* pTangentLineLeft = nullptr;
      ezQCurveTangentLine* pTangentLineRight = nullptr;

      if (cpIdx < data.m_ControlPoints.GetCount())
      {
        point = data.m_ControlPoints[cpIdx];
        pTangentHandleLeft = data.m_TangentHandlesLeft[cpIdx];
        pTangentHandleRight = data.m_TangentHandlesRight[cpIdx];
        pTangentLineLeft = data.m_TangentLinesLeft[cpIdx];
        pTangentLineRight = data.m_TangentLinesRight[cpIdx];
      }
      else
      {
        bClearSelection = true;

        point = new ezQCurveControlPoint();
        point->setRect(-5, -5, 10, 10);
        point->m_uiCurveIdx = curveIdx;
        point->m_pOwner = this;
        data.m_ControlPoints.PushBack(point);
        m_Scene.addItem(point);

        pTangentHandleLeft = new ezQCurveTangentHandle();
        pTangentHandleLeft->setRect(-5, -5, 10, 10);
        pTangentHandleLeft->m_uiCurveIdx = curveIdx;
        pTangentHandleLeft->m_pOwner = this;
        pTangentHandleLeft->m_bRightTangent = false;
        data.m_TangentHandlesLeft.PushBack(pTangentHandleLeft);
        m_Scene.addItem(pTangentHandleLeft);

        pTangentHandleRight = new ezQCurveTangentHandle();
        pTangentHandleRight->setRect(-5, -5, 10, 10);
        pTangentHandleRight->m_uiCurveIdx = curveIdx;
        pTangentHandleRight->m_pOwner = this;
        pTangentHandleRight->m_bRightTangent = true;
        data.m_TangentHandlesRight.PushBack(pTangentHandleRight);
        m_Scene.addItem(pTangentHandleRight);

        pTangentLineLeft = new ezQCurveTangentLine();
        pTangentLineLeft->m_uiCurveIdx = curveIdx;
        pTangentLineLeft->m_pOwner = this;
        pTangentLineLeft->m_bRightTangent = false;
        data.m_TangentLinesLeft.PushBack(pTangentLineLeft);
        m_Scene.addItem(pTangentLineLeft);

        pTangentLineRight = new ezQCurveTangentLine();
        pTangentLineRight->m_uiCurveIdx = curveIdx;
        pTangentLineRight->m_pOwner = this;
        pTangentLineRight->m_bRightTangent = true;
        data.m_TangentLinesRight.PushBack(pTangentLineRight);
        m_Scene.addItem(pTangentLineRight);
      }

      point->m_uiControlPoint = cpIdx;
      pTangentHandleLeft->m_uiControlPoint = cpIdx;
      pTangentHandleRight->m_uiControlPoint = cpIdx;
      pTangentLineLeft->m_uiControlPoint = cpIdx;
      pTangentLineRight->m_uiControlPoint = cpIdx;
      point->setPos(cp.m_Position.x, cp.m_Position.y);

      pTangentHandleLeft->setPos(cp.m_Position.x + cp.m_LeftTangent.x, cp.m_Position.y + cp.m_LeftTangent.y);
      pTangentHandleRight->setPos(cp.m_Position.x + cp.m_RightTangent.x, cp.m_Position.y + cp.m_RightTangent.y);
      pTangentLineLeft->setPos(cp.m_Position.x + cp.m_LeftTangent.x, cp.m_Position.y + cp.m_LeftTangent.y);
      pTangentLineRight->setPos(cp.m_Position.x + cp.m_RightTangent.x, cp.m_Position.y + cp.m_RightTangent.y);

      pTangentLineLeft->UpdateTangentLine();
      pTangentLineRight->UpdateTangentLine();

      const bool tangentsVisible = point->isSelected() || pTangentHandleLeft->isSelected() || pTangentHandleRight->isSelected();
      pTangentHandleLeft->setVisible(tangentsVisible);
      pTangentHandleRight->setVisible(tangentsVisible);
      pTangentLineLeft->setVisible(tangentsVisible);
      pTangentLineRight->setVisible(tangentsVisible);
    }

    while (data.m_ControlPoints.GetCount() > curve.GetNumControlPoints())
    {
      bClearSelection = true;

      delete data.m_ControlPoints.PeekBack();
      data.m_ControlPoints.PopBack();

      delete data.m_TangentHandlesLeft.PeekBack();
      data.m_TangentHandlesLeft.PopBack();

      delete data.m_TangentHandlesRight.PeekBack();
      data.m_TangentHandlesRight.PopBack();

      delete data.m_TangentLinesLeft.PeekBack();
      data.m_TangentLinesLeft.PopBack();

      delete data.m_TangentLinesRight.PeekBack();
      data.m_TangentLinesRight.PopBack();
    }

    for (ezUInt32 uiSegment = 0; uiSegment + 1 < curve.GetNumControlPoints(); ++uiSegment)
    {
      const auto& cp0 = curve.GetControlPoint(uiSegment);

      ezQCurveSegment* seg = nullptr;

      if (uiSegment < data.m_Segments.GetCount())
      {
        seg = data.m_Segments[uiSegment];
      }
      else
      {
        bClearSelection = true;

        seg = new ezQCurveSegment();
        seg->m_uiCurveIdx = curveIdx;
        seg->m_pOwner = this;

        data.m_Segments.PushBack(seg);
        m_Scene.addItem(seg);
      }

      seg->m_uiSegment = uiSegment;
      seg->UpdateSegment();
    }

    while (!data.m_Segments.IsEmpty() && data.m_Segments.GetCount() + 1 > curve.GetNumControlPoints())
    {
      bClearSelection = true;
      delete data.m_Segments.PeekBack();
      data.m_Segments.PopBack();
    }
  }

  if (bClearSelection)
    m_Scene.clearSelection();

  m_Scene.blockSignals(false);

  QRectF r = m_Scene.itemsBoundingRect();
  r.adjust(-5, -5, 5, 5);
  m_Scene.setSceneRect(r);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezQCurveControlPoint::ezQCurveControlPoint(QGraphicsItem* parent /*= nullptr*/)
  : QGraphicsEllipseItem(parent)
{
  m_pOwner = nullptr;
  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemIsSelectable);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges);
  setFlag(QGraphicsItem::ItemIgnoresTransformations);

  setPen(Qt::NoPen);
}

void ezQCurveControlPoint::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  auto palette = QApplication::palette();

  painter->setPen(pen());

  if (isSelected())
  {
    QColor col(255, 106, 0);
    painter->setBrush(QBrush(col));
  }
  else
  {
    QColor col(200, 200, 200);
    painter->setBrush(QBrush(col));
  }

  painter->drawEllipse(rect());
}

static ezSet<ControlPointMove> s_itemsChanged;
static ezSet<ControlPointMove> s_TangentsChanged;

static ezInt32 GetNumSelectedPoints(QList<QGraphicsItem*> selection, ezInt32 thisType)
{
  ezInt32 count = 0;

  for (ezInt32 i = 0; i < selection.size(); ++i)
  {
    if (selection[i]->type() == thisType)
      ++count;
  }

  return count;
}

QVariant ezQCurveControlPoint::itemChange(GraphicsItemChange change, const QVariant& value)
{
  if (m_pOwner != nullptr)
  {
    switch (change)
    {
    case QGraphicsItem::ItemSelectedChange:
    case QGraphicsItem::ItemSelectedHasChanged:
      s_itemsChanged.Clear();
      break;

    case QGraphicsItem::ItemPositionHasChanged:
      {
        // Working around a shitty interface

        const ezInt32 iNumCPs = GetNumSelectedPoints(scene()->selectedItems(), QGraphicsItem::UserType + 1);

        if ((int)s_itemsChanged.GetCount() < iNumCPs)
        {
          ControlPointMove move;
          move.curveIdx = m_uiCurveIdx;
          move.cpIdx = m_uiControlPoint;
          move.tangentIdx = 0;
          move.x = pos().x();
          move.y = pos().y();
          s_itemsChanged.Insert(move);


          if ((int)s_itemsChanged.GetCount() == iNumCPs)
          {
            m_pOwner->SetControlPoints(s_itemsChanged);

            s_itemsChanged.Clear();
          }
        }
      }
      break;
    }
  }

  return QGraphicsEllipseItem::itemChange(change, value);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezQCurveTangentHandle::ezQCurveTangentHandle(QGraphicsItem* parent /*= nullptr*/)
{
  m_pOwner = nullptr;
  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemIsSelectable);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges);
  setFlag(QGraphicsItem::ItemIgnoresTransformations);

  setZValue(-0.5);
}

void ezQCurveTangentHandle::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  const ezCurve1D& curve = m_pOwner->GetCurve1D(m_uiCurveIdx);
  const auto& cp = curve.GetControlPoint(m_uiControlPoint);

  float fMinX, fMaxX;
  curve.QueryExtents(fMinX, fMaxX);

  if (!m_bRightTangent && cp.m_Position.x == fMinX)
    return;
  if (m_bRightTangent && cp.m_Position.x == fMaxX)
    return;

  if (isSelected())
  {
    QColor col(0, 106, 255);
    painter->setBrush(QBrush(col));
  }
  else
  {
    QColor col(100, 100, 150);
    painter->setBrush(QBrush(col));
  }

  painter->setPen(Qt::NoPen);
  painter->drawRect(rect());
}

QVariant ezQCurveTangentHandle::itemChange(GraphicsItemChange change, const QVariant &value)
{
  if (m_pOwner != nullptr)
  {
    switch (change)
    {
    case QGraphicsItem::ItemSelectedChange:
    case QGraphicsItem::ItemSelectedHasChanged:
      s_TangentsChanged.Clear();
      break;

    case QGraphicsItem::ItemPositionHasChanged:
      {
        // Working around a shitty interface

        const ezInt32 iNumCPs = GetNumSelectedPoints(scene()->selectedItems(), QGraphicsItem::UserType + 1);
        const ezInt32 iNumTangents = GetNumSelectedPoints(scene()->selectedItems(), QGraphicsItem::UserType + 3);

        if (iNumCPs == 0 && (int)s_TangentsChanged.GetCount() < iNumTangents)
        {
          const ezCurve1D& curve = m_pOwner->GetCurve1D(m_uiCurveIdx);
          const auto& cp = curve.GetControlPoint(m_uiControlPoint);

          ControlPointMove move;
          move.curveIdx = m_uiCurveIdx;
          move.cpIdx = m_uiControlPoint;
          move.tangentIdx = m_bRightTangent ? 2 : 1;
          move.x = pos().x() - cp.m_Position.x;
          move.y = pos().y() - cp.m_Position.y;
          s_TangentsChanged.Insert(move);


          if ((int)s_TangentsChanged.GetCount() == iNumTangents)
          {
            m_pOwner->SetControlPoints(s_TangentsChanged);

            s_TangentsChanged.Clear();
          }
        }
      }
      break;
    }
  }

  return QGraphicsEllipseItem::itemChange(change, value);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezQCurveTangentLine::ezQCurveTangentLine(QGraphicsItem* parent /*= nullptr*/)
{
  setFlag(QGraphicsItem::ItemIsMovable, false);
  setFlag(QGraphicsItem::ItemIsSelectable, false);
  
  QColor col(160, 160, 160);

  QPen pen(QColor(50, 50, 100), 2.0f, Qt::DashLine);
  pen.setCosmetic(true);
  setPen(pen);
  setBrush(Qt::NoBrush);

  setZValue(-0.75);
}

void ezQCurveTangentLine::UpdateTangentLine()
{
  const ezCurve1D& curve = m_pOwner->GetCurve1D(m_uiCurveIdx);
  const auto& cp = curve.GetControlPoint(m_uiControlPoint);

  float fMinX, fMaxX;
  curve.QueryExtents(fMinX, fMaxX);

  QPainterPath p;

  if ((!m_bRightTangent && cp.m_Position.x != fMinX) ||
    (m_bRightTangent && cp.m_Position.x != fMaxX))
  {
    p.moveTo(0, 0);

    if (m_bRightTangent)
      p.lineTo(-QPointF(cp.m_RightTangent.x, cp.m_RightTangent.y));
    else
      p.lineTo(-QPointF(cp.m_LeftTangent.x, cp.m_LeftTangent.y));
  }
    
  setPath(p);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezQCurveSegment::ezQCurveSegment(QGraphicsItem* parent /*= nullptr*/)
  : QGraphicsPathItem(parent)
{
  setFlag(QGraphicsItem::ItemIsMovable, false);
  setFlag(QGraphicsItem::ItemIsSelectable, false);

  setZValue(-1);
}

void ezQCurveSegment::UpdateSegment()
{
  prepareGeometryChange();

  QPainterPath p;

  ezCurve1D curve = m_pOwner->GetCurve1D(m_uiCurveIdx);
  curve.SortControlPoints();
  curve.ClampTangents();
  //curve.MakeFixedLengthTangents();

  {
    ezColorGammaUB color = curve.GetCurveColor();
    QColor col(color.r, color.g, color.b);

    QPen pen(col, 1.0f, Qt::SolidLine);
    pen.setCosmetic(true);
    setPen(pen);
  }

  const auto& cp0 = curve.GetControlPoint(m_uiSegment);
  const auto& cp1 = curve.GetControlPoint(m_uiSegment + 1);

  const ezVec2 t0 = ezVec2(cp0.m_Position.x, cp0.m_Position.y) + cp0.m_RightTangent;
  const ezVec2 t1 = ezVec2(cp1.m_Position.x, cp1.m_Position.y) + cp1.m_LeftTangent;

  p.moveTo(cp0.m_Position.x, cp0.m_Position.y);
  p.cubicTo(QPointF(t0.x, t0.y), QPointF(t1.x, t1.y), QPointF(cp1.m_Position.x, cp1.m_Position.y));

  setPath(p);
}
