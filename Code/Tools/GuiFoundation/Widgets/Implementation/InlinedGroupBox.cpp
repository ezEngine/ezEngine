#include <PCH.h>
#include <GuiFoundation/Widgets/InlinedGroupBox.moc.h>
#include <QStyleOptionToolButton>
#include <QPainter>
#include <QBoxLayout>
#include <QMouseEvent>

ezQtInlinedGroupBox::ezQtInlinedGroupBox(QWidget* pParent)
  : ezQtGroupBoxBase(pParent, false)
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

  installEventFilter(this);
}

void ezQtInlinedGroupBox::SetTitle(const char* szTitle)
{
  ezQtGroupBoxBase::SetTitle(szTitle);
  update();
}

void ezQtInlinedGroupBox::SetIcon(const QIcon& icon)
{
  ezQtGroupBoxBase::SetIcon(icon);
  update();
}

void ezQtInlinedGroupBox::SetFillColor(const QColor& color)
{
  ezQtGroupBoxBase::SetFillColor(color);
  update();
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

  QRect wr = contentsRect();

  if (m_FillColor.isValid())
  {
    QRectF wrAdjusted = wr;
    wrAdjusted.adjust(0.5, 0.5, Rounding, -0.5);
    QPainterPath oPath;
    oPath.addRoundedRect(wrAdjusted, Rounding, Rounding);
    p.fillPath(oPath, pal.alternateBase());
  }

  DrawHeader(p, wr.adjusted(Rounding, 0, 0, 0));
}

bool ezQtInlinedGroupBox::eventFilter(QObject* object, QEvent* event)
{
  switch (event->type())
  {
  case QEvent::Type::MouseButtonPress:
    HeaderMousePress(static_cast<QMouseEvent*>(event));
    return true;
  case QEvent::Type::MouseMove:
    HeaderMouseMove(static_cast<QMouseEvent*>(event));
    return true;
  case QEvent::Type::MouseButtonRelease:
    HeaderMouseRelease(static_cast<QMouseEvent*>(event));
    return true;
  default:
    break;
  }
  return false;
}
