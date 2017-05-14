#include <PCH.h>
#include <GuiFoundation/Widgets/GroupBoxBase.moc.h>
#include <QStyleOptionToolButton>
#include <QPainter>
#include <QBoxLayout>

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

void ezQtGroupBoxBase::DrawHeader(QPainter& p, const QRect& rect, const QString& sTitle, const QIcon& icon, bool bCollapsible)
{
  QRect remainingRect = rect.adjusted(0, 0, 0, 0);

  if (bCollapsible)
  {
    QRect iconRect = remainingRect;
    iconRect.setWidth(iconRect.height() / 2);
    bool bCollapsed = GetCollapseState();
    QIcon collapseIcon = bCollapsed ? ezQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/groupClosed.png") : ezQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/groupOpen.png");
    collapseIcon.paint(&p, iconRect);
    remainingRect.adjust(iconRect.width() + Spacing, 0, 0, 0);
  }

  if (!icon.isNull())
  {
    QRect iconRect = remainingRect;
    iconRect.setWidth(iconRect.height());
    icon.paint(&p, iconRect);
    remainingRect.adjust(iconRect.width() + Spacing, 0, 0, 0);
  }

  QStyle* style = QWidget::style();
  int flags = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextExpandTabs | Qt::TextForceLeftToRight;
  style->drawItemText(&p, remainingRect, flags, palette(), isEnabled(), sTitle, foregroundRole());
}
