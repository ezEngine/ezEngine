#include <PCH.h>
#include <GuiFoundation/Widgets/TimeScrubberWidget.moc.h>
#include <QPainter>
#include <Foundation/Math/Color8UNorm.h>
#include <qevent.h>

ezQtTimeScrubberWidget::ezQtTimeScrubberWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setMinimumWidth(100);
  setMinimumHeight(24);
  setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
}

ezQtTimeScrubberWidget::~ezQtTimeScrubberWidget()
{
}

void ezQtTimeScrubberWidget::SetDuration(ezUInt64 uiNumTicks, ezUInt32 uiFramesPerSecond)
{
  if (m_uiDurationTicks == uiNumTicks)
    return;

  m_uiDurationTicks = uiNumTicks;
  m_Duration = ezTime::Seconds((double)uiNumTicks / 4800.0);
  m_fNormScrubberPosition = ezMath::Clamp((double)m_uiScrubberTickPos / (double)m_uiDurationTicks, 0.0, 1.0);

  update();
}

void ezQtTimeScrubberWidget::SetScrubberPosition(ezUInt64 uiTick)
{
  if (m_uiScrubberTickPos == uiTick)
    return;

  m_uiScrubberTickPos = uiTick;
  m_fNormScrubberPosition = ezMath::Clamp((double)uiTick / (double)m_uiDurationTicks, 0.0, 1.0);

  update();
}

void ezQtTimeScrubberWidget::paintEvent(QPaintEvent* event)
{
  QWidget::paintEvent(event);

  if (m_uiDurationTicks == 0)
    return;

  QPainter p(this);
  p.fillRect(rect(), palette().light());

  QPen linePen;
  linePen.setColor(palette().highlight().color());
  linePen.setCosmetic(true);
  linePen.setWidth(m_bDragging ? 3 : 1);
  p.setPen(linePen);

  const double posX = rect().width() * m_fNormScrubberPosition;
  p.drawLine(QLineF(posX, 0, posX, rect().height()));
}

void ezQtTimeScrubberWidget::mousePressEvent(QMouseEvent* event)
{
  QWidget::mousePressEvent(event);

  if (m_uiDurationTicks == 0)
    return;

  if (event->button() != Qt::LeftButton)
    return;

  m_bDragging = true;

  SetScrubberPosFromPixelCoord(event->pos().x());
  update();
}

void ezQtTimeScrubberWidget::mouseReleaseEvent(QMouseEvent* event)
{
  QWidget::mouseReleaseEvent(event);

  if (m_uiDurationTicks == 0)
    return;

  if (!m_bDragging || event->button() != Qt::LeftButton)
    return;

  m_bDragging = false;
  update();
}

void ezQtTimeScrubberWidget::mouseMoveEvent(QMouseEvent* event)
{
  QWidget::mouseMoveEvent(event);

  if (m_uiDurationTicks == 0)
    return;

  if (!m_bDragging)
    return;

  SetScrubberPosFromPixelCoord(event->pos().x());
}

void ezQtTimeScrubberWidget::SetScrubberPosFromPixelCoord(ezInt32 posX)
{
  double fNormPos = (double)posX / (double)rect().width();
  fNormPos = ezMath::Clamp(fNormPos, 0.0, 1.0);

  const ezUInt64 uiTickPos = (ezUInt64)(fNormPos * m_uiDurationTicks);

  if (uiTickPos != m_uiScrubberTickPos)
  {
    emit ScrubberPosChangedEvent(uiTickPos);
  }
}

//////////////////////////////////////////////////////////////////////////

ezQtTimeScrubberToolbar::ezQtTimeScrubberToolbar(QWidget* parent)
  : QToolBar("Time Scrubber", parent)
{
  m_pScrubber = new ezQtTimeScrubberWidget(this);
  setObjectName("TimeScrubberToolbar");

  addWidget(m_pScrubber);

  // Pass event through
  connect(m_pScrubber, &ezQtTimeScrubberWidget::ScrubberPosChangedEvent, this, [this](ezUInt64 uiNewScrubberTickPos) {emit ScrubberPosChangedEvent(uiNewScrubberTickPos); });
}

void ezQtTimeScrubberToolbar::SetDuration(ezUInt64 iNumTicks, ezUInt32 uiFramesPerSecond)
{
  m_pScrubber->SetDuration(iNumTicks, uiFramesPerSecond);
}

void ezQtTimeScrubberToolbar::SetScrubberPosition(ezUInt64 uiTick)
{
  m_pScrubber->SetScrubberPosition(uiTick);
}
