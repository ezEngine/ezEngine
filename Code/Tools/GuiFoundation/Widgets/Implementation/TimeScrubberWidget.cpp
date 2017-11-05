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

void ezQtTimeScrubberWidget::SetDuration(ezInt64 iNumTicks, ezUInt32 uiFramesPerSecond)
{
  if (m_iDurationTicks == iNumTicks)
    return;

  m_iDurationTicks = iNumTicks;
  m_uiFramesPerSecond = uiFramesPerSecond;
  update();
}

void ezQtTimeScrubberWidget::paintEvent(QPaintEvent* event)
{
  QWidget::paintEvent(event);

  if (m_iDurationTicks == 0)
    return;

  QPainter p(this);
  p.fillRect(rect(), palette().highlight());
}

void ezQtTimeScrubberWidget::mousePressEvent(QMouseEvent* event)
{
  QWidget::mousePressEvent(event);

  if (m_iDurationTicks == 0)
    return;


}

void ezQtTimeScrubberWidget::mouseReleaseEvent(QMouseEvent* event)
{
  QWidget::mouseReleaseEvent(event);

  if (m_iDurationTicks == 0)
    return;

}

void ezQtTimeScrubberWidget::mouseMoveEvent(QMouseEvent* event)
{
  QWidget::mouseMoveEvent(event);

  if (m_iDurationTicks == 0)
    return;

}


//////////////////////////////////////////////////////////////////////////

ezQtTimeScrubberToolbar::ezQtTimeScrubberToolbar(QWidget* parent)
  : QToolBar("Time Scrubber", parent)
{
  m_pScrubber = new ezQtTimeScrubberWidget(this);
  setObjectName("TimeScrubberToolbar");

  addWidget(m_pScrubber);
}

void ezQtTimeScrubberToolbar::SetDuration(ezInt64 iNumTicks, ezUInt32 uiFramesPerSecond)
{
  m_pScrubber->SetDuration(iNumTicks, uiFramesPerSecond);
}
