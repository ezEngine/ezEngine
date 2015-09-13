#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/Implementation/ElementGroupButton.moc.h>

ezElementGroupButton::ezElementGroupButton(QWidget* pParent, ezElementGroupButton::ElementAction action, ezPropertyBaseWidget* pGroupWidget)
  : QToolButton(pParent)
{
  m_Action = action;
  m_pGroupWidget = pGroupWidget;

  setIconSize(QSize(16, 16));

  switch (action)
  {
  case ezElementGroupButton::ElementAction::MoveElementUp:
    setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/MoveUp16.png")));
    break;
  case ezElementGroupButton::ElementAction::MoveElementDown:
    setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/MoveDown16.png")));
    break;
  case ezElementGroupButton::ElementAction::DeleteElement:
    setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/Delete16.png")));
    break;
  }
  
}
