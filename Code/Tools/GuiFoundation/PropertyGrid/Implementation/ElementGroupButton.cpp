#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/Implementation/ElementGroupButton.moc.h>

ezQtElementGroupButton::ezQtElementGroupButton(QWidget* pParent, ezQtElementGroupButton::ElementAction action, ezQtPropertyWidget* pGroupWidget)
  : QToolButton(pParent)
{
  m_Action = action;
  m_pGroupWidget = pGroupWidget;

  setIconSize(QSize(16, 16));

  switch (action)
  {
  case ezQtElementGroupButton::ElementAction::MoveElementUp:
    setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/MoveUp16.png")));
    break;
  case ezQtElementGroupButton::ElementAction::MoveElementDown:
    setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/MoveDown16.png")));
    break;
  case ezQtElementGroupButton::ElementAction::DeleteElement:
    setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/Delete16.png")));
    break;
  }
  
}
