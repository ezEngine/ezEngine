#include <PCH.h>
#include <GuiFoundation/Widgets/GridBarWidget.moc.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Containers/HybridArray.h>
#include <qevent.h>
#include <QTextOption>
#include <QPainter>

static void ComputeGridExtentsX(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinX, double& out_fMaxX)
{
  out_fMinX = ezMath::Floor((double)viewportSceneRect.left(), fGridStops);
  out_fMaxX = ezMath::Ceil((double)viewportSceneRect.right(), fGridStops);
}

ezQGridBarWidget::ezQGridBarWidget(QWidget* parent)
  : QWidget(parent)
{
  m_viewportSceneRect.setRect(0, 1, 1, 1);
  m_fFineGridStops = 10;
  m_fTextGridStops = 100;
}


void ezQGridBarWidget::SetConfig(const QRectF& viewportSceneRect, double fTextGridStops, double fFineGridStops, ezDelegate<QPoint(const QPointF&)> mapFromSceneFunc)
{
  MapFromSceneFunc = mapFromSceneFunc;

  bool bUpdate = false;
  if (m_viewportSceneRect != viewportSceneRect)
  {
    m_viewportSceneRect = viewportSceneRect;
    bUpdate = true;
  }

  if (m_fTextGridStops != fTextGridStops)
  {
    m_fTextGridStops = fTextGridStops;
    bUpdate = true;
  }

  if (m_fFineGridStops != fFineGridStops)
  {
    m_fFineGridStops = fFineGridStops;
    bUpdate = true;
  }

  if (bUpdate)
  {
    update();
  }
}

void ezQGridBarWidget::paintEvent(QPaintEvent* e)
{
  if (!MapFromSceneFunc.IsValid())
  {
    QWidget::paintEvent(e);
    return;
  }

  QPainter Painter(this);
  QPainter* painter = &Painter;

  QRect areaRect = rect();

  // background
  painter->fillRect(areaRect, palette().button());

  // render fine grid stop lines
  {
    double fSceneMinX, fSceneMaxX;
    ComputeGridExtentsX(m_viewportSceneRect, m_fFineGridStops, fSceneMinX, fSceneMaxX);

    painter->setPen(palette().buttonText().color());

    ezHybridArray<QLine, 100> lines;

    // some overcompensation for the case that the GraphicsView displays a scrollbar at the side
    for (double x = fSceneMinX; x <= fSceneMaxX + m_fTextGridStops; x += m_fFineGridStops)
    {
      const QPoint pos = MapFromSceneFunc(QPointF(x, 0));

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(pos.x(), areaRect.bottom() - 3, pos.x(), areaRect.bottom());
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }

  // Grid Stop Value Text
  {
    double fSceneMinX, fSceneMaxX;
    ComputeGridExtentsX(m_viewportSceneRect, m_fTextGridStops, fSceneMinX, fSceneMaxX);

    QTextOption textOpt(Qt::AlignCenter);
    QRectF textRect;

    painter->setPen(palette().buttonText().color());

    for (double x = fSceneMinX; x <= fSceneMaxX; x += m_fTextGridStops)
    {
      const QPoint pos = MapFromSceneFunc(QPointF(x, 0));

      textRect.setRect(pos.x() - 50, areaRect.top(), 100, areaRect.height());
      painter->drawText(textRect, QString("%1").arg(x, 2), textOpt);
    }
  }
}

