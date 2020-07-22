#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QWidget>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

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
  struct PropertyGroup
  {
    PropertyGroup(const ezGroupAttribute* attr, float& fOrder)
    {
      if (attr)
      {
        m_sGroup = attr->GetGroup();
        m_sIconName = attr->GetIconName();
        m_fOrder = attr->GetOrder();
        if (m_fOrder == -1.0f)
        {
          fOrder += 1.0f;
          m_fOrder = fOrder;
        }
      }
      else
      {
        fOrder += 1.0f;
        m_fOrder = fOrder;
      }
    }

    void MergeGroup(const ezGroupAttribute* attr)
    {
      if (attr)
      {
        m_sGroup = attr->GetGroup();
        m_sIconName = attr->GetIconName();
        if (attr->GetOrder() != -1.0f)
        {
          m_fOrder = attr->GetOrder();
        }
      }
    }

    bool operator==(const PropertyGroup& rhs) { return m_sGroup == rhs.m_sGroup; }
    bool operator<(const PropertyGroup& rhs) { return m_fOrder < rhs.m_fOrder; }

    ezString m_sGroup;
    ezString m_sIconName;
    float m_fOrder = -1.0f;
    ezHybridArray<const ezAbstractProperty*, 8> m_Properties;
  };

  void BuildUI(const ezRTTI* pType, const char* szIncludeProperties, const char* szExcludeProperties);
  void BuildUI(const ezRTTI* pType, const ezMap<ezString, const ezManipulatorAttribute*>& manipulatorMap,
    const ezDynamicArray<ezUniquePtr<PropertyGroup>>& groups, const char* szIncludeProperties, const char* szExcludeProperties);

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
