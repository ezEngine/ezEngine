#include <PCH.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Foundation/Math/Color8UNorm.h>
#include <QGraphicsItem>
#include <QPainterPath>

ezQtCurve1DEditorWidget::ezQtCurve1DEditorWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setupUi(this);

  m_Scene.setItemIndexMethod(QGraphicsScene::NoIndex);
  GraphicsView->setScene(&m_Scene);

  connect(GraphicsView, &ezQtGraphicsView::BeginDrag, this, [this]() { emit BeginOperation(); });
  connect(GraphicsView, &ezQtGraphicsView::EndDrag, this, [this]() { emit EndOperation(true); });
  connect(GraphicsView, &ezQtGraphicsView::DeleteCPs, this, &ezQtCurve1DEditorWidget::onDeleteCPs);
}


ezQtCurve1DEditorWidget::~ezQtCurve1DEditorWidget()
{

}

void ezQtCurve1DEditorWidget::SetNumCurves(ezUInt32 num)
{
  if (num < m_Curves.GetCount())
  {
    m_Scene.blockSignals(true);
    m_Scene.clear();
    m_Curves.Clear();
    m_Scene.blockSignals(false);
  }

  m_Curves.SetCount(num);
}

void ezQtCurve1DEditorWidget::SetCurve1D(ezUInt32 idx, const ezCurve1D& curve)
{
  ezQtScopedUpdatesDisabled ud(this);
  ezQtScopedBlockSignals bs(this);

  m_Curves[idx].m_Curve = curve;

  UpdateCpUi();
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

void ezQtCurve1DEditorWidget::on_SpinValue_valueChanged(double value)
{
  //if (m_iSelectedIntensityCP != -1)
  //{
  //  emit IntensityCpChanged(m_iSelectedIntensityCP, value);
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

void ezQtCurve1DEditorWidget::onDeleteCPs()
{
  QList<QGraphicsItem*> selection = m_Scene.selectedItems();

  if (!selection.isEmpty())
  {
    m_Scene.blockSignals(true);
    emit BeginCpChanges();

    for (QGraphicsItem* pItem : selection)
    {
      ezQCurveControlPoint* pCP = static_cast<ezQCurveControlPoint*>(pItem);

      if (pCP != nullptr)
      {
        emit CpDeleted(pCP->m_uiCurveIdx, pCP->m_uiControlPoint);
      }
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
      ezQCurveTangent* pTangentLeft = nullptr;
      ezQCurveTangent* pTangentRight = nullptr;

      if (cpIdx < data.m_ControlPoints.GetCount())
      {
        point = data.m_ControlPoints[cpIdx];
        pTangentLeft = data.m_TangentsLeft[cpIdx];
        pTangentRight = data.m_TangentsRight[cpIdx];
      }
      else
      {
        bClearSelection = true;

        point = new ezQCurveControlPoint();
        point->setRect(-0.15f, -0.15f, 0.3f, 0.3f);
        point->m_uiCurveIdx = curveIdx;
        point->m_pOwner = this;
        data.m_ControlPoints.PushBack(point);
        m_Scene.addItem(point);

        pTangentLeft = new ezQCurveTangent();
        pTangentLeft->setRect(-0.1f, -0.1f, 0.2f, 0.2f);
        pTangentLeft->m_uiCurveIdx = curveIdx;
        pTangentLeft->m_pOwner = this;
        pTangentLeft->m_bRightTangent = false;
        data.m_TangentsLeft.PushBack(pTangentLeft);
        m_Scene.addItem(pTangentLeft);

        pTangentRight = new ezQCurveTangent();
        pTangentRight->setRect(-0.1f, -0.1f, 0.2f, 0.2f);
        pTangentRight->m_uiCurveIdx = curveIdx;
        pTangentRight->m_pOwner = this;
        pTangentRight->m_bRightTangent = true;
        data.m_TangentsRight.PushBack(pTangentRight);
        m_Scene.addItem(pTangentRight);
      }

      point->m_uiControlPoint = cpIdx;
      pTangentLeft->m_uiControlPoint = cpIdx;
      pTangentRight->m_uiControlPoint = cpIdx;
      point->setPos(cp.m_Position.x, cp.m_Position.y);

      pTangentLeft->setPos(cp.m_Position.x + cp.m_LeftTangent.x, cp.m_Position.y + cp.m_LeftTangent.y);
      pTangentRight->setPos(cp.m_Position.x + cp.m_RightTangent.x, cp.m_Position.y + cp.m_RightTangent.y);
    }

    while (data.m_ControlPoints.GetCount() > curve.GetNumControlPoints())
    {
      bClearSelection = true;

      delete data.m_ControlPoints.PeekBack();
      data.m_ControlPoints.PopBack();

      delete data.m_TangentsLeft.PeekBack();
      data.m_TangentsLeft.PopBack();

      delete data.m_TangentsRight.PeekBack();
      data.m_TangentsRight.PopBack();
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

ezQCurveTangent::ezQCurveTangent(QGraphicsItem* parent /*= nullptr*/)
{
  m_pOwner = nullptr;
  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemIsSelectable);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges);

  setZValue(0.5);
}

void ezQCurveTangent::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  const ezCurve1D& curve = m_pOwner->GetCurve1D(m_uiCurveIdx);
  const auto& cp = curve.GetControlPoint(m_uiControlPoint);

  float fMinX, fMaxX;
  curve.QueryExtents(fMinX, fMaxX);

  if (!m_bRightTangent && cp.m_Position.x == fMinX)
    return;
  if (m_bRightTangent && cp.m_Position.x == fMaxX)
    return;

  auto palette = QApplication::palette();


  QPen pen(QColor(50, 50, 100), 0.05f, Qt::SolidLine);
  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);

  if (m_bRightTangent)
    painter->drawLine(QPointF(0, 0), -QPointF(cp.m_RightTangent.x, cp.m_RightTangent.y));
  else
    painter->drawLine(QPointF(0, 0), -QPointF(cp.m_LeftTangent.x, cp.m_LeftTangent.y));


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

QVariant ezQCurveTangent::itemChange(GraphicsItemChange change, const QVariant &value)
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

ezQCurveSegment::ezQCurveSegment(QGraphicsItem* parent /*= nullptr*/)
  : QGraphicsPathItem(parent)
{
  QColor col(160, 160, 160);

  QPen pen(col, 0.05f, Qt::SolidLine);
  setPen(pen);
  setBrush(Qt::NoBrush);

  setZValue(-1);
}

void ezQCurveSegment::UpdateSegment()
{
  prepareGeometryChange();

  QPainterPath p;

  ezCurve1D curve = m_pOwner->GetCurve1D(m_uiCurveIdx);
  curve.SortControlPoints();
  curve.ClampTangents();

  const auto& cp0 = curve.GetControlPoint(m_uiSegment);
  const auto& cp1 = curve.GetControlPoint(m_uiSegment + 1);

  const ezVec2 t0 = ezVec2(cp0.m_Position.x, cp0.m_Position.y) + cp0.m_RightTangent;
  const ezVec2 t1 = ezVec2(cp1.m_Position.x, cp1.m_Position.y) + cp1.m_LeftTangent;

  p.moveTo(cp0.m_Position.x, cp0.m_Position.y);

  p.cubicTo(QPointF(t0.x, t0.y), QPointF(t1.x, t1.y), QPointF(cp1.m_Position.x, cp1.m_Position.y));

  setPath(p);
}
