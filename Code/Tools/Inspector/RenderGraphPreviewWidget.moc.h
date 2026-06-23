#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Vec2.h>
#include <QImage>
#include <QPoint>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;
class QWheelEvent;

/// Handles viewport interaction for the render graph texture preview.
class ezQtRenderGraphPreviewWidget : public QWidget
{
  Q_OBJECT

public:
  explicit ezQtRenderGraphPreviewWidget(QWidget* pParent = nullptr);

  void SetTextureSize(ezVec2U32 vSize);
  void SetTargetSize(ezVec2U32 vSize);
  void SetView(float fZoom, ezVec2 vPanCenter);

Q_SIGNALS:
  void RequestChanged(float fZoom, ezVec2 vPanCenter, ezVec2I32 vPixel, bool bUpdatePixelPosition, bool bHighlightPixel);

protected:
  void paintEvent(QPaintEvent*) override;
  void wheelEvent(QWheelEvent* e) override;
  void mousePressEvent(QMouseEvent* e) override;
  void mouseMoveEvent(QMouseEvent* e) override;
  void mouseReleaseEvent(QMouseEvent* e) override;

private:
  ezVec2 GetUvExtents() const;
  ezVec2 GetUvAtWidgetPosition(const QPoint& pos) const;
  QRect GetPreviewRect() const;
  void EmitChange(const QPoint& pos);

  ezVec2U32 m_vTextureSize = ezVec2U32(0, 0);
  ezVec2U32 m_vTargetSize = ezVec2U32(0, 0);
  float m_fZoom = 1.0f;
  ezVec2 m_vPanCenter = ezVec2(0.5f);
  bool m_bDragging = false;
  bool m_bPixelSelectionActive = false;
  QPoint m_LastMousePos;
  QImage m_CheckerboardImage;
};
