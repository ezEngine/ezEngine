#include <PCH.h>

#include <Foundation/Math/Color8UNorm.h>
#include <GuiFoundation/UIServices/ColorDlgWidgets.moc.h>
#include <QPainter>
#include <qevent.h>

ezQtColorAreaWidget::ezQtColorAreaWidget(QWidget* parent)
    : QWidget(parent)
{
  setAutoFillBackground(false);

  m_fHue = -1.0f;
}

void ezQtColorAreaWidget::SetHue(float hue)
{
  if (m_fHue == hue)
    return;

  m_fHue = hue;
  UpdateImage();
  update();
}

void ezQtColorAreaWidget::SetSaturation(float sat)
{
  if (m_fSaturation == sat)
    return;

  m_fSaturation = sat;
  update();
}

void ezQtColorAreaWidget::SetValue(float val)
{
  if (m_fValue == val)
    return;

  m_fValue = val;
  update();
}

void ezQtColorAreaWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::RenderHint::Antialiasing);
  painter.setRenderHint(QPainter::RenderHint::HighQualityAntialiasing);

  const QRect area = rect();

  painter.drawTiledPixmap(area, QPixmap::fromImage(m_Image));

  QPointF center;
  center.setX((int)(area.width() * m_fSaturation) + 0.5f);
  center.setY((int)(area.height() * (1.0f - m_fValue)) + 0.5f);

  const QColor col = qRgb(40, 40, 40);

  painter.setPen(col);
  painter.drawEllipse(center, 5.5f, 5.5f);
}

void ezQtColorAreaWidget::UpdateImage()
{
  const int width = rect().width();
  const int height = rect().height();

  m_Image = QImage(width, height, QImage::Format::Format_RGB32);


  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      ezColor c;
      c.SetHSV(m_fHue, (double)x / (width - 1), (double)y / (height - 1));

      ezColorGammaUB cg = c;
      m_Image.setPixel(x, (height - 1) - y, qRgb(cg.r, cg.g, cg.b));
    }
  }
}

void ezQtColorAreaWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons().testFlag(Qt::LeftButton))
  {
    const int width = rect().width();
    const int height = rect().height();

    QPoint coord = event->pos();
    const int sat = ezMath::Clamp(coord.x(), 0, width - 1);
    const int val = ezMath::Clamp((height - 1) - coord.y(), 0, height - 1);

    const double fsat = (double)sat / (width - 1);
    const double fval = (double)val / (height - 1);

    valueChanged(fsat, fval);
  }
}

void ezQtColorAreaWidget::mousePressEvent(QMouseEvent* event)
{
  mouseMoveEvent(event);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezQtColorRangeWidget::ezQtColorRangeWidget(QWidget* parent)
    : QWidget(parent)
{
  setAutoFillBackground(false);
}

void ezQtColorRangeWidget::SetHue(float hue)
{
  if (m_fHue == hue)
    return;

  m_fHue = hue;
  update();
}

void ezQtColorRangeWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::RenderHint::Antialiasing);
  painter.setRenderHint(QPainter::RenderHint::HighQualityAntialiasing);

  const QRect area = rect();

  if (area.width() != m_Image.width())
    UpdateImage();

  painter.drawTiledPixmap(area, QPixmap::fromImage(m_Image));

  const float pos = (int)(m_fHue / 359.0f * area.width()) + 0.5f;

  const float top = area.top() + 0.5f;
  const float bot = area.bottom() + 0.5f;
  const float len = 5.0f;
  const float wid = 2.0f;

  const QColor col = qRgb(80, 80, 80);

  painter.setPen(col);
  painter.setBrush(col);

  {
    QPainterPath path;
    path.moveTo(QPointF(pos - wid, top));
    path.lineTo(QPointF(pos, top + len));
    path.lineTo(QPointF(pos + wid, top));
    path.closeSubpath();

    painter.drawPath(path);
  }

  {
    QPainterPath path;
    path.moveTo(QPointF(pos - wid, bot));
    path.lineTo(QPointF(pos, bot - len));
    path.lineTo(QPointF(pos + wid, bot));
    path.closeSubpath();

    painter.drawPath(path);
  }
}

void ezQtColorRangeWidget::UpdateImage()
{
  const int width = rect().width();
  const int height = rect().height();

  m_Image = QImage(width, 1, QImage::Format::Format_RGB32);

  for (int x = 0; x < width; ++x)
  {
    ezColor c;
    c.SetHSV(((double)x / (width - 1.0)) * 360.0, 1, 1);

    ezColorGammaUB cg = c;
    m_Image.setPixel(x, 0, qRgb(cg.r, cg.g, cg.b));
  }
}

void ezQtColorRangeWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons().testFlag(Qt::LeftButton))
  {
    const int width = rect().width();
    const int height = rect().height();

    QPoint coord = event->pos();
    const int x = ezMath::Clamp(coord.x(), 0, width - 1);

    const double fx = (double)x / (width - 1);

    valueChanged(fx);
  }
}

void ezQtColorRangeWidget::mousePressEvent(QMouseEvent* event)
{
  mouseMoveEvent(event);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezQtColorCompareWidget::ezQtColorCompareWidget(QWidget* parent)
{
  setAutoFillBackground(false);
}

void ezQtColorCompareWidget::SetNewColor(const ezColor& color)
{
  if (m_NewColor == color)
    return;

  m_NewColor = color;
  update();
}

void ezQtColorCompareWidget::SetInitialColor(const ezColor& color)
{
  m_InitialColor = color;
  m_NewColor = color;
}

void ezQtColorCompareWidget::paintEvent(QPaintEvent*)
{
  const QRect area = rect();
  const QRect areaTop(area.left(), area.top(), area.width(), area.height() / 2);
  const QRect areaBot(area.left(), areaTop.bottom(), area.width(), area.height() - areaTop.height()); // rounding ...

  QPainter p(this);

  ezColor inLDR = m_InitialColor;
  float fMultiplier = m_InitialColor.ComputeHdrMultiplier();

  if (fMultiplier > 1.0f)
  {
    inLDR.ScaleRGB(1.0f / fMultiplier);
  }

  ezColorGammaUB inGamma = inLDR;
  QColor qInCol = qRgb(inGamma.r, inGamma.g, inGamma.b);

  ezColor newLDR = m_NewColor;
  fMultiplier = m_NewColor.ComputeHdrMultiplier();

  if (fMultiplier > 1.0f)
  {
    newLDR.ScaleRGB(1.0f / fMultiplier);
  }

  ezColorGammaUB newGamma = newLDR;
  QColor qNewCol = qRgb(newGamma.r, newGamma.g, newGamma.b);

  p.fillRect(areaTop, qInCol);
  p.fillRect(areaBot, qNewCol);
}
