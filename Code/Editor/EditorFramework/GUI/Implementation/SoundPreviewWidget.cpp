#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/SoundPreviewWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
  QString FormatTime(ezTime time)
  {
    const double fSeconds = ezMath::Max(time.GetSeconds(), 0.0);
    const int iMinutes = static_cast<int>(fSeconds / 60.0);

    return QString::asprintf("%d:%04.1f", iMinutes, fSeconds - iMinutes * 60.0);
  }
} // namespace

ezQtSoundPreviewWidget::ezQtSoundPreviewWidget(QWidget* pParent)
  : QWidget(pParent)
{
  m_pPlayButton = new QPushButton(this);
  m_pPlayButton->setIconSize(QSize(24, 24));
  m_pPlayButton->setMinimumWidth(100);

  m_pProgress = new QProgressBar(this);
  m_pProgress->setTextVisible(false);
  m_pProgress->setRange(0, 1000);
  m_pProgress->setValue(0);

  m_pTimeLabel = new QLabel(this);
  m_pTimeLabel->setMinimumWidth(100);
  m_pTimeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  m_pInfoLabel = new QLabel(this);
  m_pInfoLabel->setWordWrap(true);
  m_pInfoLabel->setTextFormat(Qt::PlainText);
  m_pInfoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

  m_pTimer = new QTimer(this);
  m_pTimer->setInterval(33);

  QHBoxLayout* pControls = new QHBoxLayout();
  pControls->addWidget(m_pPlayButton);
  pControls->addWidget(m_pProgress, 1);
  pControls->addWidget(m_pTimeLabel);

  QVBoxLayout* pLayout = new QVBoxLayout(this);
  pLayout->addLayout(pControls);
  pLayout->addWidget(m_pInfoLabel);
  pLayout->addStretch(1);

  connect(m_pPlayButton, &QPushButton::clicked, this, [this]()
    {
    if (m_bPlaying)
      Q_EMIT StopRequested();
    else
      Q_EMIT PlayRequested(); });

  connect(m_pTimer, &QTimer::timeout, this, &ezQtSoundPreviewWidget::UpdateRequested);

  UpdateButton();
}

void ezQtSoundPreviewWidget::SetInfoText(ezStringView sText)
{
  m_pInfoLabel->setText(ezMakeQString(sText));
}

void ezQtSoundPreviewWidget::SetCanPlay(bool bCanPlay)
{
  m_bCanPlay = bCanPlay;
  UpdateButton();
}

void ezQtSoundPreviewWidget::SetPlaybackState(bool bPlaying, ezTime position, ezTime length)
{
  const bool bStateChanged = m_bPlaying != bPlaying;
  m_bPlaying = bPlaying;

  if (bStateChanged)
  {
    UpdateButton();

    if (m_bPlaying)
      m_pTimer->start();
    else
      m_pTimer->stop();
  }

  if (!m_bPlaying)
  {
    m_pProgress->setRange(0, 1000);
    m_pProgress->setValue(0);
    m_pTimeLabel->clear();
    return;
  }

  if (length.IsPositive())
  {
    m_pProgress->setRange(0, 1000);
    m_pProgress->setValue(static_cast<int>(ezMath::Clamp(position.GetSeconds() / length.GetSeconds(), 0.0, 1.0) * 1000.0));
    m_pTimeLabel->setText(FormatTime(position) + " / " + FormatTime(length));
  }
  else
  {
    // an empty range makes Qt display a busy indicator
    m_pProgress->setRange(0, 0);
    m_pTimeLabel->setText(FormatTime(position));
  }
}

void ezQtSoundPreviewWidget::UpdateButton()
{
  m_pPlayButton->setText("");
  m_pPlayButton->setIcon(ezQtUiServices::GetCachedIconResource(m_bPlaying ? ":/EditorFramework/Icons/AssetProcessingForceStop.svg" : ":/EditorFramework/Icons/Play.svg"));
  m_pPlayButton->setEnabled(m_bPlaying || m_bCanPlay);
}
