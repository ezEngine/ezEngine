#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/TypeWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/ElementGroupButton.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/Document.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <algorithm>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QStringBuilder>
#include <QLabel>

/// *** BASE ***
ezPropertyBaseWidget::ezPropertyBaseWidget() : QWidget(nullptr), m_pGrid(nullptr), m_pProp(nullptr)
{
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

ezPropertyBaseWidget::~ezPropertyBaseWidget()
{
}

void ezPropertyBaseWidget::Init(ezPropertyGridWidget* pGrid, const ezAbstractProperty* pProp, const ezPropertyPath& path)
{
  m_pGrid = pGrid;
  m_pProp = pProp;
  m_PropertyPath = path;

  if (pProp->GetAttributeByType<ezReadOnlyAttribute>() != nullptr || pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
    setEnabled(false);

  OnInit();
}

void ezPropertyBaseWidget::SetSelection(const ezHybridArray<Selection, 8>& items)
{
  m_Items = items;
}

const ezRTTI* ezPropertyBaseWidget::GetCommonBaseType(const ezHybridArray<ezPropertyBaseWidget::Selection, 8>& items)
{
  const ezRTTI* pSubtype = nullptr;

  for (const auto& item : items)
  {
    const auto& accessor = item.m_pObject->GetTypeAccessor();

    if (pSubtype == nullptr)
      pSubtype = accessor.GetType();
    else
    {
      pSubtype = ezReflectionUtils::GetCommonBaseType(pSubtype, accessor.GetType());
    }
  }

  return pSubtype;
}

void ezPropertyBaseWidget::Broadcast(Event::Type type)
{
  Event ed;
  ed.m_Type = type;
  ed.m_pPropertyPath = &m_PropertyPath;

  m_Events.Broadcast(ed);
}


/// *** ezUnsupportedPropertyWidget ***

ezUnsupportedPropertyWidget::ezUnsupportedPropertyWidget(const char* szMessage)
  : ezPropertyBaseWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new QLabel(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);
  m_sMessage = szMessage;
}

void ezUnsupportedPropertyWidget::OnInit()
{
  QString sMessage = QStringLiteral("Unsupported Type: ") % QString::fromUtf8(m_pProp->GetSpecificType()->GetTypeName());
  if (!m_sMessage.IsEmpty())
    sMessage += QStringLiteral(" (") % QString::fromUtf8(m_sMessage) % QStringLiteral(")");
  m_pWidget->setText(sMessage);
  m_pWidget->setToolTip(sMessage);
}


/// *** ezStandardPropertyBaseWidget ***

ezStandardPropertyBaseWidget::ezStandardPropertyBaseWidget()
  : ezPropertyBaseWidget()
{

}

void ezStandardPropertyBaseWidget::SetSelection(const ezHybridArray<Selection, 8>& items)
{
  ezPropertyBaseWidget::SetSelection(items);

  ezVariant value;
  // check if we have multiple values
  for (const auto& item : items)
  {
    const ezIReflectedTypeAccessor& et = item.m_pObject->GetTypeAccessor();

    if (!value.IsValid())
      value = et.GetValue(m_PropertyPath, item.m_Index);
    else
    {
      if (value != et.GetValue(m_PropertyPath, item.m_Index))
      {
        value = ezVariant();
        break;
      }
    }
  }

  m_OldValue = value;
  InternalSetValue(value);
}

void ezStandardPropertyBaseWidget::BroadcastValueChanged(const ezVariant& NewValue)
{
  if (NewValue == m_OldValue)
    return;

  m_OldValue = NewValue;

  Event ed;
  ed.m_Type = Event::Type::ValueChanged;
  ed.m_pPropertyPath = &m_PropertyPath;
  ed.m_Value = NewValue;
  ed.m_pItems = &m_Items;

  m_Events.Broadcast(ed);
}


/// *** ezPropertyPointerWidget ***

ezPropertyPointerWidget::ezPropertyPointerWidget()
  : ezPropertyBaseWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pGroup = new ezCollapsibleGroupBox(this);
  m_pGroupLayout = new QHBoxLayout(nullptr);
  m_pGroupLayout->setSpacing(1);
  m_pGroupLayout->setContentsMargins(5, 0, 0, 0);
  m_pGroup->setInnerLayout(m_pGroupLayout);

  m_pLayout->addWidget(m_pGroup);

  m_pAddButton = new ezAddSubElementButton();
  m_pGroup->HeaderLayout->addWidget(m_pAddButton);

  m_pDeleteButton = new ezElementGroupButton(m_pGroup->Header, ezElementGroupButton::ElementAction::DeleteElement, this);
  m_pGroup->HeaderLayout->addWidget(m_pDeleteButton);
  connect(m_pDeleteButton, &QToolButton::clicked, this, &ezPropertyPointerWidget::OnDeleteButtonClicked);

  m_pTypeWidget = nullptr;
}

ezPropertyPointerWidget::~ezPropertyPointerWidget()
{
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezPropertyPointerWidget::StructureEventHandler, this));
}

void ezPropertyPointerWidget::OnInit()
{
  m_pGroup->setTitle(QString::fromUtf8(m_pProp->GetPropertyName()));
  m_pGrid->SetCollapseState(m_pGroup);
  connect(m_pGroup, &ezCollapsibleGroupBox::CollapseStateChanged, m_pGrid, &ezPropertyGridWidget::OnCollapseStateChanged);

  m_pAddButton->Init(m_pGrid, m_pProp, m_PropertyPath);

  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezPropertyPointerWidget::StructureEventHandler, this));
}

void ezPropertyPointerWidget::SetSelection(const ezHybridArray<Selection, 8>& items)
{
  QtScopedUpdatesDisabled _(this);

  ezPropertyBaseWidget::SetSelection(items);

  if (m_pTypeWidget)
  {
    m_pGroupLayout->removeWidget(m_pTypeWidget);
    delete m_pTypeWidget;
    m_pTypeWidget = nullptr;
  }

  ezHybridArray<ezPropertyBaseWidget::Selection, 8> emptyItems;
  ezHybridArray<ezPropertyBaseWidget::Selection, 8> subItems;
  for (const auto& item : m_Items)
  {
    const auto& accessor = item.m_pObject->GetTypeAccessor();

    ezUuid ObjectGuid = accessor.GetValue(m_PropertyPath).ConvertTo<ezUuid>();
    if (!ObjectGuid.IsValid())
    {
      emptyItems.PushBack(item);
    }
    else
    {
      ezPropertyBaseWidget::Selection sel;
      sel.m_pObject = accessor.GetOwner()->GetDocumentObjectManager()->GetObject(ObjectGuid);

      subItems.PushBack(sel);
    }
  }

  m_pAddButton->setVisible(!emptyItems.IsEmpty());
  m_pDeleteButton->setVisible(!subItems.IsEmpty());

  if (!emptyItems.IsEmpty())
  {
    m_pAddButton->SetSelection(emptyItems);
  }

  if (!subItems.IsEmpty())
  {
    const ezRTTI* pCommonType = ezPropertyBaseWidget::GetCommonBaseType(subItems);

    ezPropertyPath emptyPath;
    m_pTypeWidget = new ezTypeWidget(m_pGroup->Content, m_pGrid, pCommonType, emptyPath);
    m_pTypeWidget->SetSelection(subItems);

    m_pGroupLayout->addWidget(m_pTypeWidget);
  }
}

void ezPropertyPointerWidget::OnDeleteButtonClicked()
{
  ezCommandHistory* history = m_pGrid->GetDocument()->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction();

  ezStatus res;
  ezRemoveObjectCommand cmd;
  const ezHybridArray<ezPropertyBaseWidget::Selection, 8> selection = m_pTypeWidget->GetSelection();
  for (auto& item : selection)
  {
    cmd.m_Object = item.m_pObject->GetGuid();
    res = history->AddCommand(cmd);
    if (res.m_Result.Failed())
      break;
  }

  if (res.m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  ezUIServices::GetInstance()->MessageBoxStatus(res, "Removing sub-element from the property failed.");
}

void ezPropertyPointerWidget::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
  case ezDocumentObjectStructureEvent::Type::AfterObjectMoved:
  case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      ezStringBuilder sPath = m_PropertyPath.GetPathString();
      if (!e.m_sParentProperty.IsEqual(sPath))
        return;

      if (std::none_of(cbegin(m_Items), cend(m_Items),
        [&](const ezPropertyBaseWidget::Selection& sel) { return e.m_pNewParent == sel.m_pObject || e.m_pPreviousParent == sel.m_pObject; }
        ))
        return;

      SetSelection(m_Items);
    }
    break;
  }
}


/// *** ezPropertyTypeWidget ***

ezPropertyTypeWidget::ezPropertyTypeWidget(bool bAddCollapsibleGroup)
  : ezPropertyBaseWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);
  m_pGroup = nullptr;
  m_pGroupLayout = nullptr;

  if (bAddCollapsibleGroup)
  {
    m_pGroup = new ezCollapsibleGroupBox(this);
    m_pGroupLayout = new QHBoxLayout(nullptr);
    m_pGroupLayout->setSpacing(1);
    m_pGroupLayout->setContentsMargins(5, 0, 0, 0);
    m_pGroup->setInnerLayout(m_pGroupLayout);

    m_pLayout->addWidget(m_pGroup);
  }
  m_pTypeWidget = nullptr;
}

ezPropertyTypeWidget::~ezPropertyTypeWidget()
{
}

void ezPropertyTypeWidget::OnInit()
{
  if (m_pGroup)
  {
    m_pGroup->setTitle(QString::fromUtf8(m_pProp->GetPropertyName()));
    m_pGrid->SetCollapseState(m_pGroup);
    connect(m_pGroup, &ezCollapsibleGroupBox::CollapseStateChanged, m_pGrid, &ezPropertyGridWidget::OnCollapseStateChanged);
  }
}

void ezPropertyTypeWidget::SetSelection(const ezHybridArray<Selection, 8>& items)
{
  QtScopedUpdatesDisabled _(this);

  ezPropertyBaseWidget::SetSelection(items);

  QHBoxLayout* pLayout = m_pGroup != nullptr ? m_pGroupLayout : m_pLayout;
  QWidget* pOwner = m_pGroup != nullptr ? m_pGroup->Content : this;
  if (m_pTypeWidget)
  {
    pLayout->removeWidget(m_pTypeWidget);
    delete m_pTypeWidget;
    m_pTypeWidget = nullptr;
  }
  const ezRTTI* pCommonType = nullptr;
  if (m_PropertyPath.IsEmpty())
  {
    // The selection can have
    pCommonType = ezPropertyBaseWidget::GetCommonBaseType(m_Items);
  }
  else
  {
    // If we create a widget for a member struct we already determined the common base type at the parent type widget.
    // As we are not dealing with a pointer in this case the type must match the property exactly.
    pCommonType = m_pProp->GetSpecificType();
  }
  m_pTypeWidget = new ezTypeWidget(pOwner, m_pGrid, pCommonType, m_PropertyPath);
  m_pTypeWidget->SetSelection(m_Items);

  pLayout->addWidget(m_pTypeWidget);
}

/// *** ezPropertyContainerBaseWidget ***

ezPropertyContainerBaseWidget::ezPropertyContainerBaseWidget()
  : ezPropertyBaseWidget()
  , m_pAddButton(nullptr)
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pGroup = new ezCollapsibleGroupBox(this);
  m_pGroupLayout = new QVBoxLayout(NULL);
  m_pGroupLayout->setSpacing(1);
  m_pGroupLayout->setContentsMargins(5, 0, 0, 0);
  m_pGroup->setInnerLayout(m_pGroupLayout);

  m_pLayout->addWidget(m_pGroup);
}

ezPropertyContainerBaseWidget::~ezPropertyContainerBaseWidget()
{
  Clear();
}

void ezPropertyContainerBaseWidget::SetSelection(const ezHybridArray<Selection, 8>& items)
{
  ezPropertyBaseWidget::SetSelection(items);

  UpdateElements();

  if (m_pAddButton)
  {
    m_pAddButton->SetSelection(m_Items);
  }
}

void ezPropertyContainerBaseWidget::OnElementButtonClicked()
{
  ezElementGroupButton* pButton = qobject_cast<ezElementGroupButton*>(sender());
  const ezPropertyPath path = pButton->GetGroupWidget()->GetPropertyPath();
  ezHybridArray<Selection, 8> items = pButton->GetGroupWidget()->GetSelection();

  switch (pButton->GetAction())
  {
  case ezElementGroupButton::ElementAction::MoveElementUp:
    {
      MoveItems(items, path, -1);
    }
    break;
  case ezElementGroupButton::ElementAction::MoveElementDown:
    {
      MoveItems(items, path, 2);
    }
    break;
  case ezElementGroupButton::ElementAction::DeleteElement:
    {
      DeleteItems(items, path);
    }
    break;
  }
}

ezPropertyContainerBaseWidget::Element& ezPropertyContainerBaseWidget::AddElement(ezUInt32 index)
{
  ezCollapsibleGroupBox* pSubGroup = new ezCollapsibleGroupBox(m_pGroup);
  connect(pSubGroup, &ezCollapsibleGroupBox::CollapseStateChanged, m_pGrid, &ezPropertyGridWidget::OnCollapseStateChanged);

  pSubGroup->SetFillColor(palette().window().color());

  QVBoxLayout* pSubLayout = new QVBoxLayout(nullptr);
  pSubLayout->setContentsMargins(5, 0, 0, 0);
  pSubLayout->setSpacing(1);
  pSubGroup->setInnerLayout(pSubLayout);

  m_pGroupLayout->insertWidget((int)index, pSubGroup);

  bool bST = m_pProp->GetFlags().IsSet(ezPropertyFlags::StandardType);
  ezPropertyBaseWidget* pNewWidget = bST ? ezPropertyGridWidget::CreateMemberPropertyWidget(m_pProp) : new ezPropertyTypeWidget();

  pNewWidget->setParent(pSubGroup);
  pSubLayout->addWidget(pNewWidget);

  if (bST)
    pNewWidget->Init(m_pGrid, m_pProp, m_PropertyPath);
  else
    // As each type widget will be filled with new objects the path starts anew.
    pNewWidget->Init(m_pGrid, m_pProp, "");

  {
    // Add Buttons
    auto pAttr = m_pProp->GetAttributeByType<ezContainerAttribute>();
    if (!pAttr || pAttr->CanMove())
    {
      ezElementGroupButton* pUpButton = new ezElementGroupButton(pSubGroup->Header, ezElementGroupButton::ElementAction::MoveElementUp, pNewWidget);
      pSubGroup->HeaderLayout->addWidget(pUpButton);
      connect(pUpButton, &QToolButton::clicked, this, &ezPropertyContainerBaseWidget::OnElementButtonClicked);

      ezElementGroupButton* pDownButton = new ezElementGroupButton(pSubGroup->Header, ezElementGroupButton::ElementAction::MoveElementDown, pNewWidget);
      pSubGroup->HeaderLayout->addWidget(pDownButton);
      connect(pDownButton, &QToolButton::clicked, this, &ezPropertyContainerBaseWidget::OnElementButtonClicked);
    }

    if (!pAttr || pAttr->CanDelete())
    {
      ezElementGroupButton* pDeleteButton = new ezElementGroupButton(pSubGroup->Header, ezElementGroupButton::ElementAction::DeleteElement, pNewWidget);
      pSubGroup->HeaderLayout->addWidget(pDeleteButton);
      connect(pDeleteButton, &QToolButton::clicked, this, &ezPropertyContainerBaseWidget::OnElementButtonClicked);
    }
  }

  m_Elements.Insert(Element(pSubGroup, pNewWidget), index);
  return m_Elements[index];
}

void ezPropertyContainerBaseWidget::RemoveElement(ezUInt32 index)
{
  Element& elem = m_Elements[index];

  m_pGroupLayout->removeWidget(elem.m_pSubGroup);
  delete elem.m_pSubGroup;
  m_Elements.RemoveAt(index);
}

void ezPropertyContainerBaseWidget::UpdateElements()
{
  QtScopedUpdatesDisabled _(this);

  ezUInt32 iElements = GetRequiredElementCount();

  while (m_Elements.GetCount() > iElements)
  {
    RemoveElement(m_Elements.GetCount() - 1);
  }
  while (m_Elements.GetCount() < iElements)
  {
    AddElement(m_Elements.GetCount());
  }

  for (ezUInt32 i = 0; i < iElements; ++i)
  {
    UpdateElement(i);
  }

  // Force re-layout of parent hierarchy to prevent flicker.
  QWidget* pCur = m_pGroup;
  while (pCur != nullptr && qobject_cast<QScrollArea*>(pCur) == nullptr)
  {
    pCur->updateGeometry();
    pCur = pCur->parentWidget();
  }
}

ezUInt32 ezPropertyContainerBaseWidget::GetRequiredElementCount() const
{
  ezInt32 iElements = 0x7FFFFFFF;
  for (const auto& item : m_Items)
  {
    const auto& accessor = item.m_pObject->GetTypeAccessor();

    iElements = ezMath::Min(iElements, accessor.GetCount(m_PropertyPath));
  }
  EZ_ASSERT_DEV(iElements >= 0, "Mismatch between storage and RTTI (%i)", iElements);
  return ezUInt32(iElements);
}

void ezPropertyContainerBaseWidget::Clear()
{
  while (m_Elements.GetCount() > 0)
  {
    RemoveElement(m_Elements.GetCount() - 1);
  }

  m_Elements.Clear();
}

void ezPropertyContainerBaseWidget::OnInit()
{
  m_pGroup->setTitle(QString::fromUtf8(m_pProp->GetPropertyName()));

  const ezContainerAttribute* pArrayAttr = m_pProp->GetAttributeByType<ezContainerAttribute>();
  if (!pArrayAttr || pArrayAttr->CanAdd())
  {
    m_pAddButton = new ezAddSubElementButton();
    m_pAddButton->Init(m_pGrid, m_pProp, m_PropertyPath);
    m_pGroup->HeaderLayout->addWidget(m_pAddButton);
  }

  m_pGrid->SetCollapseState(m_pGroup);
  connect(m_pGroup, &ezCollapsibleGroupBox::CollapseStateChanged, m_pGrid, &ezPropertyGridWidget::OnCollapseStateChanged);
}

void ezPropertyContainerBaseWidget::DeleteItems(ezHybridArray<Selection, 8>& items, const ezPropertyPath& path)
{
  ezCommandHistory* history = m_pGrid->GetDocument()->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction();

  ezStatus res;
  if (m_pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
  {
    ezRemoveObjectPropertyCommand cmd;
    cmd.SetPropertyPath(path.GetPathString());

    for (auto& item : items)
    {
      cmd.m_Object = item.m_pObject->GetGuid();
      cmd.m_Index = item.m_Index;
      res = history->AddCommand(cmd);
      if (res.m_Result.Failed())
        break;
    }
  }
  else
  {
    ezRemoveObjectCommand cmd;

    for (auto& item : items)
    {
      cmd.m_Object = item.m_pObject->GetGuid();
      res = history->AddCommand(cmd);
      if (res.m_Result.Failed())
        break;
    }
  }

  if (res.m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  ezUIServices::GetInstance()->MessageBoxStatus(res, "Removing sub-element from the property failed.");
}

void ezPropertyContainerBaseWidget::MoveItems(ezHybridArray<Selection, 8>& items, const ezPropertyPath& path, ezInt32 iMove)
{
  ezCommandHistory* history = m_pGrid->GetDocument()->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction();

  ezStatus res(EZ_SUCCESS);

  if (m_pProp != nullptr && m_pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
  {
    ezMoveObjectPropertyCommand cmd;

    for (auto& item : items)
    {
      ezInt32 iCurIndex = item.m_Index.ConvertTo<ezInt32>() + iMove;
      if (iCurIndex < 0 || iCurIndex > item.m_pObject->GetTypeAccessor().GetCount(path))
        continue;

      cmd.m_Object = item.m_pObject->GetGuid();
      cmd.m_sPropertyPath = path.GetPathString();
      cmd.m_OldIndex = item.m_Index;
      cmd.m_NewIndex = iCurIndex;

      res = history->AddCommand(cmd);
      if (res.m_Result.Failed())
        break;
    }
  }
  else
  {
    ezMoveObjectCommand cmd;

    for (auto& item : items)
    {
      ezInt32 iCurIndex = item.m_pObject->GetPropertyIndex().ConvertTo<ezInt32>() + iMove;
      if (iCurIndex < 0 || iCurIndex > item.m_pObject->GetParent()->GetTypeAccessor().GetCount(item.m_pObject->GetParentProperty()))
        continue;

      cmd.m_NewParent = item.m_pObject->GetParent()->GetGuid();
      cmd.m_Object = item.m_pObject->GetGuid();
      cmd.m_sParentProperty = item.m_pObject->GetParentProperty();
      cmd.m_Index = iCurIndex;

      res = history->AddCommand(cmd);
      if (res.m_Result.Failed())
        break;
    }
  }

  if (res.m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  ezUIServices::GetInstance()->MessageBoxStatus(res, "Moving sub-element failed.");
}


/// *** ezPropertyStandardTypeContainerWidget ***

ezPropertyStandardTypeContainerWidget::ezPropertyStandardTypeContainerWidget()
  : ezPropertyContainerBaseWidget()
{
}

ezPropertyStandardTypeContainerWidget::~ezPropertyStandardTypeContainerWidget()
{
}

ezPropertyContainerBaseWidget::Element& ezPropertyStandardTypeContainerWidget::AddElement(ezUInt32 index)
{
  ezPropertyContainerBaseWidget::Element& elem = ezPropertyContainerBaseWidget::AddElement(index);
  elem.m_pWidget->m_Events.AddEventHandler(ezMakeDelegate(&ezPropertyStandardTypeContainerWidget::PropertyChangedHandler, this));

  return elem;
}

void ezPropertyStandardTypeContainerWidget::RemoveElement(ezUInt32 index)
{
  {
    Element& elem = m_Elements[index];
    elem.m_pWidget->m_Events.RemoveEventHandler(ezMakeDelegate(&ezPropertyStandardTypeContainerWidget::PropertyChangedHandler, this));
  }
  ezPropertyContainerBaseWidget::RemoveElement(index);
}

void ezPropertyStandardTypeContainerWidget::UpdateElement(ezUInt32 index)
{
  Element& elem = m_Elements[index];

  ezHybridArray<ezPropertyBaseWidget::Selection, 8> SubItems;

  for (const auto& item : m_Items)
  {
    ezPropertyBaseWidget::Selection sel;
    sel.m_pObject = item.m_pObject;
    sel.m_Index = index;

    SubItems.PushBack(sel);
  }

  elem.m_pSubGroup->setTitle(QLatin1String("[") + QString::number(index) + QLatin1String("]"));
  m_pGrid->SetCollapseState(elem.m_pSubGroup);
  elem.m_pWidget->SetSelection(SubItems);
}

void ezPropertyStandardTypeContainerWidget::PropertyChangedHandler(const ezPropertyBaseWidget::Event& ed)
{
  // Forward from child widget to parent ezTypeWidget.
  m_Events.Broadcast(ed);
}


/// *** ezPropertyTypeContainerWidget ***

ezPropertyTypeContainerWidget::ezPropertyTypeContainerWidget()
{
}

ezPropertyTypeContainerWidget::~ezPropertyTypeContainerWidget()
{
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezPropertyTypeContainerWidget::StructureEventHandler, this));
}

void ezPropertyTypeContainerWidget::OnInit()
{
  ezPropertyContainerBaseWidget::OnInit();
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezPropertyTypeContainerWidget::StructureEventHandler, this));

}

void ezPropertyTypeContainerWidget::UpdateElement(ezUInt32 index)
{
  Element& elem = m_Elements[index];

  ezHybridArray<ezPropertyBaseWidget::Selection, 8> SubItems;

  for (const auto& item : m_Items)
  {
    const auto& accessor = item.m_pObject->GetTypeAccessor();

    ezUuid ObjectGuid = accessor.GetValue(m_PropertyPath, index).ConvertTo<ezUuid>();

    ezPropertyBaseWidget::Selection sel;
    sel.m_pObject = accessor.GetOwner()->GetDocumentObjectManager()->GetObject(ObjectGuid);
    //sel.m_Index = // supposed to be invalid;

    SubItems.PushBack(sel);
  }

  const ezRTTI* pCommonType = ezPropertyBaseWidget::GetCommonBaseType(SubItems);
  elem.m_pSubGroup->setTitle(QLatin1String("[") + QString::number(index) + QLatin1String("] - ") + QString::fromUtf8(pCommonType->GetTypeName()));
  m_pGrid->SetCollapseState(elem.m_pSubGroup);
  elem.m_pWidget->SetSelection(SubItems);
}

void ezPropertyTypeContainerWidget::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
  case ezDocumentObjectStructureEvent::Type::AfterObjectMoved:
  case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      ezStringBuilder sPath = m_PropertyPath.GetPathString();
      if (!e.m_sParentProperty.IsEqual(sPath))
        return;

      if (std::none_of(cbegin(m_Items), cend(m_Items),
        [&](const ezPropertyBaseWidget::Selection& sel) { return e.m_pNewParent == sel.m_pObject || e.m_pPreviousParent == sel.m_pObject; }
        ))
        return;

      UpdateElements();
    }
    break;
  }
}

