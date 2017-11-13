#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Time/Time.h>

#include <QWidget>
#include <QToolBar>

class QMouseEvent;
class QPushButton;

class EZ_GUIFOUNDATION_DLL ezQtTimeScrubberWidget : public QWidget
{
  Q_OBJECT

public:
  explicit ezQtTimeScrubberWidget(QWidget* pParent);
  ~ezQtTimeScrubberWidget();

  void SetDuration(ezUInt64 uiNumTicks, ezUInt32 uiFramesPerSecond);
  void SetScrubberPosition(ezUInt64 uiTick);

signals:
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
  explicit ezQtTimeScrubberToolbar(QWidget* parent);

  void SetDuration(ezUInt64 iNumTicks, ezUInt32 uiFramesPerSecond);
  void SetScrubberPosition(ezUInt64 uiTick);
  void SetButtonState(bool playing, bool repeatEnabled);

signals:
  void ScrubberPosChangedEvent(ezUInt64 uiNewScrubberTickPos);
  void PlayPauseEvent();
  void RepeatEvent();

private:
  ezQtTimeScrubberWidget* m_pScrubber = nullptr;
  QPushButton* m_pPlayButton = nullptr;
  QPushButton* m_pRepeatButton = nullptr;
};
