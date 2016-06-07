#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <QWidget>

class ezDocumentObject;
class ezTypeWidget;
class QHBoxLayout;
class QVBoxLayout;
class QLabel;
class QMenu;
class ezCollapsibleGroupBox;
class ezAddSubElementButton;
class ezPropertyGridWidget;
class ezElementGroupButton;

/// \brief Base class for all property widgets
class EZ_GUIFOUNDATION_DLL ezQtPropertyWidget : public QWidget
{
  Q_OBJECT;
public:
  struct Selection
  {
    const ezDocumentObject* m_pObject;
    ezVariant m_Index;

    bool operator==(const Selection& rhs) const
    {
      return m_pObject == rhs.m_pObject && m_Index == rhs.m_Index;
    }
  };

  struct Event
  {
    enum class Type
    {
      ValueChanged,
      BeginTemporary,
      EndTemporary,
      CancelTemporary,
    };

    Type m_Type;
    const ezPropertyPath* m_pPropertyPath;
    const ezHybridArray<Selection, 8>* m_pItems;
    ezVariant m_Value;
  };

  ezEvent<const Event&> m_Events;

public:
  explicit ezQtPropertyWidget();
  virtual ~ezQtPropertyWidget();

  void Init(ezPropertyGridWidget* pGrid, const ezAbstractProperty* pProp, const ezPropertyPath& path);
  const ezAbstractProperty* GetProperty() const { return m_pProp; }
  const ezPropertyPath& GetPropertyPath() const { return m_PropertyPath; }

  virtual void SetSelection(const ezHybridArray<Selection, 8>& items);
  const ezHybridArray<Selection, 8>& GetSelection() const { return m_Items; }

  virtual bool HasLabel() const { return true; }
  virtual const char* GetLabel() const { return m_pProp->GetPropertyName(); }

  void SetIsDefault(bool isDefault) { m_bIsDefault = isDefault; }

  static const ezRTTI* GetCommonBaseType(const ezHybridArray<ezQtPropertyWidget::Selection, 8>& items);

  void PrepareToDie();

public slots:
  void OnCustomContextMenu(const QPoint& pt);
  
protected:
  void Broadcast(Event::Type type);
  virtual void OnInit() = 0;
  bool IsUndead() const { return m_bUndead; }

  virtual void ExtendContextMenu(QMenu& menu) {}

protected:
  virtual void DoPrepareToDie() = 0;

  ezPropertyGridWidget* m_pGrid;
  const ezAbstractProperty* m_pProp;
  ezPropertyPath m_PropertyPath;
  ezHybridArray<Selection, 8> m_Items;

private:
  bool m_bUndead;
  bool m_bIsDefault;
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

  virtual void SetSelection(const ezHybridArray<Selection, 8>& items) override;

protected:
  void BroadcastValueChanged(const ezVariant& NewValue);
  virtual void DoPrepareToDie() override {}

  const ezVariant& GetOldValue() const { return m_OldValue; }
  virtual void InternalSetValue(const ezVariant& value) = 0;

protected:
  ezVariant m_OldValue;
};


/// \todo Not really sure what this is used for
class EZ_GUIFOUNDATION_DLL ezQtPropertyTypeWidget : public ezQtPropertyWidget
{
  Q_OBJECT;
public:
  explicit ezQtPropertyTypeWidget(bool bAddCollapsibleGroup = false);
  virtual ~ezQtPropertyTypeWidget();

  virtual void SetSelection(const ezHybridArray<Selection, 8>& items) override;
  virtual bool HasLabel() const override { return false; }


protected:
  virtual void OnInit() override;
  virtual void DoPrepareToDie() override;

protected:
  QHBoxLayout* m_pLayout;
  ezCollapsibleGroupBox* m_pGroup;
  QHBoxLayout* m_pGroupLayout;
  ezTypeWidget* m_pTypeWidget;
};

/// \brief Used for property types that are pointers.
class EZ_GUIFOUNDATION_DLL ezQtPropertyPointerWidget : public ezQtPropertyWidget
{
  Q_OBJECT;
public:
  explicit ezQtPropertyPointerWidget();
  virtual ~ezQtPropertyPointerWidget();

  virtual void SetSelection(const ezHybridArray<Selection, 8>& items) override;
  virtual bool HasLabel() const override { return false; }


public slots:
  void OnDeleteButtonClicked();

protected:
  virtual void OnInit() override;
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  virtual void DoPrepareToDie() override;

protected:
  QHBoxLayout* m_pLayout;
  ezCollapsibleGroupBox* m_pGroup;
  ezAddSubElementButton* m_pAddButton;
  ezElementGroupButton* m_pDeleteButton;
  QHBoxLayout* m_pGroupLayout;
  ezTypeWidget* m_pTypeWidget;
};


/// \brief Base class for all container properties
class EZ_GUIFOUNDATION_DLL ezQtPropertyContainerWidget : public ezQtPropertyWidget
{
  Q_OBJECT;
public:
  ezQtPropertyContainerWidget();
  virtual ~ezQtPropertyContainerWidget();

  virtual void SetSelection(const ezHybridArray<Selection, 8>& items) override;
  virtual bool HasLabel() const override { return false; }


public slots:
  void OnElementButtonClicked();

protected:
  struct Element
  {
    Element() : m_pSubGroup(nullptr), m_pWidget(nullptr) {}
    Element(ezCollapsibleGroupBox* pSubGroup, ezQtPropertyWidget* pWidget) : m_pSubGroup(pSubGroup), m_pWidget(pWidget) {}

    ezCollapsibleGroupBox* m_pSubGroup;
    ezQtPropertyWidget* m_pWidget;
  };

  virtual Element& AddElement(ezUInt32 index);
  virtual void RemoveElement(ezUInt32 index);
  virtual void UpdateElement(ezUInt32 index) = 0;
  void UpdateElements();
  ezUInt32 GetRequiredElementCount() const;

  void Clear();
  virtual void OnInit() override;
  
  void DeleteItems(ezHybridArray<Selection, 8>& items, const ezPropertyPath& path);
  void MoveItems(ezHybridArray<Selection, 8>& items, const ezPropertyPath& path, ezInt32 iMove);
  virtual void DoPrepareToDie() override;

protected:
  QHBoxLayout* m_pLayout;
  ezCollapsibleGroupBox* m_pGroup;
  QVBoxLayout* m_pGroupLayout;
  ezAddSubElementButton* m_pAddButton;

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

  void PropertyChangedHandler(const ezQtPropertyWidget::Event& ed);
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
};