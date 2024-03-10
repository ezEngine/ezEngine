#pragma once

#include <Foundation/Types/Delegate.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QWidget>

class QPaintEvent;

class EZ_GUIFOUNDATION_DLL ezQGridBarWidget : public QWidget
{
  Q_OBJECT

public:
  ezQGridBarWidget(QWidget* pParent);

  void SetConfig(const QRectF& viewportSceneRect, double fTextGridStops, double fFineGridStops, ezDelegate<QPointF(const QPointF&)> mapFromSceneFunc);

protected:
  virtual void paintEvent(QPaintEvent* event) override;

private:
  QRectF m_ViewportSceneRect;
  double m_fTextGridStops;
  double m_fFineGridStops;
  ezDelegate<QPointF(const QPointF&)> m_MapFromSceneFunc;
};
