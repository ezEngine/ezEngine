#pragma once

#include <GuiFoundation/Basics.h>

#include <QWidget>
#include <QToolBar>

class QMouseEvent;

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
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseReleaseEvent(QMouseEvent* event);
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

signals:
  void ScrubberPosChangedEvent(ezUInt64 uiNewScrubberTickPos);

private:
  ezQtTimeScrubberWidget* m_pScrubber;
};
