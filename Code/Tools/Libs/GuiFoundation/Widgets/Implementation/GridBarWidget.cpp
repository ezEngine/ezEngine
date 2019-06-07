#include <GuiFoundationPCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Strings/StringBuilder.h>
#include <GuiFoundation/Widgets/GridBarWidget.moc.h>
#include <GuiFoundation/Widgets/WidgetUtils.h>
#include <QPainter>
#include <QTextOption>
#include <qevent.h>

ezQGridBarWidget::ezQGridBarWidget(QWidget* parent)
    : QWidget(parent)
{
  m_viewportSceneRect.setRect(0, 1, 1, 1);
  m_fFineGridStops = 10;
  m_fTextGridStops = 100;
}

void ezQGridBarWidget::SetConfig(const QRectF& viewportSceneRect, double fTextGridStops, double fFineGridStops,
                                 ezDelegate<QPoint(const QPointF&)> mapFromSceneFunc)
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
  painter->setRenderHint(QPainter::Antialiasing);
  painter->setRenderHint(QPainter::TextAntialiasing);
  painter->setRenderHint(QPainter::HighQualityAntialiasing);

  QRect areaRect = rect();

  // background
  painter->fillRect(areaRect, palette().button());
  painter->translate(0.5, 0.5);

  // render fine grid stop lines
  {
    double fSceneMinX, fSceneMaxX;
    ezWidgetUtils::ComputeGridExtentsX(m_viewportSceneRect, m_fFineGridStops, fSceneMinX, fSceneMaxX);
    fSceneMinX = ezMath::Max(fSceneMinX, 0.0);

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
    ezWidgetUtils::ComputeGridExtentsX(m_viewportSceneRect, m_fTextGridStops, fSceneMinX, fSceneMaxX);
    fSceneMinX = ezMath::Max(fSceneMinX, 0.0);

    QTextOption textOpt(Qt::AlignCenter);
    QRectF textRect;

    painter->setPen(palette().buttonText().color());

    ezStringBuilder tmp;

    for (double x = fSceneMinX; x <= fSceneMaxX; x += m_fTextGridStops)
    {
      const QPoint pos = MapFromSceneFunc(QPointF(x, 0));

      textRect.setRect(pos.x() - 50, areaRect.top(), 99, areaRect.height());
      tmp.Format("{0}", ezArgF(x));

      painter->drawText(textRect, tmp.GetData(), textOpt);
    }
  }
}
