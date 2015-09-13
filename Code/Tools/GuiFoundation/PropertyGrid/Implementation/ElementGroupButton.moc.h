#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QToolButton>

class EZ_GUIFOUNDATION_DLL ezElementGroupButton : public QToolButton
{
  Q_OBJECT
public:
  enum class ElementAction
  {
    MoveElementUp,
    MoveElementDown,
    DeleteElement,
  };

  explicit ezElementGroupButton(QWidget* pParent, ElementAction action, ezPropertyBaseWidget* pGroupWidget);
  ElementAction GetAction() const { return m_Action; }
  ezPropertyBaseWidget* GetGroupWidget() const { return m_pGroupWidget; }

private:
  ElementAction m_Action;
  ezPropertyBaseWidget* m_pGroupWidget;
};
