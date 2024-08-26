#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Strings/StringBuilder.h>
#include <GuiFoundation/Widgets/TimeScrubberWidget.moc.h>
#include <GuiFoundation/Widgets/WidgetUtils.h>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <qevent.h>

ezQtTimeScrubberWidget::ezQtTimeScrubberWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setMinimumWidth(100);
  setMinimumHeight(24);
  setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
}

ezQtTimeScrubberWidget::~ezQtTimeScrubberWidget() = default;

void ezQtTimeScrubberWidget::SetDuration(ezUInt64 uiNumTicks)
{
  if (m_uiDurationTicks == uiNumTicks)
    return;

  m_uiDurationTicks = uiNumTicks;
  m_Duration = ezTime::MakeFromSeconds((double)uiNumTicks / 4800.0);
  m_fNormScrubberPosition = ezMath::Clamp((double)m_uiScrubberTickPos / (double)m_uiDurationTicks, 0.0, 1.0);

  update();
}

void ezQtTimeScrubberWidget::SetDuration(ezTime time)
{
  SetDuration(static_cast<ezUInt64>(time.GetSeconds() * 4800));
}

void ezQtTimeScrubberWidget::SetScrubberPosition(ezUInt64 uiTick)
{
  if (m_uiScrubberTickPos == uiTick)
    return;

  m_uiScrubberTickPos = uiTick;
  m_fNormScrubberPosition = ezMath::Clamp((double)uiTick / (double)m_uiDurationTicks, 0.0, 1.0);

  update();
}

void ezQtTimeScrubberWidget::SetScrubberPosition(ezTime time)
{
  SetScrubberPosition(static_cast<ezUInt64>(time.GetSeconds() * 4800));
}

void ezQtTimeScrubberWidget::paintEvent(QPaintEvent* event)
{
  QWidget::paintEvent(event);

  if (m_uiDurationTicks == 0)
    return;

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  p.setRenderHint(QPainter::TextAntialiasing);
  p.fillRect(rect(), palette().light());
  p.translate(0.5, 0.5);

  QPen linePen;
  linePen.setCosmetic(true);

  const double fMaxDuration = m_Duration.GetSeconds();
  double fFineGridDensity = 0.01;
  double fRoughGridDensity = 0.01;
  ezWidgetUtils::AdjustGridDensity(fFineGridDensity, fRoughGridDensity, rect().width(), fMaxDuration, 20);
  ezHybridArray<QLine, 100> lines;

  // fine lines
  {
    lines.Clear();

    const int lineBottom = rect().bottom();
    const int lineTop = lineBottom - 6;
    const double scale = rect().width() / fMaxDuration;

    for (double x = 0.0; x < fMaxDuration; x += fFineGridDensity)
    {
      lines.PushBack(QLine(QPoint((int)(scale * x), lineTop), QPoint((int)(scale * x), lineBottom)));
    }

    linePen.setColor(palette().color(QPalette::ColorGroup::Disabled, QPalette::ColorRole::WindowText));
    linePen.setWidth(1);
    p.setPen(linePen);
    p.drawLines(lines.GetData(), lines.GetCount());
  }

  // rough lines
  {
    lines.Clear();

    const int lineBottom = rect().bottom();
    const int lineTop = lineBottom - 8;
    const double scale = rect().width() / fMaxDuration;

    for (double x = 0.0; x < fMaxDuration; x += fRoughGridDensity)
    {
      lines.PushBack(QLine(QPoint((int)(scale * x), lineTop), QPoint((int)(scale * x), lineBottom)));
    }

    linePen.setColor(palette().color(QPalette::ColorGroup::Active, QPalette::ColorRole::WindowText));
    linePen.setWidth(1);
    p.setPen(linePen);
    p.drawLines(lines.GetData(), lines.GetCount());
  }

  // rough stops text
  {
    QTextOption textOpt(Qt::AlignCenter);
    QRectF textRect;

    p.setPen(palette().buttonText().color());

    ezStringBuilder tmp;

    const double areaTop = rect().top();
    const double areaHeight = 14.0;
    const double scale = rect().width() / fMaxDuration;

    for (double x = fRoughGridDensity; x < fMaxDuration; x += fRoughGridDensity)
    {
      const double scaledX = x * scale;

      textRect.setRect(scaledX - 20, areaTop, 39, areaHeight);
      tmp.SetFormat("{0}", ezArgF(x));

      p.drawText(textRect, tmp.GetData(), textOpt);
    }
  }

  // Scrubber line
  {
    linePen.setColor(palette().highlight().color());
    linePen.setWidth(m_bDragging ? 3 : 1);
    p.setPen(linePen);

    const int posX = (int)(rect().width() * m_fNormScrubberPosition);
    p.drawLine(QLine(posX, 0, posX, rect().height()));
  }
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
    Q_EMIT ScrubberPosChangedEvent(uiTickPos);
  }
}

//////////////////////////////////////////////////////////////////////////

ezQtTimeScrubberToolbar::ezQtTimeScrubberToolbar(QWidget* pParent)
  : QToolBar("Time Scrubber", pParent)
{
  m_pScrubber = new ezQtTimeScrubberWidget(this);
  setObjectName("TimeScrubberToolbar");

  m_pPlayButton = new QPushButton(this);
  m_pPlayButton->setIcon(QIcon(":/GuiFoundation/Icons/ControlPlay.svg"));
  m_pPlayButton->setToolTip("Play Animation");

  m_pRepeatButton = new QPushButton(this);
  m_pRepeatButton->setIcon(QIcon(":/GuiFoundation/Icons/ControlRepeat.svg"));
  m_pRepeatButton->setCheckable(true);
  m_pRepeatButton->setToolTip("Repeat Animation");

  m_pDuration = new QLineEdit(this);
  m_pDuration->setMaximumWidth(50);
  m_pDuration->setToolTip("Duration in seconds");
  m_pDuration->setPlaceholderText("Duration (sec)");

  m_pAdjustDurationButton = new QPushButton(this);
  m_pAdjustDurationButton->setIcon(QIcon(":/GuiFoundation/Icons/Speed.svg"));
  m_pAdjustDurationButton->setToolTip("Adjust Duration");

  addWidget(m_pPlayButton);
  addWidget(m_pRepeatButton);
  addWidget(m_pScrubber);
  addWidget(m_pDuration);
  addWidget(m_pAdjustDurationButton);

  // Pass event through
  connect(m_pScrubber, &ezQtTimeScrubberWidget::ScrubberPosChangedEvent, this,
    [this](ezUInt64 uiNewScrubberTickPos)
    { Q_EMIT ScrubberPosChangedEvent(uiNewScrubberTickPos); });

  connect(m_pPlayButton, &QPushButton::clicked, this, [this](bool)
    { Q_EMIT PlayPauseEvent(); });
  connect(m_pRepeatButton, &QPushButton::clicked, this, [this](bool)
    { Q_EMIT RepeatEvent(); });
  connect(m_pDuration, &QLineEdit::textChanged, this, [this](const QString& sText)
    {
    bool ok = false;
    double val = sText.toDouble(&ok);

    if (ok)
      Q_EMIT DurationChangedEvent(val); });
  connect(m_pAdjustDurationButton, &QPushButton::clicked, this, [this](bool)
    { Q_EMIT AdjustDurationEvent(); });
}

void ezQtTimeScrubberToolbar::SetDuration(ezUInt64 uiNumTicks)
{
  m_pScrubber->SetDuration(uiNumTicks);

  ezQtScopedBlockSignals _1(m_pDuration);

  const double oldVal = m_pDuration->text().toDouble();
  const double newVal = (double)uiNumTicks / 4800.0;

  if (ezMath::IsEqual(oldVal, newVal, 0.01))
    return;

  m_pDuration->setText(QString("%1").arg(newVal, 0, 'f', 2));
}

void ezQtTimeScrubberToolbar::SetScrubberPosition(ezUInt64 uiTick)
{
  m_pScrubber->SetScrubberPosition(uiTick);
}

void ezQtTimeScrubberToolbar::SetButtonState(bool bPlaying, bool bRepeatEnabled)
{
  if (bPlaying)
    m_pPlayButton->setIcon(QIcon(":/GuiFoundation/Icons/ControlPause.svg"));
  else
    m_pPlayButton->setIcon(QIcon(":/GuiFoundation/Icons/ControlPlay.svg"));

  m_pRepeatButton->setChecked(bRepeatEnabled);
}
