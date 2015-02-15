#include <PCH.h>
#include <EditorFramework/GUI/CollapsibleGroupBox.moc.h>
#include <QStyleOptionToolButton>
#include <QStyle>
#include <QPainter>

const ezInt32 s_GroupBoxHeight = 17;
ezCollapsibleGroupBox::ezCollapsibleGroupBox(QWidget* pParent) : QGroupBox(pParent)
{
  setCheckable(true);
  connect(this, SIGNAL( toggled(bool) ), this, SLOT( on_toggled(bool) ));
}

ezCollapsibleGroupBox::~ezCollapsibleGroupBox()
{
}

void ezCollapsibleGroupBox::paintEvent(QPaintEvent* pEvent)
{
  //QGroupBox::paintEvent(pEvent);

  QStyleOptionGroupBox option;
  initStyleOption(&option);
  QRect textRect = style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxLabel, this);

  QPainter p(this);

  // Frame
  {
      QStyleOptionFrame frame;
      frame.QStyleOption::operator=(option);
      frame.features = option.features;
      frame.lineWidth = option.lineWidth;
      frame.midLineWidth = option.midLineWidth;
      frame.rect = style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxFrame, this);
      style()->drawPrimitive(QStyle::PE_FrameGroupBox, &frame, &p, this);
  }

  // Frame Header
  {
    QStyleOptionToolButton opt;
    opt.init(this);
    opt.rect.setHeight(s_GroupBoxHeight);
    opt.state |= (QStyle::State_AutoRaise | QStyle::State_On);
    style()->drawPrimitive(QStyle::PE_PanelButtonTool, &opt, &p, this);
  }

  // Indicator
  {
    QStyleOptionViewItem opt;
    QStyle::State extraFlags = QStyle::State_None;
    if (isEnabled())
      extraFlags |= QStyle::State_Enabled;
    if (window()->isActiveWindow())
      extraFlags |= QStyle::State_Active;

    opt.rect = textRect;
    opt.rect.setX(0);
    opt.rect.setWidth(textRect.x());
    const bool expanded = isChecked();
    opt.state = extraFlags | QStyle::State_Item | QStyle::State_None | QStyle::State_Children | (expanded ? QStyle::State_Open : QStyle::State_None);
    style()->drawPrimitive(QStyle::PE_IndicatorBranch, &opt, &p, this);
  }

  // Text
  {
    p.setPen(QPen(option.palette.windowText(), 1));
    int alignment = int(option.textAlignment);
    if (!style()->styleHint(QStyle::SH_UnderlineShortcut, &option, this))
      alignment |= Qt::TextHideMnemonic;

    style()->drawItemText(&p, textRect,  Qt::TextShowMnemonic | Qt::AlignLeft | alignment, option.palette, option.state, option.text, QPalette::NoRole);
  }
}

void ezCollapsibleGroupBox::on_toggled(bool bToggled)
{
  setUpdatesEnabled(false);
  QObjectList childList = children();
  for (int i = 0; i < childList.size(); ++i)
  {
    QObject* o = childList.at(i);
    if (o->isWidgetType())
    {
      QWidget* w = static_cast<QWidget*>(o);
      w->setVisible(bToggled);
    }
  }

  if(bToggled)
  {
    setFixedHeight(QWIDGETSIZE_MAX);
    adjustSize();
  }
  else
  {
    setFixedHeight(s_GroupBoxHeight);
  }
  setUpdatesEnabled(true);
}
