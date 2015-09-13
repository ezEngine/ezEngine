#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <QStyleOptionToolButton>
#include <QStyle>
#include <QPainter>
#include <QMouseEvent>
#include <QScrollArea>
#include <QRect>
#include <QRectF>

ezCollapsibleGroupBox::ezCollapsibleGroupBox(QWidget* pParent) : QWidget(pParent), m_bCollapsed(false)
{
  setupUi(this);

  Icon->installEventFilter(this);
  Caption->installEventFilter(this);

  m_FillColor;
}

void ezCollapsibleGroupBox::setTitle(QString sTitle)
{
  Caption->setText(sTitle);
}

QString ezCollapsibleGroupBox::title() const
{
  return Caption->text();
}

void ezCollapsibleGroupBox::SetFillColor(const QColor& color)
{
  m_FillColor = color;
  update();
}

void ezCollapsibleGroupBox::SetCollapseState(bool bCollapsed)
{
  if (bCollapsed == m_bCollapsed)
    return;

  QtScopedUpdatesDisabled sud(this);

  m_bCollapsed = bCollapsed;
  Content->setVisible(!bCollapsed);

  // Force re-layout of parent hierarchy to prevent flicker.
  QWidget* pCur = this;
  while (pCur != nullptr && qobject_cast<QScrollArea*>(pCur) == nullptr)
  {
    pCur->updateGeometry();
    pCur = pCur->parentWidget();
  }

  Icon->setPixmap(QPixmap(QLatin1String(Content->isVisible() ? ":/GuiFoundation/Icons/groupOpen.png" : ":/GuiFoundation/Icons/groupClosed.png")));

  emit CollapseStateChanged(bCollapsed);
}

bool ezCollapsibleGroupBox::GetCollapseState() const
{
  return m_bCollapsed;
}

bool ezCollapsibleGroupBox::eventFilter(QObject* object, QEvent* event)
{
  if (event->type() == QEvent::Type::MouseButtonPress || event->type() == QEvent::Type::MouseButtonDblClick)
  {
    QMouseEvent* pMouseEvent = static_cast<QMouseEvent*>(event);

    if (pMouseEvent->button() == Qt::MouseButton::LeftButton)
    {
      SetCollapseState(!m_bCollapsed);
      return true;
    }
  }

  return false;
}

void ezCollapsibleGroupBox::paintEvent(QPaintEvent* event)
{
  const QPalette& pal = palette();
  QWidget::paintEvent(event);

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  ezInt32 iRounding = 4;
  QRect wr = contentsRect();

  QRect hr = Header->contentsRect();
  hr.moveTopLeft(Header->pos());

  QRect cr = wr;
  cr.setTop(hr.height());
  cr.adjust(2, 0, 0, -2);

  if (m_FillColor.isValid())
  {
    QRectF wrAdjusted = wr;
    wrAdjusted.adjust(0.5, 0.5, iRounding, -0.5);
    QPainterPath oPath;
    oPath.addRoundedRect(wrAdjusted, iRounding, iRounding);
    p.fillPath(oPath, pal.alternateBase());
   
    QRectF crAdjusted = cr;
    crAdjusted.adjust(0.5, 0.5, iRounding, -0.5);   
    QPainterPath path;
    path.addRoundedRect(crAdjusted, iRounding, iRounding);
    p.fillPath(path, pal.window());
  }
}
