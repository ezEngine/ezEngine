#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Types/Delegate.h>
#include <QWidget>

class QPaintEvent;

class EZ_GUIFOUNDATION_DLL ezQGridBarWidget : public QWidget
{
  Q_OBJECT

public:
  ezQGridBarWidget(QWidget* parent);

  void SetConfig(const QRectF& viewportSceneRect, double fTextGridStops, double fFineGridStops, ezDelegate<QPoint (const QPointF&)> mapFromSceneFunc);

protected:
  virtual void paintEvent(QPaintEvent* event) override;

private:
  QRectF m_viewportSceneRect;
  double m_fTextGridStops;
  double m_fFineGridStops;
  ezDelegate<QPoint(const QPointF&)> MapFromSceneFunc;
};
