#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Time/Time.h>

#include <QToolBar>
#include <QWidget>

class QMouseEvent;
class QPushButton;
class QLineEdit;

class EZ_GUIFOUNDATION_DLL ezQtTimeScrubberWidget : public QWidget
{
  Q_OBJECT

public:
  explicit ezQtTimeScrubberWidget(QWidget* pParent);
  ~ezQtTimeScrubberWidget();

  /// \brief Sets the duration in 'ticks'. There are 4800 ticks per second.
  void SetDuration(ezUInt64 uiNumTicks);

  /// \brief Sets the duration.
  void SetDuration(ezTime time);

  /// \brief Sets the current position in 'ticks'. There are 4800 ticks per second.
  void SetScrubberPosition(ezUInt64 uiTick);

  /// \brief Sets the current position.
  void SetScrubberPosition(ezTime time);

Q_SIGNALS:
  void ScrubberPosChangedEvent(ezUInt64 uiNewScrubberTickPos);

private:
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  void SetScrubberPosFromPixelCoord(ezInt32 x);

  ezUInt64 m_uiDurationTicks = 0;
  ezTime m_Duration;
  ezUInt64 m_uiScrubberTickPos = 0;
  double m_fNormScrubberPosition = 0.0;
  bool m_bDragging = false;
};

class EZ_GUIFOUNDATION_DLL ezQtTimeScrubberToolbar : public QToolBar
{
  Q_OBJECT

public:
  explicit ezQtTimeScrubberToolbar(QWidget* pParent);

  /// \brief Sets the duration in 'ticks'. There are 4800 ticks per second.
  void SetDuration(ezUInt64 uiNumTicks);

  /// \brief Sets the current position in 'ticks'. There are 4800 ticks per second.
  void SetScrubberPosition(ezUInt64 uiTick);

  void SetButtonState(bool bPlaying, bool bRepeatEnabled);

Q_SIGNALS:
  void ScrubberPosChangedEvent(ezUInt64 uiNewScrubberTickPos);
  void PlayPauseEvent();
  void RepeatEvent();
  void DurationChangedEvent(double fDuration);
  void AdjustDurationEvent();

private:
  ezQtTimeScrubberWidget* m_pScrubber = nullptr;
  QPushButton* m_pPlayButton = nullptr;
  QPushButton* m_pRepeatButton = nullptr;
  QLineEdit* m_pDuration = nullptr;
  QPushButton* m_pAdjustDurationButton = nullptr;
};
