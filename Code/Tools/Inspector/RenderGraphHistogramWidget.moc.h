#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/ArrayPtr.h>
#include <QWidget>

class QPaintEvent;

/// Displays the normalized RGBA histogram returned by the render graph observer.
class ezQtRenderGraphHistogramWidget : public QWidget
{
  Q_OBJECT

public:
  explicit ezQtRenderGraphHistogramWidget(QWidget* pParent = nullptr);

  void SetHistogram(ezArrayPtr<const ezUInt8> histogram);
  void Clear();
  void UpdateHistogram(QRect plotRect);

protected:
  void paintEvent(QPaintEvent*) override;

private:
  ezDynamicArray<ezUInt8> m_Histogram;
  bool m_bRepaintPending = false;
  QColor m_Colors[16];
  QImage m_HistogramImage;
};
