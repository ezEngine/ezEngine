#include <PCH.h>
#include <GuiFoundation/Widgets/InlinedGroupBox.moc.h>
#include <QStyleOptionToolButton>
#include <QPainter>
#include <QBoxLayout>

ezQtInlinedGroupBox::ezQtInlinedGroupBox(QWidget* pParent) : ezQtGroupBoxBase(pParent)
{
  QHBoxLayout* pRootLayout = new QHBoxLayout(this);
  pRootLayout->setContentsMargins(0, 1, 0, 1);
  pRootLayout->setSpacing(0);

  m_pContent = new QWidget(this);
  QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Preferred);
  sp.setHorizontalStretch(2);
  sp.setVerticalStretch(0);
  m_pContent->setSizePolicy(sp);

  m_pHeader = new QFrame(this);
  QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
  sizePolicy1.setHorizontalStretch(0);
  sizePolicy1.setVerticalStretch(0);
  m_pHeader->setSizePolicy(sizePolicy1);

  QHBoxLayout* pHeaderLayout = new QHBoxLayout(m_pHeader);
  pHeaderLayout->setSpacing(0);
  pHeaderLayout->setContentsMargins(0, 0, 0, 0);

  pRootLayout->addSpacerItem(new QSpacerItem(0, 0));
  pRootLayout->setStretch(0, 1);
  pRootLayout->addWidget(m_pContent);
  pRootLayout->addWidget(m_pHeader);
  //Icon->installEventFilter(this);
  //Caption->installEventFilter(this);
}

void ezQtInlinedGroupBox::SetTitle(const char* szTitle)
{
  m_sTitle = szTitle;
  update();
}

QString ezQtInlinedGroupBox::GetTitle() const
{
  return m_sTitle;
}


void ezQtInlinedGroupBox::SetIcon(const QPixmap& icon)
{
  m_Icon = icon;
  update();
}

void ezQtInlinedGroupBox::SetFillColor(const QColor& color)
{
  m_FillColor = color;
  update();
}

QColor ezQtInlinedGroupBox::GetFillColor() const
{
  return m_FillColor;
}

void ezQtInlinedGroupBox::SetCollapseState(bool bCollapsed)
{
}

bool ezQtInlinedGroupBox::GetCollapseState() const
{
  return false;
}

QWidget* ezQtInlinedGroupBox::GetContent()
{
  return m_pContent;
}

QWidget* ezQtInlinedGroupBox::GetHeader()
{
  return m_pHeader;
}

void ezQtInlinedGroupBox::paintEvent(QPaintEvent* event)
{
  const QPalette& pal = palette();
  QWidget::paintEvent(event);

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  ezInt32 iRounding = 4;
  QRect wr = contentsRect();

  if (m_FillColor.isValid())
  {
    QRectF wrAdjusted = wr;
    wrAdjusted.adjust(0.5, 0.5, iRounding, -0.5);
    QPainterPath oPath;
    oPath.addRoundedRect(wrAdjusted, iRounding, iRounding);
    p.fillPath(oPath, pal.alternateBase());
  }

  QRect textRect = wr.adjusted(iRounding, 0, 0, 0);
  QStyle* style = QWidget::style();
  int flags = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextExpandTabs | Qt::TextForceLeftToRight;
  style->drawItemText(&p, textRect, flags, pal, isEnabled(), m_sTitle, foregroundRole());

}
