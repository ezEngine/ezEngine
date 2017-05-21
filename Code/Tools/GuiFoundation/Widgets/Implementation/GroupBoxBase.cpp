#include <PCH.h>
#include <GuiFoundation/Widgets/GroupBoxBase.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QStyleOptionToolButton>
#include <QPainter>
#include <QBoxLayout>
#include <QMimeData>
#include <QDrag>
#include <QMouseEvent>

ezQtGroupBoxBase::ezQtGroupBoxBase(QWidget* pParent, bool bCollapsible) : QWidget(pParent)
{
  m_bCollapsible = bCollapsible;
}

void ezQtGroupBoxBase::SetTitle(const char* szTitle)
{
  m_sTitle = szTitle;
}

QString ezQtGroupBoxBase::GetTitle() const
{
  return m_sTitle;
}


void ezQtGroupBoxBase::SetIcon(const QIcon& icon)
{
  m_Icon = icon;
}


QIcon ezQtGroupBoxBase::GetIcon() const
{
  return m_Icon;
}

void ezQtGroupBoxBase::SetFillColor(const QColor& color)
{
  m_FillColor = color;
  update();
}

QColor ezQtGroupBoxBase::GetFillColor() const
{
  return m_FillColor;
}

void ezQtGroupBoxBase::SetDraggable(bool bDraggable)
{
  m_bDraggable = bDraggable;
}

bool ezQtGroupBoxBase::IsDraggable() const
{
  return m_bDraggable;
}

void ezQtGroupBoxBase::DrawHeader(QPainter& p, const QRect& rect)
{
  QRect remainingRect = rect.adjusted(0, 0, 0, 0);

  if (m_bCollapsible)
  {
    QRect iconRect = remainingRect;
    iconRect.setWidth(iconRect.height() / 2);
    bool bCollapsed = GetCollapseState();
    QIcon collapseIcon = bCollapsed ? ezQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/groupClosed.png") : ezQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/groupOpen.png");
    collapseIcon.paint(&p, iconRect);
    remainingRect.adjust(iconRect.width() + Spacing, 0, 0, 0);
  }

  if (!m_Icon.isNull())
  {
    QRect iconRect = remainingRect;
    iconRect.setWidth(iconRect.height());
    m_Icon.paint(&p, iconRect);
    remainingRect.adjust(iconRect.width() + Spacing, 0, 0, 0);
  }

  QStyle* style = QWidget::style();
  int flags = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextExpandTabs | Qt::TextForceLeftToRight;
  style->drawItemText(&p, remainingRect, flags, palette(), isEnabled(), m_sTitle, foregroundRole());
}

void ezQtGroupBoxBase::HeaderMousePress(QMouseEvent* me)
{
  if (me->button() == Qt::MouseButton::LeftButton)
  {
    m_bDragging = false;
    me->accept();
  }
}

void ezQtGroupBoxBase::HeaderMouseMove(QMouseEvent* me)
{
  if (m_bDraggable)
  {
    me->accept();

    QMimeData* mimeData = new QMimeData;
    emit DragStarted(*mimeData);

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setHotSpot(me->pos());

    {
      QPixmap tempPixmap(QSize(width(), GetHeader()->height()));
      QPainter painter;
      painter.begin(&tempPixmap);
      painter.fillRect(tempPixmap.rect(), palette().alternateBase());
      DrawHeader(painter, GetHeader()->contentsRect());
      painter.end();
      drag->setPixmap(tempPixmap);
    }

    drag->exec(Qt::MoveAction);
  }
}

void ezQtGroupBoxBase::HeaderMouseRelease(QMouseEvent* me)
{
  if (me->button() == Qt::MouseButton::LeftButton)
  {
    if (!m_bDragging && m_bCollapsible)
    {
      SetCollapseState(!GetCollapseState());
    }
    me->accept();
    m_bDragging = false;
  }
}
