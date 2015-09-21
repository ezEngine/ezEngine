#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <QWidget>

class ezDocumentObjectBase;
class ezTypeWidget;
class QHBoxLayout;
class QVBoxLayout;
class QLabel;
class ezCollapsibleGroupBox;
class ezAddSubElementButton;
class ezPropertyGridWidget;
class ezElementGroupButton;

class EZ_GUIFOUNDATION_DLL ezPropertyBaseWidget : public QWidget
{
  Q_OBJECT;
public:
  struct Selection
  {
    const ezDocumentObjectBase* m_pObject;
    ezVariant m_Index;
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
  explicit ezPropertyBaseWidget();
  virtual ~ezPropertyBaseWidget();

  void Init(ezPropertyGridWidget* pGrid, const ezAbstractProperty* pProp, const ezPropertyPath& path);
  const ezAbstractProperty* GetProperty() const { return m_pProp; }
  const ezPropertyPath& GetPropertyPath() const { return m_PropertyPath; }

  virtual void SetSelection(const ezHybridArray<Selection, 8>& items);
  const ezHybridArray<Selection, 8>& GetSelection() const { return m_Items; }

  virtual bool HasLabel() const { return true; }
  virtual const char* GetLabel() const { return m_pProp->GetPropertyName(); }

  static const ezRTTI* GetCommonBaseType(const ezHybridArray<ezPropertyBaseWidget::Selection, 8>& items);
  
protected:
  void Broadcast(Event::Type type);
  virtual void OnInit() = 0;

protected:
  ezPropertyGridWidget* m_pGrid;
  const ezAbstractProperty* m_pProp;
  ezPropertyPath m_PropertyPath;
  ezHybridArray<Selection, 8> m_Items;
};


class EZ_GUIFOUNDATION_DLL ezUnsupportedPropertyWidget : public ezPropertyBaseWidget
{
  Q_OBJECT;
public:
  explicit ezUnsupportedPropertyWidget(const char* szMessage = nullptr);

protected:
  virtual void OnInit() override;

  QHBoxLayout* m_pLayout;
  QLabel* m_pWidget;
  ezString m_sMessage;
};


class EZ_GUIFOUNDATION_DLL ezStandardPropertyBaseWidget : public ezPropertyBaseWidget
{
  Q_OBJECT;
public:
  explicit ezStandardPropertyBaseWidget();

  virtual void SetSelection(const ezHybridArray<Selection, 8>& items) override;

protected:
  void BroadcastValueChanged(const ezVariant& NewValue);

  const ezVariant& GetOldValue() const { return m_OldValue; }
  virtual void InternalSetValue(const ezVariant& value) = 0;

protected:
  ezVariant m_OldValue;
};


class EZ_GUIFOUNDATION_DLL ezPropertyTypeWidget : public ezPropertyBaseWidget
{
  Q_OBJECT;
public:
  explicit ezPropertyTypeWidget(bool bAddCollapsibleGroup = false);
  virtual ~ezPropertyTypeWidget();

  virtual void SetSelection(const ezHybridArray<Selection, 8>& items) override;
  virtual bool HasLabel() const override { return false; }

protected:
  virtual void OnInit() override;

protected:
  QHBoxLayout* m_pLayout;
  ezCollapsibleGroupBox* m_pGroup;
  QHBoxLayout* m_pGroupLayout;
  ezTypeWidget* m_pTypeWidget;
};


class EZ_GUIFOUNDATION_DLL ezPropertyPointerWidget : public ezPropertyBaseWidget
{
  Q_OBJECT;
public:
  explicit ezPropertyPointerWidget();
  virtual ~ezPropertyPointerWidget();

  virtual void SetSelection(const ezHybridArray<Selection, 8>& items) override;
  virtual bool HasLabel() const override { return false; }

public slots:
  void OnDeleteButtonClicked();

protected:
  virtual void OnInit() override;
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);

protected:
  QHBoxLayout* m_pLayout;
  ezCollapsibleGroupBox* m_pGroup;
  ezAddSubElementButton* m_pAddButton;
  ezElementGroupButton* m_pDeleteButton;
  QHBoxLayout* m_pGroupLayout;
  ezTypeWidget* m_pTypeWidget;
};


class EZ_GUIFOUNDATION_DLL ezPropertyContainerBaseWidget : public ezPropertyBaseWidget
{
  Q_OBJECT;
public:
  ezPropertyContainerBaseWidget();
  virtual ~ezPropertyContainerBaseWidget();

  virtual void SetSelection(const ezHybridArray<Selection, 8>& items) override;
  virtual bool HasLabel() const override { return false; }

public slots:
  void OnElementButtonClicked();

protected:
  struct Element
  {
    Element() : m_pSubGroup(nullptr), m_pWidget(nullptr) {}
    Element(ezCollapsibleGroupBox* pSubGroup, ezPropertyBaseWidget* pWidget) : m_pSubGroup(pSubGroup), m_pWidget(pWidget) {}

    ezCollapsibleGroupBox* m_pSubGroup;
    ezPropertyBaseWidget* m_pWidget;
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

protected:
  QHBoxLayout* m_pLayout;
  ezCollapsibleGroupBox* m_pGroup;
  QVBoxLayout* m_pGroupLayout;
  ezAddSubElementButton* m_pAddButton;

  ezDynamicArray<Element> m_Elements;
};


class EZ_GUIFOUNDATION_DLL ezPropertyStandardTypeContainerWidget : public ezPropertyContainerBaseWidget
{
  Q_OBJECT;
public:
  ezPropertyStandardTypeContainerWidget();
  virtual ~ezPropertyStandardTypeContainerWidget();

protected:
  virtual Element& AddElement(ezUInt32 index) override;
  virtual void RemoveElement(ezUInt32 index) override;
  virtual void UpdateElement(ezUInt32 index) override;

  void PropertyChangedHandler(const ezPropertyBaseWidget::Event& ed);
};

class EZ_GUIFOUNDATION_DLL ezPropertyTypeContainerWidget : public ezPropertyContainerBaseWidget
{
  Q_OBJECT;
public:
  ezPropertyTypeContainerWidget();
  virtual ~ezPropertyTypeContainerWidget();

protected:
  virtual void OnInit() override;
  virtual void UpdateElement(ezUInt32 index) override;

  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
};