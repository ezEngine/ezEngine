#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyEventHandler.h>
#include <QWidget>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/VariantSubAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class ezDocumentObject;
class ezQtTypeWidget;
class QHBoxLayout;
class QVBoxLayout;
class QLabel;
class QMenu;
class QComboBox;
class ezQtGroupBoxBase;
class ezQtAddSubElementButton;
class ezQtPropertyGridWidget;
class ezQtElementGroupButton;
class QMimeData;
struct ezCommandHistoryEvent;
class ezObjectAccessorBase;

/// \brief Base class for all property widgets
class EZ_GUIFOUNDATION_DLL ezQtPropertyWidget : public QWidget
{
  Q_OBJECT;

public:
  explicit ezQtPropertyWidget();
  virtual ~ezQtPropertyWidget();

  void Init(ezQtPropertyGridWidget* pGrid, ezObjectAccessorBase* pObjectAccessor, const ezRTTI* pType, const ezAbstractProperty* pProp);

  ezQtPropertyGridWidget* GetPropertyGrid() { return m_pGrid; }
  ezObjectAccessorBase* GetObjectAccessor() { return m_pObjectAccessor; }
  const ezRTTI* GetType() const { return m_pType; }
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
  virtual const char* GetLabel(ezStringBuilder& ref_sTmp) const;

  virtual void ExtendContextMenu(QMenu& ref_menu);

  /// \brief Whether the variable that the widget represents is currently set to the default value or has been modified.
  virtual void SetIsDefault(bool bIsDefault) { m_bIsDefault = bIsDefault; }

  /// \brief If the property is of type ezVariant this function returns whether all items have the same type.
  /// If true is returned, out_Type contains the common type. Note that 'invalid' can be a common type.
  bool GetCommonVariantSubType(
    const ezHybridArray<ezPropertySelection, 8>& items, const ezAbstractProperty* pProperty, ezVariantType::Enum& out_type);

  ezVariant GetCommonValue(const ezHybridArray<ezPropertySelection, 8>& items, const ezAbstractProperty* pProperty);
  void PrepareToDie();

  /// \brief By default disables the widget, but can be overridden to make a widget more interactable (for example to be able to copy text from it).
  virtual void SetReadOnly(bool bReadOnly = true);

public:
  static const ezRTTI* GetCommonBaseType(const ezHybridArray<ezPropertySelection, 8>& items);
  static QColor SetPaletteBackgroundColor(ezColorGammaUB inputColor, QPalette& ref_palette);

public Q_SLOTS:
  void OnCustomContextMenu(const QPoint& pt);

protected:
  void Broadcast(ezPropertyEvent::Type type);
  void PropertyChangedHandler(const ezPropertyEvent& ed);

  virtual void OnInit() = 0;
  bool IsUndead() const { return m_bUndead; }

protected:
  virtual void DoPrepareToDie() = 0;

  virtual bool eventFilter(QObject* pWatched, QEvent* pEvent) override;

  ezQtPropertyGridWidget* m_pGrid = nullptr;
  ezObjectAccessorBase* m_pObjectAccessor = nullptr;
  const ezRTTI* m_pType = nullptr;
  const ezAbstractProperty* m_pProp = nullptr;
  ezHybridArray<ezPropertySelection, 8> m_Items;
  bool m_bIsDefault; ///< Whether the variable that the widget represents is currently set to the default value or has been modified.

private:
  bool m_bUndead;    ///< Widget is being destroyed
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


/// \brief Base class for more 'advanced' property type widgets for Pointer or Class type properties.
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
  bool m_bTemporaryCommand = false;
  const ezRTTI* m_pResolvedType = nullptr;
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
  virtual void SetIsDefault(bool bIsDefault) override;

protected:
  virtual void OnInit() override;
  virtual void DoPrepareToDie() override;

protected:
  QVBoxLayout* m_pLayout;
  ezQtGroupBoxBase* m_pGroup;
  QVBoxLayout* m_pGroupLayout;
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


public Q_SLOTS:
  void OnDeleteButtonClicked();

protected:
  virtual void OnInit() override;
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  virtual void DoPrepareToDie() override;
  void UpdateTitle(const ezRTTI* pType = nullptr);

protected:
  QHBoxLayout* m_pLayout = nullptr;
  ezQtGroupBoxBase* m_pGroup = nullptr;
  ezQtAddSubElementButton* m_pAddButton = nullptr;
  ezQtElementGroupButton* m_pDeleteButton = nullptr;
  QHBoxLayout* m_pGroupLayout = nullptr;
  ezQtTypeWidget* m_pTypeWidget = nullptr;
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
  virtual void SetIsDefault(bool bIsDefault) override;

public Q_SLOTS:
  void OnElementButtonClicked();
  void OnDragStarted(QMimeData& ref_mimeData);
  void OnContainerContextMenu(const QPoint& pt);
  void OnCustomElementContextMenu(const QPoint& pt);

protected:
  struct Element
  {
    Element() = default;

    Element(ezQtGroupBoxBase* pSubGroup, ezQtPropertyWidget* pWidget, ezQtElementGroupButton* pHelpButton)
      : m_pSubGroup(pSubGroup)
      , m_pWidget(pWidget)
      , m_pHelpButton(pHelpButton)
    {
    }

    ezQtGroupBoxBase* m_pSubGroup = nullptr;
    ezQtPropertyWidget* m_pWidget = nullptr;
    ezQtElementGroupButton* m_pHelpButton = nullptr;
  };

  virtual ezQtGroupBoxBase* CreateElement(QWidget* pParent);
  virtual ezQtPropertyWidget* CreateWidget(ezUInt32 index);
  virtual Element& AddElement(ezUInt32 index);
  virtual void RemoveElement(ezUInt32 index);
  virtual void UpdateElement(ezUInt32 index) = 0;
  void UpdateElements();
  virtual void GetRequiredElements(ezDynamicArray<ezVariant>& out_keys) const;
  virtual void UpdatePropertyMetaState();
  /// \brief Some containers like ezVariant can be both a map or an array so we can't reply on the property type alone. For these containers, this method can be overwritten to retrieve the category from something other than `m_pProp->GetCategory()`.
  virtual ezPropertyCategory::Enum GetContainerCategory() const;

  void Clear();
  virtual void OnInit() override;

  void DeleteItems(ezHybridArray<ezPropertySelection, 8>& items);
  void MoveItems(ezHybridArray<ezPropertySelection, 8>& items, ezInt32 iMove);
  virtual void DoPrepareToDie() override;
  virtual void dragEnterEvent(QDragEnterEvent* event) override;
  virtual void dragMoveEvent(QDragMoveEvent* event) override;
  virtual void dragLeaveEvent(QDragLeaveEvent* event) override;
  virtual void dropEvent(QDropEvent* event) override;
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void showEvent(QShowEvent* event) override;

private:
  bool updateDropIndex(QDropEvent* pEvent);

protected:
  QHBoxLayout* m_pLayout;
  ezQtGroupBoxBase* m_pGroup;
  QVBoxLayout* m_pGroupLayout;
  ezQtAddSubElementButton* m_pAddButton = nullptr;
  QPalette m_Pal;

  ezHybridArray<ezVariant, 16> m_Keys;
  ezDynamicArray<Element> m_Elements;
  ezInt32 m_iDropSource = -1;
  ezInt32 m_iDropTarget = -1;
};


class EZ_GUIFOUNDATION_DLL ezQtPropertyStandardTypeContainerWidget : public ezQtPropertyContainerWidget
{
  Q_OBJECT;

public:
  ezQtPropertyStandardTypeContainerWidget();
  virtual ~ezQtPropertyStandardTypeContainerWidget();

protected:
  virtual ezQtGroupBoxBase* CreateElement(QWidget* pParent) override;
  virtual ezQtPropertyWidget* CreateWidget(ezUInt32 index) override;
  virtual Element& AddElement(ezUInt32 index) override;
  virtual void RemoveElement(ezUInt32 index) override;
  virtual void UpdateElement(ezUInt32 index) override;
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
  bool m_bNeedsUpdate = false;
};

class EZ_GUIFOUNDATION_DLL ezQtVariantPropertyWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT;

public:
  ezQtVariantPropertyWidget();
  virtual ~ezQtVariantPropertyWidget();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;
  virtual void DoPrepareToDie() override;
  void UpdateTypeListSelection(ezVariantType::Enum type);
  void ChangeVariantType(ezVariantType::Enum type);

  virtual ezResult GetVariantTypeDisplayName(ezVariantType::Enum type, ezStringBuilder& out_sName) const;

protected:
  QVBoxLayout* m_pLayout = nullptr;
  QComboBox* m_pTypeList = nullptr;
  ezQtPropertyWidget* m_pWidget = nullptr;
  const ezRTTI* m_pCurrentSubType = nullptr;
};

// Used for sub-containers of an ezVariant, e.g. an ezVariantArray or ezVariantDictionary stored inside an ezVariant. ezVariantSubAccessor is used to create a view into a sub-tree container of the ezVariant.
class EZ_GUIFOUNDATION_DLL ezQtVariantContainerWidget : public ezQtPropertyStandardTypeContainerWidget
{
  Q_OBJECT;

public:
  ezQtVariantContainerWidget(ezVariantType::Enum variantType);
  virtual ~ezQtVariantContainerWidget() = default;

protected:
  virtual void OnInit() override;
  virtual void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items) override;
  virtual ezPropertyCategory::Enum GetContainerCategory() const override;

private:
  ezUniquePtr<ezVariantSubAccessor> m_pVariantSubAccessor;
  ezEnum<ezPropertyCategory> m_ContainerCategory;
};
