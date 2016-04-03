#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <QWidget>

class QGridLayout;
class ezDocument;
class ezManipulatorLabel;
struct ezManipulatorManagerEvent;

class EZ_GUIFOUNDATION_DLL ezTypeWidget : public QWidget
{
  Q_OBJECT
public:
  ezTypeWidget(QWidget* pParent, ezPropertyGridWidget* pGrid, const ezRTTI* pType, ezPropertyPath& parentPath);
  ~ezTypeWidget();
  void SetSelection(const ezHybridArray<ezQtPropertyWidget::Selection, 8>& items);
  const ezHybridArray<ezQtPropertyWidget::Selection, 8>& GetSelection() const { return m_Items; }
  const ezRTTI* GetType() const { return m_pType; }
  const ezPropertyPath& GetPropertyPath() const { return m_ParentPath; }

private:
  void BuildUI(const ezRTTI* pType, ezPropertyPath& ParentPath);
  void BuildUI(const ezRTTI* pType, ezPropertyPath& ParentPath, const ezMap<ezString, const ezManipulatorAttribute*>& manipulatorMap);

  void PropertyChangedHandler(const ezQtPropertyWidget::Event& ed);
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistory::Event& e);
  void ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e);

  void UpdateProperty(const ezDocumentObject* pObject, const ezString& sProperty);
  void FlushQueuedChanges();
  void UpdatePropertyMetaState();

private:
  ezPropertyGridWidget* m_pGrid;
  const ezRTTI* m_pType;
  ezPropertyPath m_ParentPath;
  ezHybridArray<ezQtPropertyWidget::Selection, 8> m_Items;

  struct PropertyWidgetData
  {
    ezQtPropertyWidget* m_pWidget;
    ezManipulatorLabel* m_pLabel;
    ezString m_sOriginalLabelText;
  };

  QGridLayout* m_pLayout;
  ezMap<ezString, PropertyWidgetData> m_PropertyWidgets;
  ezHybridArray<ezString, 1> m_QueuedChanges;

};


