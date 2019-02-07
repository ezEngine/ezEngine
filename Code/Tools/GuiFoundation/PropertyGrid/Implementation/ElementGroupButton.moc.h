#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QToolButton>

class EZ_GUIFOUNDATION_DLL ezQtElementGroupButton : public QToolButton
{
  Q_OBJECT
public:
  enum class ElementAction
  {
    MoveElementUp,
    MoveElementDown,
    DeleteElement,
  };

  explicit ezQtElementGroupButton(QWidget* pParent, ElementAction action, ezQtPropertyWidget* pGroupWidget);
  ElementAction GetAction() const { return m_Action; }
  ezQtPropertyWidget* GetGroupWidget() const { return m_pGroupWidget; }

private:
  ElementAction m_Action;
  ezQtPropertyWidget* m_pGroupWidget;
};
