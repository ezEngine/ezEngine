#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Communication/Event.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyEventHandler.h>
#include <QWidget>

class ezDocumentObject;
class ezQtTypeWidget;
class QHBoxLayout;
class QVBoxLayout;
class QLabel;
class QMenu;
class ezQtCollapsibleGroupBox;
class ezQtAddSubElementButton;
class ezQtPropertyGridWidget;
class ezQtElementGroupButton;
struct ezCommandHistoryEvent;

/// \brief Base class for all property widgets
class EZ_GUIFOUNDATION_DLL ezQtPropertyWidget : public QWidget
{
  Q_OBJECT;
public:
  ezEvent<const ezPropertyEvent&> m_Events;

public:
  explicit ezQtPropertyWidget();
  virtual ~ezQtPropertyWidget();

  void Init(ezQtPropertyGridWidget* pGrid, const ezAbstractProperty* pProp);
  const ezAbstractProperty* GetProperty() const { return m_pProp; }

  /// \brief This is called whenever the selection in the editor changes and thus the widget may need to display a different value.
  ///
  /// If the array holds more than one element, the user selected multiple objects. In this case, the code should check whether
  /// the values differ across the selected objects and if so, the widget should display "multiple values".
  virtual void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items);
  const ezHybridArray<ezPropertySelection, 8>& GetSelection() const { return m_Items; }

  /// \brief If this returns true (default), a QLabel is created and the text that GetLabel() returns is displayed.
  virtual bool HasLabel() const { return true; }

  /// \brief The return value is used to display a label, if HasLabel() returns true.
  virtual const char* GetLabel() const { return m_pProp->GetPropertyName(); }

  /// \brief Whether the variable that the widget represents is currently set to the default value or has been modified.
  void SetIsDefault(bool isDefault) { m_bIsDefault = isDefault; }

  static const ezRTTI* GetCommonBaseType(const ezHybridArray<ezPropertySelection, 8>& items);
  ezVariant GetCommonValue(const ezHybridArray<ezPropertySelection, 8>& items, const ezAbstractProperty* pProperty);
  void PrepareToDie();

public slots:
  void OnCustomContextMenu(const QPoint& pt);

protected:
  void Broadcast(ezPropertyEvent::Type type);
  virtual void OnInit() = 0;
  bool IsUndead() const { return m_bUndead; }

  virtual void ExtendContextMenu(QMenu& menu) {}

protected:
  virtual void DoPrepareToDie() = 0;

  ezQtPropertyGridWidget* m_pGrid;
  const ezAbstractProperty* m_pProp;
  ezHybridArray<ezPropertySelection, 8> m_Items;

private:
  bool m_bUndead;    ///< Widget is being destroyed
  bool m_bIsDefault; ///< Whether the variable that the widget represents is currently set to the default value or has been modified.
};


/// \brief Fallback widget for all property types for which no other widget type is registered
class EZ_GUIFOUNDATION_DLL ezQtUnsupportedPropertyWidget : public ezQtPropertyWidget
{
  Q_OBJECT;
public:
  explicit ezQtUnsupportedPropertyWidget(const char* szMessage = nullptr);

protected:
  virtual void OnInit() override;
  virtual void DoPrepareToDie() override {}

  QHBoxLayout* m_pLayout;
  QLabel* m_pWidget;
  ezString m_sMessage;
};


/// \brief Base class for most 'simple' property type widgets. Implements some of the standard functionality.
class EZ_GUIFOUNDATION_DLL ezQtStandardPropertyWidget : public ezQtPropertyWidget
{
  Q_OBJECT;
public:
  explicit ezQtStandardPropertyWidget();

  virtual void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items) override;

protected:
  void BroadcastValueChanged(const ezVariant& NewValue);
  virtual void DoPrepareToDie() override {}

  const ezVariant& GetOldValue() const { return m_OldValue; }
  virtual void InternalSetValue(const ezVariant& value) = 0;

protected:
  ezVariant m_OldValue;
};


/// \brief Base class for more 'advanced' property type widgets for Pointer or EmbeddedClass type properties.
/// Implements some of ezQtTypeWidget functionality at property widget level.
class EZ_GUIFOUNDATION_DLL ezQtEmbeddedClassPropertyWidget : public ezQtPropertyWidget
{
  Q_OBJECT;
public:
  explicit ezQtEmbeddedClassPropertyWidget();
  ~ezQtEmbeddedClassPropertyWidget();

  virtual void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items) override;

protected:
  void SetPropertyValue(const ezAbstractProperty* pProperty, const ezVariant& NewValue);

  virtual void OnInit() override;
  virtual void DoPrepareToDie() override;
  virtual void OnPropertyChanged(const ezString& sProperty) = 0;

private:
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistoryEvent& e);
  void FlushQueuedChanges();

protected:
  bool m_bTemporaryCommand;
  const ezRTTI* m_pResolvedType;
  ezHybridArray<ezPropertySelection, 8> m_ResolvedObjects;

  ezHybridArray<ezString, 1> m_QueuedChanges;
};


/// Used for pointers and embedded classes.
/// Does not inherit from ezQtEmbeddedClassPropertyWidget as it just embeds
/// a ezQtTypeWidget for the property's value which handles everything already.
class EZ_GUIFOUNDATION_DLL ezQtPropertyTypeWidget : public ezQtPropertyWidget
{
  Q_OBJECT;
public:
  explicit ezQtPropertyTypeWidget(bool bAddCollapsibleGroup = false);
  virtual ~ezQtPropertyTypeWidget();

  virtual void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items) override;
  virtual bool HasLabel() const override { return false; }


protected:
  virtual void OnInit() override;
  virtual void DoPrepareToDie() override;

protected:
  QHBoxLayout* m_pLayout;
  ezQtCollapsibleGroupBox* m_pGroup;
  QHBoxLayout* m_pGroupLayout;
  ezQtTypeWidget* m_pTypeWidget;
};

/// \brief Used for property types that are pointers.
class EZ_GUIFOUNDATION_DLL ezQtPropertyPointerWidget : public ezQtPropertyWidget
{
  Q_OBJECT;
public:
  explicit ezQtPropertyPointerWidget();
  virtual ~ezQtPropertyPointerWidget();

  virtual void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items) override;
  virtual bool HasLabel() const override { return false; }


public slots:
  void OnDeleteButtonClicked();

protected:
  virtual void OnInit() override;
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  virtual void DoPrepareToDie() override;

protected:
  QHBoxLayout* m_pLayout;
  ezQtCollapsibleGroupBox* m_pGroup;
  ezQtAddSubElementButton* m_pAddButton;
  ezQtElementGroupButton* m_pDeleteButton;
  QHBoxLayout* m_pGroupLayout;
  ezQtTypeWidget* m_pTypeWidget;
};


/// \brief Base class for all container properties
class EZ_GUIFOUNDATION_DLL ezQtPropertyContainerWidget : public ezQtPropertyWidget
{
  Q_OBJECT;
public:
  ezQtPropertyContainerWidget();
  virtual ~ezQtPropertyContainerWidget();

  virtual void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items) override;
  virtual bool HasLabel() const override { return false; }


public slots:
  void OnElementButtonClicked();

protected:
  struct Element
  {
    Element() : m_pSubGroup(nullptr), m_pWidget(nullptr) {}
    Element(ezQtCollapsibleGroupBox* pSubGroup, ezQtPropertyWidget* pWidget) : m_pSubGroup(pSubGroup), m_pWidget(pWidget) {}

    ezQtCollapsibleGroupBox* m_pSubGroup;
    ezQtPropertyWidget* m_pWidget;
  };

  virtual Element& AddElement(ezUInt32 index);
  virtual void RemoveElement(ezUInt32 index);
  virtual void UpdateElement(ezUInt32 index) = 0;
  void UpdateElements();
  ezUInt32 GetRequiredElementCount() const;

  void Clear();
  virtual void OnInit() override;

  void DeleteItems(ezHybridArray<ezPropertySelection, 8>& items);
  void MoveItems(ezHybridArray<ezPropertySelection, 8>& items, ezInt32 iMove);
  virtual void DoPrepareToDie() override;

protected:
  QHBoxLayout* m_pLayout;
  ezQtCollapsibleGroupBox* m_pGroup;
  QVBoxLayout* m_pGroupLayout;
  ezQtAddSubElementButton* m_pAddButton;

  mutable ezHybridArray<ezVariant, 16> m_Keys;
  ezDynamicArray<Element> m_Elements;
};


class EZ_GUIFOUNDATION_DLL ezQtPropertyStandardTypeContainerWidget : public ezQtPropertyContainerWidget
{
  Q_OBJECT;
public:
  ezQtPropertyStandardTypeContainerWidget();
  virtual ~ezQtPropertyStandardTypeContainerWidget();

protected:
  virtual Element& AddElement(ezUInt32 index) override;
  virtual void RemoveElement(ezUInt32 index) override;
  virtual void UpdateElement(ezUInt32 index) override;

  void PropertyChangedHandler(const ezPropertyEvent& ed);
};

class EZ_GUIFOUNDATION_DLL ezQtPropertyTypeContainerWidget : public ezQtPropertyContainerWidget
{
  Q_OBJECT;
public:
  ezQtPropertyTypeContainerWidget();
  virtual ~ezQtPropertyTypeContainerWidget();

protected:
  virtual void OnInit() override;
  virtual void UpdateElement(ezUInt32 index) override;

  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistoryEvent& e);

private:
  bool m_bNeedsUpdate;
};
