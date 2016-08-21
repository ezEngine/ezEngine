#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Foundation/Math/Color8UNorm.h>
#include <QGraphicsItem>
#include <QPainterPath>

ezInt32 ezQCurveControlPoint::s_iMovedCps = 0;

QCurve1DEditorWidget::QCurve1DEditorWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setupUi(this);

  m_Scene.setItemIndexMethod(QGraphicsScene::NoIndex);
  GraphicsView->setScene(&m_Scene);

  connect(GraphicsView, &ezQGraphicsView::BeginDrag, this, [this]() { emit BeginOperation(); });
  connect(GraphicsView, &ezQGraphicsView::EndDrag, this, [this]() { emit EndOperation(true); });
  connect(GraphicsView, &ezQGraphicsView::DeleteCPs, this, &QCurve1DEditorWidget::onDeleteCPs);
}


QCurve1DEditorWidget::~QCurve1DEditorWidget()
{

}

void QCurve1DEditorWidget::SetNumCurves(ezUInt32 num)
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

void QCurve1DEditorWidget::SetCurve1D(ezUInt32 idx, const ezCurve1D& curve)
{
  QtScopedUpdatesDisabled ud(this);
  QtScopedBlockSignals bs(this);

  m_Curves[idx].m_Curve = curve;

  UpdateCpUi();
}


void QCurve1DEditorWidget::FrameCurve()
{

  //curveWidget->update();
}


void QCurve1DEditorWidget::SetControlPoints(const ezSet<ControlPointMove>& moves)
{
  if (m_Scene.signalsBlocked())
    return;

  m_Scene.blockSignals(true);
  emit BeginCpChanges();

  for (const auto& m : moves)
  {
    emit CpMoved(m.curveIdx, m.cpIdx, m.x, m.y);
  }

  emit EndCpChanges();
  m_Scene.blockSignals(false);

  UpdateCpUi();
}

void QCurve1DEditorWidget::on_ButtonFrame_clicked()
{
  FrameCurve();
}

//void QCurve1DEditorWidget::on_curveWidget_selectionChanged(ezInt32 colorCP, ezInt32 alphaCP, ezInt32 intensityCP)
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


void QCurve1DEditorWidget::on_SpinPosition_valueChanged(double value)
{
  //if (m_iSelectedColorCP != -1)
  //{
  //  emit ColorCpMoved(m_iSelectedColorCP, value);
  //}
}

void QCurve1DEditorWidget::on_SpinValue_valueChanged(double value)
{
  //if (m_iSelectedIntensityCP != -1)
  //{
  //  emit IntensityCpChanged(m_iSelectedIntensityCP, value);
  //}
}


void QCurve1DEditorWidget::on_ButtonNormalize_clicked()
{
  //emit NormalizeRange();
}


void QCurve1DEditorWidget::onDeleteCPs()
{
  QList<QGraphicsItem*> selection = m_Scene.selectedItems();

  if (!selection.isEmpty())
  {
    m_Scene.blockSignals(true);
    emit BeginCpChanges();

    for (QGraphicsItem* pItem : selection)
    {
      ezQCurveControlPoint* pCP = static_cast<ezQCurveControlPoint*>(pItem);

      emit CpDeleted(pCP->m_uiCurveIdx, pCP->m_uiControlPoint);
    }

    emit EndCpChanges();
    m_Scene.clearSelection();
    m_Scene.blockSignals(false);

    UpdateCpUi();
  }
}

void QCurve1DEditorWidget::UpdateCpUi()
{
  if (m_Scene.signalsBlocked())
    return;

  m_Scene.blockSignals(true);

  QtScopedBlockSignals bs(this);
  QtScopedUpdatesDisabled ud(this);

  bool bClearSelection = false;

  const QPen penCP(QColor(200, 200, 0), 0);
  const QBrush brushCP(QColor(150, 150, 0));

  const QPen penPath(QColor(0, 200, 0), 0);
  const QBrush brushPath(Qt::NoBrush);

  for (ezUInt32 curveIdx = 0; curveIdx < m_Curves.GetCount(); ++curveIdx)
  {
    auto& data = m_Curves[curveIdx];
    const auto& curve = data.m_Curve;

    for (ezUInt32 cpIdx = 0; cpIdx < curve.GetNumControlPoints(); ++cpIdx)
    {
      const auto& cp = curve.GetControlPoint(cpIdx);

      ezQCurveControlPoint* point = nullptr;

      if (cpIdx < data.m_ControlPoints.GetCount())
      {
        point = data.m_ControlPoints[cpIdx];
      }
      else
      {
        bClearSelection = true;

        point = new ezQCurveControlPoint();
        point->setRect(-0.1f, -0.1f, 0.2f, 0.2f);
        point->m_uiCurveIdx = curveIdx;
        point->m_pOwner = this;

        data.m_ControlPoints.PushBack(point);
        m_Scene.addItem(point);
      }

      point->m_uiControlPoint = cpIdx;
      point->setPos(cp.m_fPosX, cp.m_fValue);
    }

    while (data.m_ControlPoints.GetCount() > curve.GetNumControlPoints())
    {
      bClearSelection = true;
      delete data.m_ControlPoints.PeekBack();
      data.m_ControlPoints.PopBack();
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

        if ((int)s_itemsChanged.GetCount() < scene()->selectedItems().size())
        {
          ControlPointMove move;
          move.curveIdx = m_uiCurveIdx;
          move.cpIdx = m_uiControlPoint;
          move.x = pos().x();
          move.y = pos().y();
          s_itemsChanged.Insert(move);


          if ((int)s_itemsChanged.GetCount() == scene()->selectedItems().size())
          {
            m_pOwner->SetControlPoints(s_itemsChanged);

            s_itemsChanged.Clear();
          }
        }
      }
      break;

    default:
      return QGraphicsEllipseItem::itemChange(change, value);
    }
  }

  return QGraphicsEllipseItem::itemChange(change, value);
}

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

  const auto& cp0 = curve.GetControlPoint(m_uiSegment);
  const auto& cp1 = curve.GetControlPoint(m_uiSegment + 1);

  p.moveTo(cp0.m_fPosX, cp0.m_fValue);

  p.cubicTo(QPointF(cp0.m_fPosX + 1.0f, cp0.m_fValue), QPointF(cp1.m_fPosX - 1.0f, cp1.m_fValue), QPointF(cp1.m_fPosX, cp1.m_fValue));

  //QPointF ctr1 = m_OutPoint + m_OutDir * (fDotOut * 0.5f);
  //QPointF ctr2 = m_InPoint + m_InDir * (fDotIn * 0.5f);

  //p.cubicTo(ctr1, ctr2, m_InPoint);

  setPath(p);
}
