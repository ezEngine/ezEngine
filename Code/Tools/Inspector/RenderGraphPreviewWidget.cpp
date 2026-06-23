#include <Inspector/InspectorPCH.h>

#include <GuiFoundation/GuiFoundationDLL.h>
#include <Inspector/RenderGraphPreviewWidget.moc.h>

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

ezQtRenderGraphPreviewWidget::ezQtRenderGraphPreviewWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setMinimumHeight(160);
  setMouseTracking(true);

  constexpr ezInt32 checker = 16;
  m_CheckerboardImage = QImage(checker * 2, checker * 2, QImage::Format_ARGB32_Premultiplied);
  m_CheckerboardImage.fill(palette().color(QPalette::Midlight));
  QPainter checkerPainter(&m_CheckerboardImage);
  checkerPainter.fillRect(QRect(0, 0, checker, checker), palette().color(QPalette::Light));
  checkerPainter.fillRect(QRect(checker, checker, checker, checker), palette().color(QPalette::Light));
}

void ezQtRenderGraphPreviewWidget::SetTextureSize(ezVec2U32 vSize)
{
  m_vTextureSize = vSize;
  update();
}

void ezQtRenderGraphPreviewWidget::SetTargetSize(ezVec2U32 vSize)
{
  m_vTargetSize = vSize;
  update();
}

void ezQtRenderGraphPreviewWidget::SetView(float fZoom, ezVec2 vPanCenter)
{
  m_fZoom = ezMath::Clamp(fZoom, 0.1f, 64.0f);
  m_vPanCenter = vPanCenter;
  update();
}

void ezQtRenderGraphPreviewWidget::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.fillRect(rect(), palette().color(QPalette::Dark));
  const QRect viewRect = GetPreviewRect();
  painter.fillRect(viewRect, palette().color(QPalette::Base));

  painter.save();
  painter.setBrushOrigin(viewRect.topLeft());
  painter.fillRect(viewRect, QBrush(m_CheckerboardImage));
  painter.restore();

  painter.setPen(QPen(palette().color(QPalette::Highlight), 2));
  painter.drawRect(viewRect);
  painter.setPen(palette().color(QPalette::Text));
  ezStringBuilder text;
  text.SetFormat("Remote preview target\nZoom: {}%%\nPan: {}, {}\nLeft Click to pan\nRight click to select pixel\nMouse wheel to zoom", (ezInt32)(m_fZoom * 100.0f), ezArgF(m_vPanCenter.x, 3), ezArgF(m_vPanCenter.y, 3));
  painter.drawText(viewRect.adjusted(8, 8, -8, -8), Qt::AlignTop | Qt::AlignLeft, ezMakeQString(text));
}

void ezQtRenderGraphPreviewWidget::wheelEvent(QWheelEvent* e)
{
  const QPoint pos = e->position().toPoint();
  const ezVec2 uvBeforeZoom = GetUvAtWidgetPosition(pos);
  const float fZoomFactor = e->angleDelta().y() > 0 ? 1.2f : 1.0f / 1.2f;
  m_fZoom = ezMath::Clamp(m_fZoom * fZoomFactor, 0.1f, 64.0f);
  const ezVec2 uvAfterZoom = GetUvAtWidgetPosition(pos);
  m_vPanCenter += uvBeforeZoom - uvAfterZoom;
  EmitChange(pos);
}

void ezQtRenderGraphPreviewWidget::mousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::LeftButton || e->button() == Qt::MiddleButton)
  {
    m_bDragging = true;
    m_LastMousePos = e->pos();
  }
  else if (e->button() == Qt::RightButton)
  {
    m_bPixelSelectionActive = true;
    EmitChange(e->pos());
  }
}

void ezQtRenderGraphPreviewWidget::mouseMoveEvent(QMouseEvent* e)
{
  if (m_bDragging)
  {
    const ezVec2 extents = GetUvExtents();
    const QRect viewRect = GetPreviewRect();
    const QPoint delta = e->pos() - m_LastMousePos;
    m_LastMousePos = e->pos();
    m_vPanCenter.x -= (float)delta.x() / ezMath::Max(viewRect.width(), 1) * extents.x;
    m_vPanCenter.y -= (float)delta.y() / ezMath::Max(viewRect.height(), 1) * extents.y;
  }
  EmitChange(e->pos());
}

void ezQtRenderGraphPreviewWidget::mouseReleaseEvent(QMouseEvent* e)
{
  if (e->button() == Qt::LeftButton || e->button() == Qt::MiddleButton)
  {
    m_bDragging = false;
  }
  else if (e->button() == Qt::RightButton)
  {
    m_bPixelSelectionActive = false;
    EmitChange(e->pos());
  }
}

ezVec2 ezQtRenderGraphPreviewWidget::GetUvExtents() const
{
  if (m_vTextureSize.x == 0 || m_vTextureSize.y == 0)
  {
    return ezVec2(1.0f);
  }
  const float fTexAspect = (float)m_vTextureSize.x / (float)ezMath::Max(m_vTextureSize.y, 1u);
  const QRect viewRect = GetPreviewRect();
  const float fViewAspect = (float)ezMath::Max(viewRect.width(), 1) / (float)ezMath::Max(viewRect.height(), 1);
  return fTexAspect > fViewAspect ? ezVec2(1.0f / m_fZoom, 1.0f / m_fZoom * (fTexAspect / fViewAspect)) : ezVec2(1.0f / m_fZoom * (fViewAspect / fTexAspect), 1.0f / m_fZoom);
}

ezVec2 ezQtRenderGraphPreviewWidget::GetUvAtWidgetPosition(const QPoint& pos) const
{
  const ezVec2 extents = GetUvExtents();
  const QRect viewRect = GetPreviewRect();
  const float fX = ezMath::Clamp((float)(pos.x() - viewRect.left()) / (float)ezMath::Max(viewRect.width(), 1), 0.0f, 1.0f);
  const float fY = ezMath::Clamp((float)(pos.y() - viewRect.top()) / (float)ezMath::Max(viewRect.height(), 1), 0.0f, 1.0f);
  return ezVec2(m_vPanCenter.x - extents.x * 0.5f + fX * extents.x, m_vPanCenter.y - extents.y * 0.5f + fY * extents.y);
}

QRect ezQtRenderGraphPreviewWidget::GetPreviewRect() const
{
  const QRect available = rect().adjusted(8, 8, -8, -8);
  if (m_vTargetSize.x == 0 || m_vTargetSize.y == 0 || available.width() <= 0 || available.height() <= 0)
    return available;

  const float fTargetAspect = (float)m_vTargetSize.x / (float)ezMath::Max(m_vTargetSize.y, 1u);
  const float fAvailableAspect = (float)available.width() / (float)ezMath::Max(available.height(), 1);

  if (fTargetAspect > fAvailableAspect)
  {
    const ezInt32 iHeight = ezMath::Max(1, (ezInt32)((float)available.width() / fTargetAspect));
    const ezInt32 iTop = available.top() + (available.height() - iHeight) / 2;
    return QRect(available.left(), iTop, available.width(), iHeight);
  }

  const ezInt32 iWidth = ezMath::Max(1, (ezInt32)((float)available.height() * fTargetAspect));
  const ezInt32 iLeft = available.left() + (available.width() - iWidth) / 2;
  return QRect(iLeft, available.top(), iWidth, available.height());
}

void ezQtRenderGraphPreviewWidget::EmitChange(const QPoint& pos)
{
  ezVec2I32 pixel(-1, -1);
  if (m_bPixelSelectionActive && m_vTextureSize.x > 0 && m_vTextureSize.y > 0)
  {
    const ezVec2 uv = GetUvAtWidgetPosition(pos);
    pixel.x = ezMath::Clamp((ezInt32)(uv.x * (float)m_vTextureSize.x), 0, (ezInt32)m_vTextureSize.x - 1);
    pixel.y = ezMath::Clamp((ezInt32)(uv.y * (float)m_vTextureSize.y), 0, (ezInt32)m_vTextureSize.y - 1);
  }
  update();
  Q_EMIT RequestChanged(m_fZoom, m_vPanCenter, pixel, m_bPixelSelectionActive, m_bPixelSelectionActive);
}
