#include <Inspector/InspectorPCH.h>

#include <Foundation/Math/ColorScheme.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <Inspector/RenderGraphHistogramWidget.moc.h>

#include <QImage>
#include <QPainter>
#include <QSizePolicy>
#include <QStyle>
#include <QStyleOptionFrame>

ezQtRenderGraphHistogramWidget::ezQtRenderGraphHistogramWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setFixedSize(260, 96);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  m_Colors[0] = QColor();
  m_Colors[EZ_BIT(0)] = ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Red));
  m_Colors[EZ_BIT(1)] = ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Green));
  m_Colors[EZ_BIT(2)] = ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Blue));
  m_Colors[EZ_BIT(0) | EZ_BIT(1)] = ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Yellow));
  m_Colors[EZ_BIT(0) | EZ_BIT(2)] = ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Violet));
  m_Colors[EZ_BIT(1) | EZ_BIT(2)] = ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Cyan));
  m_Colors[EZ_BIT(0) | EZ_BIT(1) | EZ_BIT(2)] = palette().color(QPalette::Text);
  m_Colors[EZ_BIT(3)] = palette().color(QPalette::Midlight);
  for (ezUInt32 i = 1; i < 8; ++i)
  {
    m_Colors[i + 8] = m_Colors[i].darker();
  }
}

void ezQtRenderGraphHistogramWidget::SetHistogram(ezArrayPtr<const ezUInt8> histogram)
{
  m_Histogram.SetCount(histogram.GetCount());
  for (ezUInt32 i = 0; i < histogram.GetCount(); ++i)
  {
    m_Histogram[i] = histogram[i];
  }

  if (!m_bRepaintPending)
  {
    m_bRepaintPending = true;
    update();
  }
}

void ezQtRenderGraphHistogramWidget::Clear()
{
  m_Histogram.Clear();
  if (!m_bRepaintPending)
  {
    m_bRepaintPending = true;
    update();
  }
}

void ezQtRenderGraphHistogramWidget::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, false);

  QStyleOptionFrame option;
  option.initFrom(this);
  option.rect = rect();
  option.lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, this);
  option.midLineWidth = 0;
  option.state |= QStyle::State_Sunken;

  style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter, this);

  const QRect plotRect = rect().adjusted(option.lineWidth, option.lineWidth, -option.lineWidth, -option.lineWidth);
  painter.fillRect(plotRect, palette().color(QPalette::Dark));

  if (m_bRepaintPending)
  {
    m_bRepaintPending = false;
    UpdateHistogram(plotRect);
  }
  // TODO: DPI scaling?
  painter.drawImage(plotRect.adjusted(2, 2, -2, -2), m_HistogramImage);
}

void ezQtRenderGraphHistogramWidget::UpdateHistogram(const QRect plotRect)
{
  if (m_HistogramImage.isNull())
  {
    m_HistogramImage = QImage(256, plotRect.height() - 4, QImage::Format_ARGB32_Premultiplied);
  }
  m_HistogramImage.fill(Qt::transparent);
  if (m_Histogram.IsEmpty())
    return;



  const ezUInt32 uiImageHeight = (ezUInt32)m_HistogramImage.height();
  const ezUInt32 uiMaxImageHeight = uiImageHeight - 1u;
  for (ezUInt32 bin = 0; bin < 256; ++bin)
  {
    // TODO: Interleave rgba?
    const ezUInt32 uiR = m_Histogram[bin];
    const ezUInt32 uiG = m_Histogram[256 + bin];
    const ezUInt32 uiB = m_Histogram[512 + bin];
    const ezUInt32 uiA = m_Histogram[768 + bin];
    const ezUInt32 uiRHeight = (uiR * uiMaxImageHeight + 254u) / 255u;
    const ezUInt32 uiGHeight = (uiG * uiMaxImageHeight + 254u) / 255u;
    const ezUInt32 uiBHeight = (uiB * uiMaxImageHeight + 254u) / 255u;
    const ezUInt32 uiAHeight = (uiA * uiMaxImageHeight + 254u) / 255u;
    const ezUInt32 uiMaxHeight = ezMath::Min(uiMaxImageHeight, ezMath::Max(uiRHeight, uiGHeight, uiBHeight, uiAHeight));
    for (ezUInt32 y = 0; y <= uiMaxHeight; ++y)
    {
      const bool bR = y <= uiRHeight;
      const bool bG = y <= uiGHeight;
      const bool bB = y <= uiBHeight;
      const bool bA = y <= uiAHeight;
      const ezUInt32 uiLookup = EZ_BIT(0) * bR | EZ_BIT(1) * bG | EZ_BIT(2) * bB | EZ_BIT(3) * bA;
      m_HistogramImage.setPixelColor((ezInt32)bin, (ezInt32)uiImageHeight - (ezInt32)y - 1, m_Colors[uiLookup]);
    }
  }
}
