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

  void SetDuration(ezInt64 iNumTicks, ezUInt32 uiFramesPerSecond);

signals:

private:
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseReleaseEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent* event) override;

  ezInt64 m_iDurationTicks = 0;
  ezUInt32 m_uiFramesPerSecond = 60;
};

class EZ_GUIFOUNDATION_DLL ezQtTimeScrubberToolbar : public QToolBar
{
  Q_OBJECT

public:
  explicit ezQtTimeScrubberToolbar(QWidget* parent);

  void SetDuration(ezInt64 iNumTicks, ezUInt32 uiFramesPerSecond);

private:
  ezQtTimeScrubberWidget* m_pScrubber;
};
