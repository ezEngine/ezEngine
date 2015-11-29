#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <QWidget>

class QGridLayout;
class ezDocument;

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

  void PropertyChangedHandler(const ezQtPropertyWidget::Event& ed);
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistory::Event& e);

  void UpdateProperty(const ezDocumentObject* pObject, const ezString& sProperty);
  void FlushQueuedChanges();

private:
  ezPropertyGridWidget* m_pGrid;
  const ezRTTI* m_pType;
  ezPropertyPath m_ParentPath;
  ezHybridArray<ezQtPropertyWidget::Selection, 8> m_Items;

  QGridLayout* m_pLayout;
  ezMap<ezString, ezQtPropertyWidget*> m_PropertyWidgets;
  ezHybridArray<ezString, 1> m_QueuedChanges;

};


