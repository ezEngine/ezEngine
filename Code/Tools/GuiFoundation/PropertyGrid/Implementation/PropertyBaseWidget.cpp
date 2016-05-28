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
#include <CoreUtils/Localization/TranslationLookup.h>

/// *** BASE ***
ezQtPropertyWidget::ezQtPropertyWidget() : QWidget(nullptr), m_pGrid(nullptr), m_pProp(nullptr)
{
  m_bUndead = false;
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

ezQtPropertyWidget::~ezQtPropertyWidget()
{
}

void ezQtPropertyWidget::Init(ezPropertyGridWidget* pGrid, const ezAbstractProperty* pProp, const ezPropertyPath& path)
{
  m_pGrid = pGrid;
  m_pProp = pProp;
  m_PropertyPath = path;

  if (pProp->GetAttributeByType<ezReadOnlyAttribute>() != nullptr || pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
    setEnabled(false);

  OnInit();
}

void ezQtPropertyWidget::SetSelection(const ezHybridArray<Selection, 8>& items)
{
  m_Items = items;
}

const ezRTTI* ezQtPropertyWidget::GetCommonBaseType(const ezHybridArray<ezQtPropertyWidget::Selection, 8>& items)
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


void ezQtPropertyWidget::PrepareToDie()
{
  EZ_ASSERT_DEBUG(!m_bUndead, "Object has already been marked for cleanup");

  m_bUndead = true;

  DoPrepareToDie();
}

void ezQtPropertyWidget::Broadcast(Event::Type type)
{
  Event ed;
  ed.m_Type = type;
  ed.m_pPropertyPath = &m_PropertyPath;

  m_Events.Broadcast(ed);
}


/// *** ezQtUnsupportedPropertyWidget ***

ezQtUnsupportedPropertyWidget::ezQtUnsupportedPropertyWidget(const char* szMessage)
  : ezQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new QLabel(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);
  m_sMessage = szMessage;
}

void ezQtUnsupportedPropertyWidget::OnInit()
{
  QtScopedBlockSignals bs(m_pWidget);

  QString sMessage = QStringLiteral("Unsupported Type: ") % QString::fromUtf8(m_pProp->GetSpecificType()->GetTypeName());
  if (!m_sMessage.IsEmpty())
    sMessage += QStringLiteral(" (") % QString::fromUtf8(m_sMessage) % QStringLiteral(")");
  m_pWidget->setText(sMessage);
  m_pWidget->setToolTip(sMessage);
}


/// *** ezQtStandardPropertyWidget ***

ezQtStandardPropertyWidget::ezQtStandardPropertyWidget()
  : ezQtPropertyWidget()
{

}

void ezQtStandardPropertyWidget::SetSelection(const ezHybridArray<Selection, 8>& items)
{
  ezQtPropertyWidget::SetSelection(items);

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

void ezQtStandardPropertyWidget::BroadcastValueChanged(const ezVariant& NewValue)
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


/// *** ezQtPropertyPointerWidget ***

ezQtPropertyPointerWidget::ezQtPropertyPointerWidget()
  : ezQtPropertyWidget()
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
  connect(m_pDeleteButton, &QToolButton::clicked, this, &ezQtPropertyPointerWidget::OnDeleteButtonClicked);

  m_pTypeWidget = nullptr;
}

ezQtPropertyPointerWidget::~ezQtPropertyPointerWidget()
{
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyPointerWidget::StructureEventHandler, this));
}

void ezQtPropertyPointerWidget::OnInit()
{
  m_pGroup->setTitle(ezTranslate(m_pProp->GetPropertyName()));
  m_pGrid->SetCollapseState(m_pGroup);
  connect(m_pGroup, &ezCollapsibleGroupBox::CollapseStateChanged, m_pGrid, &ezPropertyGridWidget::OnCollapseStateChanged);

  // Add Buttons
  auto pAttr = m_pProp->GetAttributeByType<ezContainerAttribute>();
  m_pAddButton->setVisible(!pAttr || pAttr->CanAdd());
  m_pDeleteButton->setVisible(!pAttr || pAttr->CanDelete());

  m_pAddButton->Init(m_pGrid, m_pProp, m_PropertyPath);
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezQtPropertyPointerWidget::StructureEventHandler, this));
}

void ezQtPropertyPointerWidget::SetSelection(const ezHybridArray<Selection, 8>& items)
{
  QtScopedUpdatesDisabled _(this);

  ezQtPropertyWidget::SetSelection(items);

  if (m_pTypeWidget)
  {
    m_pGroupLayout->removeWidget(m_pTypeWidget);
    delete m_pTypeWidget;
    m_pTypeWidget = nullptr;
  }

  ezHybridArray<ezQtPropertyWidget::Selection, 8> emptyItems;
  ezHybridArray<ezQtPropertyWidget::Selection, 8> subItems;
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
      ezQtPropertyWidget::Selection sel;
      sel.m_pObject = accessor.GetOwner()->GetDocumentObjectManager()->GetObject(ObjectGuid);

      subItems.PushBack(sel);
    }
  }

  auto pAttr = m_pProp->GetAttributeByType<ezContainerAttribute>();
  if (!pAttr || pAttr->CanAdd())
    m_pAddButton->setVisible(!emptyItems.IsEmpty());
  if (!pAttr || pAttr->CanDelete())
    m_pDeleteButton->setVisible(!subItems.IsEmpty());

  if (!emptyItems.IsEmpty())
  {
    m_pAddButton->SetSelection(emptyItems);
  }

  if (!subItems.IsEmpty())
  {
    const ezRTTI* pCommonType = ezQtPropertyWidget::GetCommonBaseType(subItems);

    ezPropertyPath emptyPath;
    m_pTypeWidget = new ezTypeWidget(m_pGroup->Content, m_pGrid, pCommonType, emptyPath);
    m_pTypeWidget->SetSelection(subItems);

    m_pGroupLayout->addWidget(m_pTypeWidget);
  }
}


void ezQtPropertyPointerWidget::DoPrepareToDie()
{
  if (m_pTypeWidget)
  {
    m_pTypeWidget->PrepareToDie();
  }
}

void ezQtPropertyPointerWidget::OnDeleteButtonClicked()
{
  ezCommandHistory* history = m_pGrid->GetDocument()->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction();

  ezStatus res;
  ezRemoveObjectCommand cmd;
  const ezHybridArray<ezQtPropertyWidget::Selection, 8> selection = m_pTypeWidget->GetSelection();
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

  ezUIServices::GetSingleton()->MessageBoxStatus(res, "Removing sub-element from the property failed.");
}

void ezQtPropertyPointerWidget::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  if (IsUndead())
    return;

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
                       [&](const ezQtPropertyWidget::Selection& sel) { return e.m_pNewParent == sel.m_pObject || e.m_pPreviousParent == sel.m_pObject; }
      ))
        return;

      SetSelection(m_Items);
    }
    break;
  }
}


/// *** ezQtPropertyTypeWidget ***

ezQtPropertyTypeWidget::ezQtPropertyTypeWidget(bool bAddCollapsibleGroup)
  : ezQtPropertyWidget()
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

ezQtPropertyTypeWidget::~ezQtPropertyTypeWidget()
{
}

void ezQtPropertyTypeWidget::OnInit()
{
  if (m_pGroup)
  {
    m_pGroup->setTitle(ezTranslate(m_pProp->GetPropertyName()));
    m_pGrid->SetCollapseState(m_pGroup);
    connect(m_pGroup, &ezCollapsibleGroupBox::CollapseStateChanged, m_pGrid, &ezPropertyGridWidget::OnCollapseStateChanged);
  }
}

void ezQtPropertyTypeWidget::SetSelection(const ezHybridArray<Selection, 8>& items)
{
  QtScopedUpdatesDisabled _(this);

  ezQtPropertyWidget::SetSelection(items);

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
    pCommonType = ezQtPropertyWidget::GetCommonBaseType(m_Items);
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


void ezQtPropertyTypeWidget::DoPrepareToDie()
{
  if (m_pTypeWidget)
  {
    m_pTypeWidget->PrepareToDie();
  }
}

/// *** ezQtPropertyContainerWidget ***

ezQtPropertyContainerWidget::ezQtPropertyContainerWidget()
  : ezQtPropertyWidget()
  , m_pAddButton(nullptr)
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pGroup = new ezCollapsibleGroupBox(this);
  m_pGroupLayout = new QVBoxLayout(nullptr);
  m_pGroupLayout->setSpacing(1);
  m_pGroupLayout->setContentsMargins(5, 0, 0, 0);
  m_pGroup->setInnerLayout(m_pGroupLayout);

  m_pLayout->addWidget(m_pGroup);
}

ezQtPropertyContainerWidget::~ezQtPropertyContainerWidget()
{
  Clear();
}

void ezQtPropertyContainerWidget::SetSelection(const ezHybridArray<Selection, 8>& items)
{
  ezQtPropertyWidget::SetSelection(items);

  UpdateElements();

  if (m_pAddButton)
  {
    m_pAddButton->SetSelection(m_Items);
  }
}


void ezQtPropertyContainerWidget::DoPrepareToDie()
{
  for (const auto& e : m_Elements)
  {
    e.m_pWidget->PrepareToDie();
  }
}

void ezQtPropertyContainerWidget::OnElementButtonClicked()
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

ezQtPropertyContainerWidget::Element& ezQtPropertyContainerWidget::AddElement(ezUInt32 index)
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
  ezQtPropertyWidget* pNewWidget = bST ? ezPropertyGridWidget::CreateMemberPropertyWidget(m_pProp) : new ezQtPropertyTypeWidget();

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
      connect(pUpButton, &QToolButton::clicked, this, &ezQtPropertyContainerWidget::OnElementButtonClicked);

      ezElementGroupButton* pDownButton = new ezElementGroupButton(pSubGroup->Header, ezElementGroupButton::ElementAction::MoveElementDown, pNewWidget);
      pSubGroup->HeaderLayout->addWidget(pDownButton);
      connect(pDownButton, &QToolButton::clicked, this, &ezQtPropertyContainerWidget::OnElementButtonClicked);
    }

    if (!pAttr || pAttr->CanDelete())
    {
      ezElementGroupButton* pDeleteButton = new ezElementGroupButton(pSubGroup->Header, ezElementGroupButton::ElementAction::DeleteElement, pNewWidget);
      pSubGroup->HeaderLayout->addWidget(pDeleteButton);
      connect(pDeleteButton, &QToolButton::clicked, this, &ezQtPropertyContainerWidget::OnElementButtonClicked);
    }
  }

  m_Elements.Insert(Element(pSubGroup, pNewWidget), index);
  return m_Elements[index];
}

void ezQtPropertyContainerWidget::RemoveElement(ezUInt32 index)
{
  Element& elem = m_Elements[index];

  m_pGroupLayout->removeWidget(elem.m_pSubGroup);
  delete elem.m_pSubGroup;
  m_Elements.RemoveAt(index);
}

void ezQtPropertyContainerWidget::UpdateElements()
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

ezUInt32 ezQtPropertyContainerWidget::GetRequiredElementCount() const
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

void ezQtPropertyContainerWidget::Clear()
{
  while (m_Elements.GetCount() > 0)
  {
    RemoveElement(m_Elements.GetCount() - 1);
  }

  m_Elements.Clear();
}

void ezQtPropertyContainerWidget::OnInit()
{
  m_pGroup->setTitle(ezTranslate(m_pProp->GetPropertyName()));

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

void ezQtPropertyContainerWidget::DeleteItems(ezHybridArray<Selection, 8>& items, const ezPropertyPath& path)
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

  ezUIServices::GetSingleton()->MessageBoxStatus(res, "Removing sub-element from the property failed.");
}

void ezQtPropertyContainerWidget::MoveItems(ezHybridArray<Selection, 8>& items, const ezPropertyPath& path, ezInt32 iMove)
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

  ezUIServices::GetSingleton()->MessageBoxStatus(res, "Moving sub-element failed.");
}


/// *** ezQtPropertyStandardTypeContainerWidget ***

ezQtPropertyStandardTypeContainerWidget::ezQtPropertyStandardTypeContainerWidget()
  : ezQtPropertyContainerWidget()
{
}

ezQtPropertyStandardTypeContainerWidget::~ezQtPropertyStandardTypeContainerWidget()
{
}

ezQtPropertyContainerWidget::Element& ezQtPropertyStandardTypeContainerWidget::AddElement(ezUInt32 index)
{
  ezQtPropertyContainerWidget::Element& elem = ezQtPropertyContainerWidget::AddElement(index);
  elem.m_pWidget->m_Events.AddEventHandler(ezMakeDelegate(&ezQtPropertyStandardTypeContainerWidget::PropertyChangedHandler, this));

  return elem;
}

void ezQtPropertyStandardTypeContainerWidget::RemoveElement(ezUInt32 index)
{
  {
    Element& elem = m_Elements[index];
    elem.m_pWidget->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyStandardTypeContainerWidget::PropertyChangedHandler, this));
  }
  ezQtPropertyContainerWidget::RemoveElement(index);
}

void ezQtPropertyStandardTypeContainerWidget::UpdateElement(ezUInt32 index)
{
  Element& elem = m_Elements[index];

  ezHybridArray<ezQtPropertyWidget::Selection, 8> SubItems;

  for (const auto& item : m_Items)
  {
    ezQtPropertyWidget::Selection sel;
    sel.m_pObject = item.m_pObject;
    sel.m_Index = index;

    SubItems.PushBack(sel);
  }

  ezStringBuilder sTitle;
  sTitle.Format("[%i]", index);

  elem.m_pSubGroup->setTitle(sTitle);
  m_pGrid->SetCollapseState(elem.m_pSubGroup);
  elem.m_pWidget->SetSelection(SubItems);
}

void ezQtPropertyStandardTypeContainerWidget::PropertyChangedHandler(const ezQtPropertyWidget::Event& ed)
{
  if (IsUndead())
    return;

  // Forward from child widget to parent ezTypeWidget.
  m_Events.Broadcast(ed);
}


/// *** ezQtPropertyTypeContainerWidget ***

ezQtPropertyTypeContainerWidget::ezQtPropertyTypeContainerWidget()
{
}

ezQtPropertyTypeContainerWidget::~ezQtPropertyTypeContainerWidget()
{
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyTypeContainerWidget::StructureEventHandler, this));
}

void ezQtPropertyTypeContainerWidget::OnInit()
{
  ezQtPropertyContainerWidget::OnInit();
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezQtPropertyTypeContainerWidget::StructureEventHandler, this));

}

void ezQtPropertyTypeContainerWidget::UpdateElement(ezUInt32 index)
{
  Element& elem = m_Elements[index];

  ezHybridArray<ezQtPropertyWidget::Selection, 8> SubItems;

  for (const auto& item : m_Items)
  {
    const auto& accessor = item.m_pObject->GetTypeAccessor();

    ezUuid ObjectGuid = accessor.GetValue(m_PropertyPath, index).ConvertTo<ezUuid>();

    ezQtPropertyWidget::Selection sel;
    sel.m_pObject = accessor.GetOwner()->GetDocumentObjectManager()->GetObject(ObjectGuid);
    //sel.m_Index = // supposed to be invalid;

    SubItems.PushBack(sel);
  }

  const ezRTTI* pCommonType = ezQtPropertyWidget::GetCommonBaseType(SubItems);

  ezStringBuilder sTitle;
  sTitle.Format("[%i] - %s", index, ezTranslate(pCommonType->GetTypeName()));
  elem.m_pSubGroup->setTitle(sTitle);

  {
    ezStringBuilder sIconName;
    sIconName.Set(":/TypeIcons/", pCommonType->GetTypeName());
    elem.m_pSubGroup->Icon2->setPixmap(ezUIServices::GetCachedPixmapResource(sIconName.GetData()));
  }

  m_pGrid->SetCollapseState(elem.m_pSubGroup);
  elem.m_pWidget->SetSelection(SubItems);
}

void ezQtPropertyTypeContainerWidget::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  if (IsUndead())
    return;

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
                       [&](const ezQtPropertyWidget::Selection& sel) { return e.m_pNewParent == sel.m_pObject || e.m_pPreviousParent == sel.m_pObject; }
      ))
        return;

      UpdateElements();
    }
    break;
  }
}

