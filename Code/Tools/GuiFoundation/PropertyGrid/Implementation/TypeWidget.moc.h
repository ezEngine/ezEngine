#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <QWidget>

class QGridLayout;
class ezDocument;
class ezQtManipulatorLabel;
struct ezManipulatorManagerEvent;
class ezObjectAccessorBase;

class EZ_GUIFOUNDATION_DLL ezQtTypeWidget : public QWidget
{
  Q_OBJECT
public:
  ezQtTypeWidget(QWidget* pParent, ezQtPropertyGridWidget* pGrid, ezObjectAccessorBase* pObjectAccessor, const ezRTTI* pType,
                 const char* szIncludeProperties, const char* szExcludeProperties);
  ~ezQtTypeWidget();
  void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items);
  const ezHybridArray<ezPropertySelection, 8>& GetSelection() const { return m_Items; }
  const ezRTTI* GetType() const { return m_pType; }
  void PrepareToDie();

private:
  void BuildUI(const ezRTTI* pType, const char* szIncludeProperties, const char* szExcludeProperties);
  void BuildUI(const ezRTTI* pType, const ezMap<ezString, const ezManipulatorAttribute*>& manipulatorMap, const char* szIncludeProperties,
               const char* szExcludeProperties);

  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistoryEvent& e);
  void ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e);

  void UpdateProperty(const ezDocumentObject* pObject, const ezString& sProperty);
  void FlushQueuedChanges();
  void UpdatePropertyMetaState();

private:
  bool m_bUndead = false;
  ezQtPropertyGridWidget* m_pGrid = nullptr;
  ezObjectAccessorBase* m_pObjectAccessor = nullptr;
  const ezRTTI* m_pType = nullptr;
  ezHybridArray<ezPropertySelection, 8> m_Items;

  struct PropertyWidgetData
  {
    ezQtPropertyWidget* m_pWidget;
    ezQtManipulatorLabel* m_pLabel;
    ezString m_sOriginalLabelText;
  };

  QGridLayout* m_pLayout;
  ezMap<ezString, PropertyWidgetData> m_PropertyWidgets;
  ezHybridArray<ezString, 1> m_QueuedChanges;

};


