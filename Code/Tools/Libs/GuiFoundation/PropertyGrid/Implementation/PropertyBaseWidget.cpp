#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/ElementGroupButton.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <GuiFoundation/Widgets/InlinedGroupBox.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

#include <GuiFoundation/GuiFoundationDLL.h>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QScrollArea>
#include <QStringBuilder>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezPropertyClipboard, ezNoBase, 1, ezRTTIDefaultAllocator<ezPropertyClipboard>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("m_Type", m_Type),
    EZ_MEMBER_PROPERTY("m_Value", m_Value),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

/// *** BASE ***
ezQtPropertyWidget::ezQtPropertyWidget()
  : QWidget(nullptr)

{
  m_bUndead = false;
  m_bIsDefault = true;
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

ezQtPropertyWidget::~ezQtPropertyWidget() = default;

void ezQtPropertyWidget::Init(ezQtPropertyGridWidget* pGrid, ezObjectAccessorBase* pObjectAccessor, const ezRTTI* pType, const ezAbstractProperty* pProp)
{
  m_pGrid = pGrid;
  m_pObjectAccessor = pObjectAccessor;
  m_pType = pType;
  m_pProp = pProp;
  EZ_ASSERT_DEBUG(m_pGrid && m_pObjectAccessor && m_pType && m_pProp, "");

  if (pProp->GetAttributeByType<ezReadOnlyAttribute>() != nullptr || pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
  {
    SetReadOnly();
  }

  OnInit();
}

void ezQtPropertyWidget::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  m_Items = items;
}

const char* ezQtPropertyWidget::GetLabel(ezStringBuilder& ref_sTmp) const
{
  ref_sTmp.Set(m_pType->GetTypeName(), "::", m_pProp->GetPropertyName());
  return ref_sTmp;
}

void ezQtPropertyWidget::ExtendContextMenu(QMenu& m)
{
  m.setToolTipsVisible(true);
  // revert
  {
    QAction* pRevert = m.addAction("Revert to Default");
    pRevert->setEnabled(!m_bIsDefault);
    connect(pRevert, &QAction::triggered, this, [this]()
      {
      m_pObjectAccessor->StartTransaction("Revert to Default");

      switch (m_pProp->GetCategory())
      {
        case ezPropertyCategory::Enum::Array:
        case ezPropertyCategory::Enum::Set:
        case ezPropertyCategory::Enum::Map:
        {

          ezStatus res = ezStatus(EZ_SUCCESS);
          if (!m_Items[0].m_Index.IsValid())
          {
            // Revert container
            ezDefaultContainerState defaultState(m_pObjectAccessor, m_Items, m_pProp->GetPropertyName());
            res = defaultState.RevertContainer();
          }
          else
          {
            const bool bIsValueType = ezReflectionUtils::IsValueType(m_pProp) || m_pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags);
            if (bIsValueType)
            {
              // Revert container value type element
              ezDefaultContainerState defaultState(m_pObjectAccessor, m_Items, m_pProp->GetPropertyName());
              res = defaultState.RevertElement({});
            }
            else
            {
              // Revert objects pointed to by the object type element
              ezHybridArray<ezPropertySelection, 8> ResolvedObjects;
              for (const auto& item : m_Items)
              {
                ezUuid ObjectGuid = m_pObjectAccessor->Get<ezUuid>(item.m_pObject, m_pProp, item.m_Index);
                if (ObjectGuid.IsValid())
                {
                  ResolvedObjects.PushBack({m_pObjectAccessor->GetObject(ObjectGuid), ezVariant()});
                }
              }
              ezDefaultObjectState defaultState(m_pObjectAccessor, ResolvedObjects);
              res = defaultState.RevertObject();
            }
          }
          if (res.Failed())
          {
            res.LogFailure();
            m_pObjectAccessor->CancelTransaction();
            return;
          }
        }
        break;
        default:
        {
          // Revert object member property
          ezDefaultObjectState defaultState(m_pObjectAccessor, m_Items);
          ezStatus res = defaultState.RevertProperty(m_pProp);
          if (res.Failed())
          {
            res.LogFailure();
            m_pObjectAccessor->CancelTransaction();
            return;
          }
        }
        break;
      }
      m_pObjectAccessor->FinishTransaction(); });
  }

  const char* szMimeType = "application/ezEditor.Property";
  bool bValueType = ezReflectionUtils::IsValueType(m_pProp) || m_pProp->GetFlags().IsAnySet(ezPropertyFlags::Bitflags | ezPropertyFlags::IsEnum);
  // Copy
  {
    ezVariant commonValue = GetCommonValue(m_Items, m_pProp);
    QAction* pCopy = m.addAction("Copy Value");
    if (!bValueType)
    {
      pCopy->setEnabled(false);
      pCopy->setToolTip("Not a value type");
    }
    else if (!commonValue.IsValid())
    {
      pCopy->setEnabled(false);
      pCopy->setToolTip("No common value in selection");
    }

    connect(pCopy, &QAction::triggered, this, [this, szMimeType, commonValue]()
      {
      ezPropertyClipboard content;
      content.m_Type = m_pProp->GetSpecificType()->GetTypeName();
      content.m_Value = commonValue;

      // Serialize
      ezContiguousMemoryStreamStorage streamStorage;
      ezMemoryStreamWriter memoryWriter(&streamStorage);
      ezReflectionSerializer::WriteObjectToDDL(memoryWriter, ezGetStaticRTTI<ezPropertyClipboard>(), &content);
      memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

      // Write to clipboard
      QClipboard* clipboard = QApplication::clipboard();
      QMimeData* mimeData = new QMimeData();
      QByteArray encodedData((const char*)streamStorage.GetData(), streamStorage.GetStorageSize32());

      mimeData->setData(szMimeType, encodedData);
      mimeData->setText(QString::fromUtf8((const char*)streamStorage.GetData()));
      clipboard->setMimeData(mimeData); });
  }

  // Paste
  {
    QAction* pPaste = m.addAction("Paste Value");

    QClipboard* clipboard = QApplication::clipboard();
    auto mimedata = clipboard->mimeData();

    if (!bValueType)
    {
      pPaste->setEnabled(false);
      pPaste->setToolTip("Not a value type");
    }
    else if (!isEnabled())
    {
      pPaste->setEnabled(false);
      pPaste->setToolTip("Property is read only");
    }
    else if (!mimedata->hasFormat(szMimeType))
    {
      pPaste->setEnabled(false);
      pPaste->setToolTip("No property in clipboard");
    }
    else
    {
      QByteArray ba = mimedata->data(szMimeType);
      ezRawMemoryStreamReader memoryReader(ba.data(), ba.size());

      ezPropertyClipboard content;
      ezReflectionSerializer::ReadObjectPropertiesFromDDL(memoryReader, *ezGetStaticRTTI<ezPropertyClipboard>(), &content);

      const bool bIsArray = m_pProp->GetCategory() == ezPropertyCategory::Array || m_pProp->GetCategory() == ezPropertyCategory::Set;
      const ezRTTI* pClipboardType = ezRTTI::FindTypeByName(content.m_Type);
      const bool bIsEnumeration = pClipboardType && (pClipboardType->IsDerivedFrom<ezEnumBase>() || pClipboardType->IsDerivedFrom<ezBitflagsBase>() || m_pProp->GetSpecificType()->IsDerivedFrom<ezEnumBase>() || m_pProp->GetSpecificType()->IsDerivedFrom<ezBitflagsBase>());
      const bool bEnumerationMissmatch = bIsEnumeration ? pClipboardType != m_pProp->GetSpecificType() : false;
      const ezResult clamped = ezReflectionUtils::ClampValue(content.m_Value, m_pProp->GetAttributeByType<ezClampValueAttribute>());

      if (content.m_Value.IsA<ezVariantArray>() != bIsArray)
      {
        pPaste->setEnabled(false);
        ezStringBuilder sTemp;
        sTemp.SetFormat("Cannot convert clipboard and property content between arrays and members.");
        pPaste->setToolTip(sTemp.GetData());
      }
      else if (bEnumerationMissmatch || (!content.m_Value.CanConvertTo(m_pProp->GetSpecificType()->GetVariantType()) && content.m_Type != m_pProp->GetSpecificType()->GetTypeName()))
      {
        pPaste->setEnabled(false);
        ezStringBuilder sTemp;
        sTemp.SetFormat("Cannot convert clipboard of type '{}' to property of type '{}'", content.m_Type, m_pProp->GetSpecificType()->GetTypeName());
        pPaste->setToolTip(sTemp.GetData());
      }
      else if (clamped.Failed())
      {
        pPaste->setEnabled(false);
        ezStringBuilder sTemp;
        sTemp.SetFormat("The member property '{}' has an ezClampValueAttribute but ezReflectionUtils::ClampValue failed.", m_pProp->GetPropertyName());
      }

      connect(pPaste, &QAction::triggered, this, [this, content]()
        {
        m_pObjectAccessor->StartTransaction("Paste Value");
        if (content.m_Value.IsA<ezVariantArray>())
        {
          const ezVariantArray& values = content.m_Value.Get<ezVariantArray>();
          for (const ezPropertySelection& sel : m_Items)
          {
            if (m_pObjectAccessor->ClearByName(sel.m_pObject, m_pProp->GetPropertyName()).Failed())
            {
              m_pObjectAccessor->CancelTransaction();
              return;
            }
            for (const ezVariant& val : values)
            {
              if (m_pObjectAccessor->InsertValue(sel.m_pObject, m_pProp, val, -1).Failed())
              {
                m_pObjectAccessor->CancelTransaction();
                return;
              }
            }
          }
        }
        else
        {
          for (const ezPropertySelection& sel : m_Items)
          {
            if (m_pObjectAccessor->SetValue(sel.m_pObject, m_pProp, content.m_Value, sel.m_Index).Failed())
            {
              m_pObjectAccessor->CancelTransaction();
              return;
            }
          }
        }

        m_pObjectAccessor->FinishTransaction(); });
    }
  }

  // copy internal name
  {
    auto lambda = [this]()
    {
      QClipboard* clipboard = QApplication::clipboard();
      QMimeData* mimeData = new QMimeData();
      mimeData->setText(m_pProp->GetPropertyName());
      clipboard->setMimeData(mimeData);

      ezQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(
        ezFmt("Copied Property Name: {}", m_pProp->GetPropertyName()), ezTime::MakeFromSeconds(5));
    };

    QAction* pAction = m.addAction("Copy Internal Property Name:");
    connect(pAction, &QAction::triggered, this, lambda);

    QAction* pAction2 = m.addAction(m_pProp->GetPropertyName());
    connect(pAction2, &QAction::triggered, this, lambda);
  }
}

const ezRTTI* ezQtPropertyWidget::GetCommonBaseType(const ezHybridArray<ezPropertySelection, 8>& items)
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

QColor ezQtPropertyWidget::SetPaletteBackgroundColor(ezColorGammaUB inputColor, QPalette& ref_palette)
{
  QColor qColor = qApp->palette().color(QPalette::Window);
  if (inputColor.a != 0)
  {
    const ezColor paletteColorLinear = qtToEzColor(qColor);
    const ezColor inputColorLinear = inputColor;

    ezColor blendedColor = ezMath::Lerp(paletteColorLinear, inputColorLinear, inputColorLinear.a);
    blendedColor.a = 1.0f;
    qColor = ezToQtColor(blendedColor);
  }

  ref_palette.setBrush(QPalette::Window, QBrush(qColor, Qt::SolidPattern));
  return qColor;
}

bool ezQtPropertyWidget::GetCommonVariantSubType(
  const ezHybridArray<ezPropertySelection, 8>& items, const ezAbstractProperty* pProperty, ezVariantType::Enum& out_type)
{
  bool bFirst = true;
  // check if we have multiple values
  for (const auto& item : items)
  {
    if (bFirst)
    {
      bFirst = false;
      ezVariant value;
      m_pObjectAccessor->GetValue(item.m_pObject, pProperty, value, item.m_Index).AssertSuccess();
      out_type = value.GetType();
    }
    else
    {
      ezVariant valueNext;
      m_pObjectAccessor->GetValue(item.m_pObject, pProperty, valueNext, item.m_Index).AssertSuccess();
      if (valueNext.GetType() != out_type)
      {
        out_type = ezVariantType::Invalid;
        return false;
      }
    }
  }
  return true;
}

ezVariant ezQtPropertyWidget::GetCommonValue(const ezHybridArray<ezPropertySelection, 8>& items, const ezAbstractProperty* pProperty)
{
  if (!items[0].m_Index.IsValid() && (m_pProp->GetCategory() == ezPropertyCategory::Array || m_pProp->GetCategory() == ezPropertyCategory::Set))
  {
    ezVariantArray values;
    // check if we have multiple values
    for (ezUInt32 i = 0; i < items.GetCount(); i++)
    {
      const auto& item = items[i];
      if (i == 0)
      {
        m_pObjectAccessor->GetValues(item.m_pObject, pProperty, values).AssertSuccess();
      }
      else
      {
        ezVariantArray valuesNext;
        m_pObjectAccessor->GetValues(item.m_pObject, pProperty, valuesNext).AssertSuccess();
        if (values != valuesNext)
        {
          return ezVariant();
        }
      }
    }
    return values;
  }
  else
  {
    ezVariant value;
    // check if we have multiple values
    for (const auto& item : items)
    {
      if (!value.IsValid())
      {
        m_pObjectAccessor->GetValue(item.m_pObject, pProperty, value, item.m_Index).IgnoreResult();
      }
      else
      {
        ezVariant valueNext;
        m_pObjectAccessor->GetValue(item.m_pObject, pProperty, valueNext, item.m_Index).AssertSuccess();
        if (value != valueNext)
        {
          value = ezVariant();
          break;
        }
      }
    }
    return value;
  }
}

void ezQtPropertyWidget::PrepareToDie()
{
  EZ_ASSERT_DEBUG(!m_bUndead, "Object has already been marked for cleanup");

  m_bUndead = true;

  DoPrepareToDie();
}

void ezQtPropertyWidget::SetReadOnly(bool bReadOnly /*= true*/)
{
  setDisabled(bReadOnly);
}

void ezQtPropertyWidget::OnCustomContextMenu(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  ExtendContextMenu(m);
  m_pGrid->ExtendContextMenu(m, m_Items, m_pProp);

  m.exec(pt); // pt is already in global space, because we fixed that
}

void ezQtPropertyWidget::Broadcast(ezPropertyEvent::Type type)
{
  ezPropertyEvent ed;
  ed.m_Type = type;
  ed.m_pProperty = m_pProp;
  PropertyChangedHandler(ed);
}

void ezQtPropertyWidget::PropertyChangedHandler(const ezPropertyEvent& ed)
{
  if (m_bUndead)
    return;


  switch (ed.m_Type)
  {
    case ezPropertyEvent::Type::SingleValueChanged:
    {
      ezStringBuilder sTemp;
      sTemp.SetFormat("Change Property '{0}'", ezTranslate(ed.m_pProperty->GetPropertyName()));
      m_pObjectAccessor->StartTransaction(sTemp);

      ezStatus res;
      for (const auto& sel : *ed.m_pItems)
      {
        res = m_pObjectAccessor->SetValue(sel.m_pObject, ed.m_pProperty, ed.m_Value, sel.m_Index);
        if (res.m_Result.Failed())
          break;
      }

      if (res.m_Result.Failed())
        m_pObjectAccessor->CancelTransaction();
      else
        m_pObjectAccessor->FinishTransaction();

      ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Changing the property failed.");
    }
    break;

    case ezPropertyEvent::Type::BeginTemporary:
    {
      ezStringBuilder sTemp;
      sTemp.SetFormat("Change Property '{0}'", ezTranslate(ed.m_pProperty->GetPropertyName()));
      m_pObjectAccessor->BeginTemporaryCommands(sTemp);
    }
    break;

    case ezPropertyEvent::Type::EndTemporary:
    {
      m_pObjectAccessor->FinishTemporaryCommands();
    }
    break;

    case ezPropertyEvent::Type::CancelTemporary:
    {
      m_pObjectAccessor->CancelTemporaryCommands();
    }
    break;
  }
}

bool ezQtPropertyWidget::eventFilter(QObject* pWatched, QEvent* pEvent)
{
  if (pEvent->type() == QEvent::Wheel)
  {
    if (pWatched->parent())
    {
      pWatched->parent()->event(pEvent);
    }

    return true;
  }

  return false;
}

/// *** ezQtUnsupportedPropertyWidget ***

ezQtUnsupportedPropertyWidget::ezQtUnsupportedPropertyWidget(const char* szMessage)
  : ezQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QLabel(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);
  m_sMessage = szMessage;
}

void ezQtUnsupportedPropertyWidget::OnInit()
{
  ezQtScopedBlockSignals bs(m_pWidget);

  QString sMessage;
  if (!m_sMessage.IsEmpty())
  {
    sMessage = m_sMessage;
  }
  else
  {
    ezStringBuilder tmp;
    sMessage = QStringLiteral("Unsupported Type: ") % QString::fromUtf8(m_pProp->GetSpecificType()->GetTypeName().GetData(tmp));
  }

  m_pWidget->setText(sMessage);
  m_pWidget->setToolTip(sMessage);
}


/// *** ezQtStandardPropertyWidget ***

ezQtStandardPropertyWidget::ezQtStandardPropertyWidget()
  : ezQtPropertyWidget()
{
}

void ezQtStandardPropertyWidget::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  ezQtPropertyWidget::SetSelection(items);

  m_OldValue = GetCommonValue(items, m_pProp);
  InternalSetValue(m_OldValue);
}

void ezQtStandardPropertyWidget::BroadcastValueChanged(const ezVariant& NewValue)
{
  if (NewValue == m_OldValue)
    return;

  m_OldValue = NewValue;

  ezPropertyEvent ed;
  ed.m_Type = ezPropertyEvent::Type::SingleValueChanged;
  ed.m_pProperty = m_pProp;
  ed.m_Value = NewValue;
  ed.m_pItems = &m_Items;
  PropertyChangedHandler(ed);
}


/// *** ezQtPropertyPointerWidget ***

ezQtPropertyPointerWidget::ezQtPropertyPointerWidget()
  : ezQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pGroup = new ezQtCollapsibleGroupBox(this);
  m_pGroupLayout = new QHBoxLayout(nullptr);
  m_pGroupLayout->setSpacing(1);
  m_pGroupLayout->setContentsMargins(5, 0, 0, 0);
  m_pGroup->GetContent()->setLayout(m_pGroupLayout);

  m_pLayout->addWidget(m_pGroup);

  m_pAddButton = new ezQtAddSubElementButton();
  m_pGroup->GetHeader()->layout()->addWidget(m_pAddButton);

  m_pDeleteButton = new ezQtElementGroupButton(m_pGroup->GetHeader(), ezQtElementGroupButton::ElementAction::DeleteElement, this);
  m_pGroup->GetHeader()->layout()->addWidget(m_pDeleteButton);
  connect(m_pDeleteButton, &QToolButton::clicked, this, &ezQtPropertyPointerWidget::OnDeleteButtonClicked);

  m_pTypeWidget = nullptr;
}

ezQtPropertyPointerWidget::~ezQtPropertyPointerWidget()
{
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(
    ezMakeDelegate(&ezQtPropertyPointerWidget::StructureEventHandler, this));
}

void ezQtPropertyPointerWidget::OnInit()
{
  UpdateTitle();
  m_pGrid->SetCollapseState(m_pGroup);
  connect(m_pGroup, &ezQtGroupBoxBase::CollapseStateChanged, m_pGrid, &ezQtPropertyGridWidget::OnCollapseStateChanged);

  // Add Buttons
  auto pAttr = m_pProp->GetAttributeByType<ezContainerAttribute>();
  m_pAddButton->setVisible(!pAttr || pAttr->CanAdd());
  m_pDeleteButton->setVisible(!pAttr || pAttr->CanDelete());

  m_pAddButton->Init(m_pGrid, m_pObjectAccessor, m_pType, m_pProp);
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(
    ezMakeDelegate(&ezQtPropertyPointerWidget::StructureEventHandler, this));
}

void ezQtPropertyPointerWidget::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  ezQtScopedUpdatesDisabled _(this);

  ezQtPropertyWidget::SetSelection(items);

  if (m_pTypeWidget)
  {
    m_pGroupLayout->removeWidget(m_pTypeWidget);
    delete m_pTypeWidget;
    m_pTypeWidget = nullptr;
  }


  ezHybridArray<ezPropertySelection, 8> emptyItems;
  ezHybridArray<ezPropertySelection, 8> subItems;
  for (const auto& item : m_Items)
  {
    ezUuid ObjectGuid = m_pObjectAccessor->Get<ezUuid>(item.m_pObject, m_pProp, item.m_Index);
    if (!ObjectGuid.IsValid())
    {
      emptyItems.PushBack(item);
    }
    else
    {
      ezPropertySelection sel;
      sel.m_pObject = m_pObjectAccessor->GetObject(ObjectGuid);

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

  const ezRTTI* pCommonType = nullptr;
  if (!subItems.IsEmpty())
  {
    pCommonType = ezQtPropertyWidget::GetCommonBaseType(subItems);

    m_pTypeWidget = new ezQtTypeWidget(m_pGroup->GetContent(), m_pGrid, m_pObjectAccessor, pCommonType, nullptr, nullptr);
    m_pTypeWidget->SetSelection(subItems);

    m_pGroupLayout->addWidget(m_pTypeWidget);
  }

  UpdateTitle(pCommonType);
}


void ezQtPropertyPointerWidget::DoPrepareToDie()
{
  if (m_pTypeWidget)
  {
    m_pTypeWidget->PrepareToDie();
  }
}

void ezQtPropertyPointerWidget::UpdateTitle(const ezRTTI* pType /*= nullptr*/)
{
  ezStringBuilder sb = ezTranslate(m_pProp->GetPropertyName());
  if (pType != nullptr)
  {
    ezStringBuilder tmp;
    sb.Append(": ", ezTranslate(pType->GetTypeName().GetData(tmp)));
  }
  m_pGroup->SetTitle(sb);
}

void ezQtPropertyPointerWidget::OnDeleteButtonClicked()
{
  m_pObjectAccessor->StartTransaction("Delete Object");

  ezStatus res;
  const ezHybridArray<ezPropertySelection, 8> selection = m_pTypeWidget->GetSelection();
  for (auto& item : selection)
  {
    res = m_pObjectAccessor->RemoveObject(item.m_pObject);
    if (res.m_Result.Failed())
      break;
  }

  if (res.m_Result.Failed())
    m_pObjectAccessor->CancelTransaction();
  else
    m_pObjectAccessor->FinishTransaction();

  ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Removing sub-element from the property failed.");
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
      if (!e.m_sParentProperty.IsEqual(m_pProp->GetPropertyName()))
        return;

      if (std::none_of(cbegin(m_Items), cend(m_Items),
            [&](const ezPropertySelection& sel)
            { return e.m_pNewParent == sel.m_pObject || e.m_pPreviousParent == sel.m_pObject; }))
        return;

      SetSelection(m_Items);
    }
    break;
    default:
      break;
  }
}

/// *** ezQtEmbeddedClassPropertyWidget ***

ezQtEmbeddedClassPropertyWidget::ezQtEmbeddedClassPropertyWidget()
  : ezQtPropertyWidget()

{
}


ezQtEmbeddedClassPropertyWidget::~ezQtEmbeddedClassPropertyWidget()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtEmbeddedClassPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtEmbeddedClassPropertyWidget::CommandHistoryEventHandler, this));
}

void ezQtEmbeddedClassPropertyWidget::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  ezQtScopedUpdatesDisabled _(this);

  ezQtPropertyWidget::SetSelection(items);

  // Retrieve the objects the property points to. This could be an embedded class or
  // an element of an array, be it pointer or embedded class.
  m_ResolvedObjects.Clear();
  for (const auto& item : m_Items)
  {
    ezUuid ObjectGuid = m_pObjectAccessor->Get<ezUuid>(item.m_pObject, m_pProp, item.m_Index);
    ezPropertySelection sel;
    sel.m_pObject = m_pObjectAccessor->GetObject(ObjectGuid);
    // sel.m_Index; intentionally invalid as we just retrieved the value so it is a pointer to an object

    m_ResolvedObjects.PushBack(sel);
  }

  m_pResolvedType = m_pProp->GetSpecificType();
  if (m_pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
  {
    m_pResolvedType = ezQtPropertyWidget::GetCommonBaseType(m_ResolvedObjects);
  }
}

void ezQtEmbeddedClassPropertyWidget::SetPropertyValue(const ezAbstractProperty* pProperty, const ezVariant& NewValue)
{
  ezStatus res;
  for (const auto& sel : m_ResolvedObjects)
  {
    res = m_pObjectAccessor->SetValue(sel.m_pObject, pProperty, NewValue, sel.m_Index);
    if (res.m_Result.Failed())
      break;
  }
  // ezPropertyEvent ed;
  // ed.m_Type = ezPropertyEvent::Type::SingleValueChanged;
  // ed.m_pProperty = pProperty;
  // ed.m_Value = NewValue;
  // ed.m_pItems = &m_ResolvedObjects;

  // m_Events.Broadcast(ed);
}

void ezQtEmbeddedClassPropertyWidget::OnInit()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtEmbeddedClassPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtEmbeddedClassPropertyWidget::CommandHistoryEventHandler, this));
}


void ezQtEmbeddedClassPropertyWidget::DoPrepareToDie() {}

void ezQtEmbeddedClassPropertyWidget::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (IsUndead())
    return;

  if (std::none_of(cbegin(m_ResolvedObjects), cend(m_ResolvedObjects), [=](const ezPropertySelection& sel)
        { return e.m_pObject == sel.m_pObject; }))
    return;

  if (!m_QueuedChanges.Contains(e.m_sProperty))
  {
    m_QueuedChanges.PushBack(e.m_sProperty);
  }
}


void ezQtEmbeddedClassPropertyWidget::CommandHistoryEventHandler(const ezCommandHistoryEvent& e)
{
  if (IsUndead())
    return;

  switch (e.m_Type)
  {
    case ezCommandHistoryEvent::Type::UndoEnded:
    case ezCommandHistoryEvent::Type::RedoEnded:
    case ezCommandHistoryEvent::Type::TransactionEnded:
    case ezCommandHistoryEvent::Type::TransactionCanceled:
    {
      FlushQueuedChanges();
    }
    break;

    default:
      break;
  }
}

void ezQtEmbeddedClassPropertyWidget::FlushQueuedChanges()
{
  for (const ezString& sProperty : m_QueuedChanges)
  {
    OnPropertyChanged(sProperty);
  }

  m_QueuedChanges.Clear();
}

/// *** ezQtPropertyTypeWidget ***

ezQtPropertyTypeWidget::ezQtPropertyTypeWidget(bool bAddCollapsibleGroup)
  : ezQtPropertyWidget()
{
  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);
  m_pGroup = nullptr;
  m_pGroupLayout = nullptr;

  if (bAddCollapsibleGroup)
  {
    m_pGroup = new ezQtCollapsibleGroupBox(this);
    m_pGroupLayout = new QVBoxLayout(nullptr);
    m_pGroupLayout->setSpacing(1);
    m_pGroupLayout->setContentsMargins(5, 0, 0, 0);
    m_pGroup->GetContent()->setLayout(m_pGroupLayout);

    m_pLayout->addWidget(m_pGroup);
  }
  m_pTypeWidget = nullptr;
}

ezQtPropertyTypeWidget::~ezQtPropertyTypeWidget() = default;

void ezQtPropertyTypeWidget::OnInit()
{
  if (m_pGroup)
  {
    m_pGroup->SetTitle(ezTranslate(m_pProp->GetPropertyName()));
    m_pGrid->SetCollapseState(m_pGroup);
    connect(m_pGroup, &ezQtGroupBoxBase::CollapseStateChanged, m_pGrid, &ezQtPropertyGridWidget::OnCollapseStateChanged);
  }
}

void ezQtPropertyTypeWidget::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  ezQtScopedUpdatesDisabled _(this);

  ezQtPropertyWidget::SetSelection(items);

  QVBoxLayout* pLayout = m_pGroup != nullptr ? m_pGroupLayout : m_pLayout;
  QWidget* pOwner = m_pGroup != nullptr ? m_pGroup->GetContent() : this;
  if (m_pTypeWidget)
  {
    pLayout->removeWidget(m_pTypeWidget);
    delete m_pTypeWidget;
    m_pTypeWidget = nullptr;
  }

  // Retrieve the objects the property points to. This could be an embedded class or
  // an element of an array, be it pointer or embedded class.
  ezHybridArray<ezPropertySelection, 8> ResolvedObjects;
  for (const auto& item : m_Items)
  {
    ezUuid ObjectGuid = m_pObjectAccessor->Get<ezUuid>(item.m_pObject, m_pProp, item.m_Index);
    ezPropertySelection sel;
    sel.m_pObject = m_pObjectAccessor->GetObject(ObjectGuid);
    // sel.m_Index; intentionally invalid as we just retrieved the value so it is a pointer to an object

    ResolvedObjects.PushBack(sel);
  }

  const ezRTTI* pCommonType = nullptr;
  if (m_pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
  {
    pCommonType = ezQtPropertyWidget::GetCommonBaseType(ResolvedObjects);
  }
  else
  {
    // If we create a widget for a member class we already determined the common base type at the parent type widget.
    // As we are not dealing with a pointer in this case the type must match the property exactly.
    pCommonType = m_pProp->GetSpecificType();
  }
  m_pTypeWidget = new ezQtTypeWidget(pOwner, m_pGrid, m_pObjectAccessor, pCommonType, nullptr, nullptr);
  pLayout->addWidget(m_pTypeWidget);
  m_pTypeWidget->SetSelection(ResolvedObjects);
}


void ezQtPropertyTypeWidget::SetIsDefault(bool bIsDefault)
{
  // The default state set by the parent object / container only refers to the element's correct position in the container but the entire state of the object. As recursively checking an entire object if is has any non-default values is quite costly, we just pretend the object is never in its default state the the user can click revert to default on any object at any time.
  m_bIsDefault = false;
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

{
  m_Pal = palette();
  setAutoFillBackground(true);

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pGroup = new ezQtCollapsibleGroupBox(this);
  m_pGroupLayout = new QVBoxLayout(nullptr);
  m_pGroupLayout->setSpacing(1);
  m_pGroupLayout->setContentsMargins(5, 0, 0, 0);
  m_pGroup->GetContent()->setLayout(m_pGroupLayout);
  m_pGroup->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  connect(m_pGroup, &QWidget::customContextMenuRequested, this, &ezQtPropertyContainerWidget::OnContainerContextMenu);

  setAcceptDrops(true);
  m_pLayout->addWidget(m_pGroup);
}

ezQtPropertyContainerWidget::~ezQtPropertyContainerWidget()
{
  Clear();
}

void ezQtPropertyContainerWidget::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  ezQtPropertyWidget::SetSelection(items);

  UpdateElements();

  if (m_pAddButton)
  {
    m_pAddButton->SetSelection(m_Items);
  }
}

void ezQtPropertyContainerWidget::SetIsDefault(bool bIsDefault)
{
  // This is called from the type widget which we ignore as we have a tighter scoped default value provider for containers.
}

void ezQtPropertyContainerWidget::DoPrepareToDie()
{
  for (const auto& e : m_Elements)
  {
    e.m_pWidget->PrepareToDie();
  }
}

void ezQtPropertyContainerWidget::dragEnterEvent(QDragEnterEvent* event)
{
  updateDropIndex(event);
}

void ezQtPropertyContainerWidget::dragMoveEvent(QDragMoveEvent* event)
{
  updateDropIndex(event);
}

void ezQtPropertyContainerWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
  m_iDropSource = -1;
  m_iDropTarget = -1;
  update();
}

void ezQtPropertyContainerWidget::dropEvent(QDropEvent* event)
{
  if (updateDropIndex(event))
  {
    ezQtGroupBoxBase* pGroup = qobject_cast<ezQtGroupBoxBase*>(event->source());
    Element* pDragElement =
      std::find_if(begin(m_Elements), end(m_Elements), [pGroup](const Element& elem) -> bool
        { return elem.m_pSubGroup == pGroup; });
    if (pDragElement)
    {
      const ezAbstractProperty* pProp = pDragElement->m_pWidget->GetProperty();
      ezHybridArray<ezPropertySelection, 8> items = pDragElement->m_pWidget->GetSelection();
      if (m_iDropSource != m_iDropTarget && (m_iDropSource + 1) != m_iDropTarget)
      {
        MoveItems(items, m_iDropTarget - m_iDropSource);
      }
    }
  }
  m_iDropSource = -1;
  m_iDropTarget = -1;
  update();
}

void ezQtPropertyContainerWidget::paintEvent(QPaintEvent* event)
{
  ezQtPropertyWidget::paintEvent(event);
  if (m_iDropSource != -1 && m_iDropTarget != -1)
  {
    ezInt32 iYPos = 0;
    if (m_iDropTarget < (ezInt32)m_Elements.GetCount())
    {
      const QPoint globalPos = m_Elements[m_iDropTarget].m_pSubGroup->mapToGlobal(QPoint(0, 0));
      iYPos = mapFromGlobal(globalPos).y();
    }
    else
    {
      const QPoint globalPos = m_Elements[m_Elements.GetCount() - 1].m_pSubGroup->mapToGlobal(QPoint(0, 0));
      iYPos = mapFromGlobal(globalPos).y() + m_Elements[m_Elements.GetCount() - 1].m_pSubGroup->height();
    }

    QPainter painter(this);
    painter.setPen(QPen(Qt::PenStyle::NoPen));
    painter.setBrush(palette().brush(QPalette::Highlight));
    painter.drawRect(0, iYPos - 3, width(), 4);
  }
}

void ezQtPropertyContainerWidget::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set palette.
  setPalette(m_Pal);
  ezQtPropertyWidget::showEvent(event);
}

bool ezQtPropertyContainerWidget::updateDropIndex(QDropEvent* pEvent)
{
  if (pEvent->source() && pEvent->mimeData()->hasFormat("application/x-groupBoxDragProperty"))
  {
    // Is the drop source part of this widget?
    for (ezUInt32 i = 0; i < m_Elements.GetCount(); i++)
    {
      if (m_Elements[i].m_pSubGroup == pEvent->source())
      {
        pEvent->setDropAction(Qt::MoveAction);
        pEvent->accept();
        ezInt32 iNewDropTarget = -1;
        // Find closest drop target.
        const ezInt32 iGlobalYPos = mapToGlobal(pEvent->position().toPoint()).y();
        for (ezUInt32 j = 0; j < m_Elements.GetCount(); j++)
        {
          const QRect rect(m_Elements[j].m_pSubGroup->mapToGlobal(QPoint(0, 0)), m_Elements[j].m_pSubGroup->size());
          if (iGlobalYPos > rect.center().y())
          {
            iNewDropTarget = (ezInt32)j + 1;
          }
          else if (iGlobalYPos < rect.center().y())
          {
            iNewDropTarget = (ezInt32)j;
            break;
          }
        }
        if (m_iDropSource != (ezInt32)i || m_iDropTarget != iNewDropTarget)
        {
          m_iDropSource = (ezInt32)i;
          m_iDropTarget = iNewDropTarget;
          update();
        }
        return true;
      }
    }
  }

  if (m_iDropSource != -1 || m_iDropTarget != -1)
  {
    m_iDropSource = -1;
    m_iDropTarget = -1;
    update();
  }
  pEvent->ignore();
  return false;
}

void ezQtPropertyContainerWidget::OnElementButtonClicked()
{
  ezQtElementGroupButton* pButton = qobject_cast<ezQtElementGroupButton*>(sender());
  const ezAbstractProperty* pProp = pButton->GetGroupWidget()->GetProperty();
  ezHybridArray<ezPropertySelection, 8> items = pButton->GetGroupWidget()->GetSelection();

  switch (pButton->GetAction())
  {
    case ezQtElementGroupButton::ElementAction::MoveElementUp:
    {
      MoveItems(items, -1);
    }
    break;
    case ezQtElementGroupButton::ElementAction::MoveElementDown:
    {
      MoveItems(items, 2);
    }
    break;
    case ezQtElementGroupButton::ElementAction::DeleteElement:
    {
      DeleteItems(items);
    }
    break;

    case ezQtElementGroupButton::ElementAction::Help:
      // handled by custom lambda
      break;
  }
}

void ezQtPropertyContainerWidget::OnDragStarted(QMimeData& ref_mimeData)
{
  ezQtGroupBoxBase* pGroup = qobject_cast<ezQtGroupBoxBase*>(sender());
  Element* pDragElement =
    std::find_if(begin(m_Elements), end(m_Elements), [pGroup](const Element& elem) -> bool
      { return elem.m_pSubGroup == pGroup; });
  if (pDragElement)
  {
    ref_mimeData.setData("application/x-groupBoxDragProperty", QByteArray());
  }
}

void ezQtPropertyContainerWidget::OnContainerContextMenu(const QPoint& pt)
{
  ezQtGroupBoxBase* pGroup = qobject_cast<ezQtGroupBoxBase*>(sender());

  QMenu m;
  m.setToolTipsVisible(true);
  ExtendContextMenu(m);

  if (!m.isEmpty())
  {
    m.exec(pGroup->mapToGlobal(pt));
  }
}

void ezQtPropertyContainerWidget::OnCustomElementContextMenu(const QPoint& pt)
{
  ezQtGroupBoxBase* pGroup = qobject_cast<ezQtGroupBoxBase*>(sender());
  Element* pElement = std::find_if(begin(m_Elements), end(m_Elements), [pGroup](const Element& elem) -> bool
    { return elem.m_pSubGroup == pGroup; });

  if (pElement)
  {
    QMenu m;
    m.setToolTipsVisible(true);
    pElement->m_pWidget->ExtendContextMenu(m);

    m_pGrid->ExtendContextMenu(m, pElement->m_pWidget->GetSelection(), pElement->m_pWidget->GetProperty());

    if (!m.isEmpty())
    {
      m.exec(pGroup->mapToGlobal(pt));
    }
  }
}

ezQtGroupBoxBase* ezQtPropertyContainerWidget::CreateElement(QWidget* pParent)
{
  auto pBox = new ezQtCollapsibleGroupBox(pParent);
  return pBox;
}

ezQtPropertyWidget* ezQtPropertyContainerWidget::CreateWidget(ezUInt32 index)
{
  return new ezQtPropertyTypeWidget();
}

ezQtPropertyContainerWidget::Element& ezQtPropertyContainerWidget::AddElement(ezUInt32 index)
{
  ezQtGroupBoxBase* pSubGroup = CreateElement(m_pGroup);
  pSubGroup->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  connect(pSubGroup, &ezQtGroupBoxBase::CollapseStateChanged, m_pGrid, &ezQtPropertyGridWidget::OnCollapseStateChanged);
  connect(pSubGroup, &QWidget::customContextMenuRequested, this, &ezQtPropertyContainerWidget::OnCustomElementContextMenu);

  QVBoxLayout* pSubLayout = new QVBoxLayout(nullptr);
  pSubLayout->setContentsMargins(5, 0, 5, 0);
  pSubLayout->setSpacing(1);
  pSubGroup->GetContent()->setLayout(pSubLayout);

  m_pGroupLayout->insertWidget((int)index, pSubGroup);

  ezQtPropertyWidget* pNewWidget = CreateWidget(index);

  pNewWidget->setParent(pSubGroup);
  pSubLayout->addWidget(pNewWidget);

  pNewWidget->Init(m_pGrid, m_pObjectAccessor, m_pType, m_pProp);

  // Add Buttons
  auto pAttr = m_pProp->GetAttributeByType<ezContainerAttribute>();
  if ((!pAttr || pAttr->CanMove()) && m_pProp->GetCategory() != ezPropertyCategory::Map)
  {
    pSubGroup->SetDraggable(true);
    connect(pSubGroup, &ezQtGroupBoxBase::DragStarted, this, &ezQtPropertyContainerWidget::OnDragStarted);
  }

  ezQtElementGroupButton* pHelpButton = new ezQtElementGroupButton(pSubGroup->GetHeader(), ezQtElementGroupButton::ElementAction::Help, pNewWidget);
  pSubGroup->GetHeader()->layout()->addWidget(pHelpButton);
  pHelpButton->setVisible(false); // added now, and shown later when we know the URL

  if (!pAttr || pAttr->CanDelete())
  {
    ezQtElementGroupButton* pDeleteButton =
      new ezQtElementGroupButton(pSubGroup->GetHeader(), ezQtElementGroupButton::ElementAction::DeleteElement, pNewWidget);
    pSubGroup->GetHeader()->layout()->addWidget(pDeleteButton);
    connect(pDeleteButton, &QToolButton::clicked, this, &ezQtPropertyContainerWidget::OnElementButtonClicked);
  }

  m_Elements.InsertAt(index, Element(pSubGroup, pNewWidget, pHelpButton));
  return m_Elements[index];
}

void ezQtPropertyContainerWidget::RemoveElement(ezUInt32 index)
{
  Element& elem = m_Elements[index];

  m_pGroupLayout->removeWidget(elem.m_pSubGroup);
  delete elem.m_pSubGroup;
  m_Elements.RemoveAtAndCopy(index);
}

void ezQtPropertyContainerWidget::UpdateElements()
{
  ezQtScopedUpdatesDisabled _(this);

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

  UpdatePropertyMetaState();

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
  if (m_pProp->GetCategory() == ezPropertyCategory::Map)
  {
    m_Keys.Clear();
    EZ_VERIFY(m_pObjectAccessor->GetKeys(m_Items[0].m_pObject, m_pProp, m_Keys).m_Result.Succeeded(), "GetKeys should always succeed.");
    ezHybridArray<ezVariant, 16> keys;
    for (ezUInt32 i = 1; i < m_Items.GetCount(); i++)
    {
      keys.Clear();
      EZ_VERIFY(m_pObjectAccessor->GetKeys(m_Items[i].m_pObject, m_pProp, keys).m_Result.Succeeded(), "GetKeys should always succeed.");
      for (ezInt32 k = (ezInt32)m_Keys.GetCount() - 1; k >= 0; --k)
      {
        if (!keys.Contains(m_Keys[k]))
        {
          m_Keys.RemoveAtAndSwap(k);
        }
      }
    }
    m_Keys.Sort([](const ezVariant& a, const ezVariant& b)
      { return a.Get<ezString>().Compare(b.Get<ezString>()) < 0; });
    return m_Keys.GetCount();
  }
  else
  {
    ezInt32 iElements = 0x7FFFFFFF;
    for (const auto& item : m_Items)
    {
      ezInt32 iCount = 0;
      EZ_VERIFY(m_pObjectAccessor->GetCount(item.m_pObject, m_pProp, iCount).m_Result.Succeeded(), "GetCount should always succeed.");
      iElements = ezMath::Min(iElements, iCount);
    }
    EZ_ASSERT_DEV(iElements >= 0, "Mismatch between storage and RTTI ({0})", iElements);
    m_Keys.Clear();
    for (ezUInt32 i = 0; i < (ezUInt32)iElements; i++)
    {
      m_Keys.PushBack(i);
    }

    return ezUInt32(iElements);
  }
}

void ezQtPropertyContainerWidget::UpdatePropertyMetaState()
{
  ezPropertyMetaState* pMeta = ezPropertyMetaState::GetSingleton();
  ezHashTable<ezVariant, ezPropertyUiState> ElementStates;
  pMeta->GetContainerElementsState(m_Items, m_pProp->GetPropertyName(), ElementStates);

  ezDefaultContainerState defaultState(m_pObjectAccessor, m_Items, m_pProp->GetPropertyName());
  m_bIsDefault = defaultState.IsDefaultContainer();
  m_pGroup->SetBoldTitle(!m_bIsDefault);

  QColor qColor = ezQtPropertyWidget::SetPaletteBackgroundColor(defaultState.GetBackgroundColor(), m_Pal);
  setPalette(m_Pal);

  const bool bReadOnly = m_pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly) ||
                         (m_pProp->GetAttributeByType<ezReadOnlyAttribute>() != nullptr);
  for (ezUInt32 i = 0; i < m_Elements.GetCount(); i++)
  {
    Element& element = m_Elements[i];
    ezVariant& key = m_Keys[i];
    const bool bIsDefault = defaultState.IsDefaultElement(key);
    auto itData = ElementStates.Find(key);
    ezPropertyUiState::Visibility state = ezPropertyUiState::Default;
    if (itData.IsValid())
    {
      state = itData.Value().m_Visibility;
    }

    if (element.m_pSubGroup)
    {
      element.m_pSubGroup->setVisible(state != ezPropertyUiState::Invisible);
      element.m_pSubGroup->setEnabled(!bReadOnly && state != ezPropertyUiState::Disabled);
      element.m_pSubGroup->SetBoldTitle(!bIsDefault);

      // If the fill color is invalid that means no border is drawn and we don't want to change the color then.
      if (!element.m_pSubGroup->GetFillColor().isValid())
      {
        element.m_pSubGroup->SetFillColor(qColor);
      }
    }
    if (element.m_pWidget)
    {
      element.m_pWidget->setVisible(state != ezPropertyUiState::Invisible);
      element.m_pWidget->SetReadOnly(bReadOnly || state == ezPropertyUiState::Disabled);
      element.m_pWidget->SetIsDefault(bIsDefault);
    }
  }
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
  ezStringBuilder fullname(m_pType->GetTypeName(), "::", m_pProp->GetPropertyName());

  m_pGroup->SetTitle(ezTranslate(fullname));

  const ezContainerAttribute* pArrayAttr = m_pProp->GetAttributeByType<ezContainerAttribute>();
  if (!pArrayAttr || pArrayAttr->CanAdd())
  {
    m_pAddButton = new ezQtAddSubElementButton();
    m_pAddButton->Init(m_pGrid, m_pObjectAccessor, m_pType, m_pProp);
    m_pGroup->GetHeader()->layout()->addWidget(m_pAddButton);
  }

  m_pGrid->SetCollapseState(m_pGroup);
  connect(m_pGroup, &ezQtGroupBoxBase::CollapseStateChanged, m_pGrid, &ezQtPropertyGridWidget::OnCollapseStateChanged);
}

void ezQtPropertyContainerWidget::DeleteItems(ezHybridArray<ezPropertySelection, 8>& items)
{
  m_pObjectAccessor->StartTransaction("Delete Object");

  ezStatus res(EZ_SUCCESS);
  const bool bIsValueType = ezReflectionUtils::IsValueType(m_pProp);

  if (bIsValueType)
  {
    for (auto& item : items)
    {
      res = m_pObjectAccessor->RemoveValue(item.m_pObject, m_pProp, item.m_Index);
      if (res.m_Result.Failed())
        break;
    }
  }
  else
  {
    ezRemoveObjectCommand cmd;

    for (auto& item : items)
    {
      ezUuid value = m_pObjectAccessor->Get<ezUuid>(item.m_pObject, m_pProp, item.m_Index);
      const ezDocumentObject* pObject = m_pObjectAccessor->GetObject(value);
      res = m_pObjectAccessor->RemoveObject(pObject);
      if (res.m_Result.Failed())
        break;
    }
  }

  if (res.m_Result.Failed())
    m_pObjectAccessor->CancelTransaction();
  else
    m_pObjectAccessor->FinishTransaction();

  ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Removing sub-element from the property failed.");
}

void ezQtPropertyContainerWidget::MoveItems(ezHybridArray<ezPropertySelection, 8>& items, ezInt32 iMove)
{
  EZ_ASSERT_DEV(m_pProp->GetCategory() != ezPropertyCategory::Map, "Map entries can't be moved.");

  m_pObjectAccessor->StartTransaction("Reparent Object");

  ezStatus res(EZ_SUCCESS);
  const bool bIsValueType = ezReflectionUtils::IsValueType(m_pProp);
  if (bIsValueType)
  {
    for (auto& item : items)
    {
      ezInt32 iCurIndex = item.m_Index.ConvertTo<ezInt32>() + iMove;
      if (iCurIndex < 0 || iCurIndex > m_pObjectAccessor->GetCount(item.m_pObject, m_pProp))
        continue;

      res = m_pObjectAccessor->MoveValue(item.m_pObject, m_pProp, item.m_Index, iCurIndex);
      if (res.m_Result.Failed())
        break;
    }
  }
  else
  {
    ezMoveObjectCommand cmd;

    for (auto& item : items)
    {
      ezInt32 iCurIndex = item.m_Index.ConvertTo<ezInt32>() + iMove;
      if (iCurIndex < 0 || iCurIndex > m_pObjectAccessor->GetCount(item.m_pObject, m_pProp))
        continue;

      ezUuid value = m_pObjectAccessor->Get<ezUuid>(item.m_pObject, m_pProp, item.m_Index);
      const ezDocumentObject* pObject = m_pObjectAccessor->GetObject(value);

      res = m_pObjectAccessor->MoveObject(pObject, item.m_pObject, m_pProp, iCurIndex);
      if (res.m_Result.Failed())
        break;
    }
  }

  if (res.m_Result.Failed())
    m_pObjectAccessor->CancelTransaction();
  else
    m_pObjectAccessor->FinishTransaction();

  ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Moving sub-element failed.");
}


/// *** ezQtPropertyStandardTypeContainerWidget ***

ezQtPropertyStandardTypeContainerWidget::ezQtPropertyStandardTypeContainerWidget()
  : ezQtPropertyContainerWidget()
{
}

ezQtPropertyStandardTypeContainerWidget::~ezQtPropertyStandardTypeContainerWidget() = default;

ezQtGroupBoxBase* ezQtPropertyStandardTypeContainerWidget::CreateElement(QWidget* pParent)
{
  auto* pBox = new ezQtInlinedGroupBox(pParent);
  pBox->SetFillColor(QColor::Invalid);
  return pBox;
}


ezQtPropertyWidget* ezQtPropertyStandardTypeContainerWidget::CreateWidget(ezUInt32 index)
{
  return ezQtPropertyGridWidget::CreateMemberPropertyWidget(m_pProp);
}

ezQtPropertyContainerWidget::Element& ezQtPropertyStandardTypeContainerWidget::AddElement(ezUInt32 index)
{
  ezQtPropertyContainerWidget::Element& elem = ezQtPropertyContainerWidget::AddElement(index);
  return elem;
}

void ezQtPropertyStandardTypeContainerWidget::RemoveElement(ezUInt32 index)
{
  ezQtPropertyContainerWidget::RemoveElement(index);
}

void ezQtPropertyStandardTypeContainerWidget::UpdateElement(ezUInt32 index)
{
  Element& elem = m_Elements[index];

  ezHybridArray<ezPropertySelection, 8> SubItems;

  for (const auto& item : m_Items)
  {
    ezPropertySelection sel;
    sel.m_pObject = item.m_pObject;
    sel.m_Index = m_Keys[index];

    SubItems.PushBack(sel);
  }

  ezStringBuilder sTitle;
  if (m_pProp->GetCategory() == ezPropertyCategory::Map)
    sTitle.SetFormat("{0}", m_Keys[index].ConvertTo<ezString>());
  else
    sTitle.SetFormat("[{0}]", m_Keys[index].ConvertTo<ezString>());

  elem.m_pSubGroup->SetTitle(sTitle);
  m_pGrid->SetCollapseState(elem.m_pSubGroup);
  elem.m_pWidget->SetSelection(SubItems);
}

/// *** ezQtPropertyTypeContainerWidget ***

ezQtPropertyTypeContainerWidget::ezQtPropertyTypeContainerWidget()

  = default;

ezQtPropertyTypeContainerWidget::~ezQtPropertyTypeContainerWidget()
{
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(
    ezMakeDelegate(&ezQtPropertyTypeContainerWidget::StructureEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyTypeContainerWidget::CommandHistoryEventHandler, this));
}

void ezQtPropertyTypeContainerWidget::OnInit()
{
  ezQtPropertyContainerWidget::OnInit();
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(
    ezMakeDelegate(&ezQtPropertyTypeContainerWidget::StructureEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtPropertyTypeContainerWidget::CommandHistoryEventHandler, this));
}

void ezQtPropertyTypeContainerWidget::UpdateElement(ezUInt32 index)
{
  Element& elem = m_Elements[index];
  ezHybridArray<ezPropertySelection, 8> SubItems;

  // To be in line with all other ezQtPropertyWidget the container element will
  // be given a selection in the form of this is the parent object, this is the property and in this
  // specific case this is the index you are working on. So SubItems only decorates the items with the correct index.
  for (const auto& item : m_Items)
  {
    ezPropertySelection sel;
    sel.m_pObject = item.m_pObject;
    sel.m_Index = m_Keys[index];

    SubItems.PushBack(sel);
  }

  {
    // To get the correct name we actually need to resolve the selection to the actual objects
    // they are pointing to.
    ezHybridArray<ezPropertySelection, 8> ResolvedObjects;
    for (const auto& item : SubItems)
    {
      ezUuid ObjectGuid = m_pObjectAccessor->Get<ezUuid>(item.m_pObject, m_pProp, item.m_Index);
      ezPropertySelection sel;
      sel.m_pObject = m_pObjectAccessor->GetObject(ObjectGuid);
      ResolvedObjects.PushBack(sel);
    }

    const ezRTTI* pCommonType = ezQtPropertyWidget::GetCommonBaseType(ResolvedObjects);

    // Label
    {
      ezStringBuilder sTitle, tmp;
      sTitle.SetFormat("[{0}] - {1}", m_Keys[index].ConvertTo<ezString>(), ezTranslate(pCommonType->GetTypeName().GetData(tmp)));

      if (auto pInDev = pCommonType->GetAttributeByType<ezInDevelopmentAttribute>())
      {
        sTitle.AppendFormat(" [ {} ]", pInDev->GetString());
      }

      elem.m_pSubGroup->SetTitle(sTitle);
    }

    ezColor borderIconColor = ezColor::MakeZero();

    if (const ezColorAttribute* pColorAttrib = pCommonType->GetAttributeByType<ezColorAttribute>())
    {
      borderIconColor = pColorAttrib->GetColor();
      elem.m_pSubGroup->SetFillColor(ezToQtColor(pColorAttrib->GetColor()));
    }
    else if (const ezCategoryAttribute* pCatAttrib = pCommonType->GetAttributeByType<ezCategoryAttribute>())
    {
      borderIconColor = ezColorScheme::GetCategoryColor(pCatAttrib->GetCategory(), ezColorScheme::CategoryColorUsage::BorderIconColor);
      elem.m_pSubGroup->SetFillColor(ezToQtColor(ezColorScheme::GetCategoryColor(pCatAttrib->GetCategory(), ezColorScheme::CategoryColorUsage::BorderColor)));
    }
    else
    {
      const QPalette& pal = palette();
      elem.m_pSubGroup->SetFillColor(pal.mid().color());
    }

    // Icon
    {
      ezStringBuilder sIconName;
      sIconName.Set(":/TypeIcons/", pCommonType->GetTypeName(), ".svg");
      elem.m_pSubGroup->SetIcon(ezQtUiServices::GetCachedIconResource(sIconName.GetData(), borderIconColor));
    }

    // help URL
    {
      ezStringBuilder tmp;
      QString url = ezMakeQString(ezTranslateHelpURL(pCommonType->GetTypeName().GetData(tmp)));

      if (!url.isEmpty())
      {
        elem.m_pHelpButton->setVisible(true);
        connect(elem.m_pHelpButton, &QToolButton::clicked, this, [=]()
          { QDesktopServices::openUrl(QUrl(url)); });
      }
      else
      {
        elem.m_pHelpButton->setVisible(false);
      }
    }
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
      if (!e.m_sParentProperty.IsEqual(m_pProp->GetPropertyName()))
        return;

      if (std::none_of(cbegin(m_Items), cend(m_Items),
            [&](const ezPropertySelection& sel)
            { return e.m_pNewParent == sel.m_pObject || e.m_pPreviousParent == sel.m_pObject; }))
        return;

      m_bNeedsUpdate = true;
    }
    break;
    default:
      break;
  }
}

void ezQtPropertyTypeContainerWidget::CommandHistoryEventHandler(const ezCommandHistoryEvent& e)
{
  if (IsUndead())
    return;

  switch (e.m_Type)
  {
    case ezCommandHistoryEvent::Type::UndoEnded:
    case ezCommandHistoryEvent::Type::RedoEnded:
    case ezCommandHistoryEvent::Type::TransactionEnded:
    case ezCommandHistoryEvent::Type::TransactionCanceled:
    {
      if (m_bNeedsUpdate)
      {
        m_bNeedsUpdate = false;
        UpdateElements();
      }
    }
    break;

    default:
      break;
  }
}

/// *** ezQtVariantPropertyWidget ***

ezQtVariantPropertyWidget::ezQtVariantPropertyWidget()
{
  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 4);
  m_pLayout->setSpacing(1);
  setLayout(m_pLayout);

  m_pTypeList = new QComboBox(this);
  m_pTypeList->installEventFilter(this);
  m_pTypeList->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pTypeList);
}

ezQtVariantPropertyWidget::~ezQtVariantPropertyWidget() = default;

void ezQtVariantPropertyWidget::OnInit()
{
  ezStringBuilder sName;
  for (int i = ezVariantType::Invalid; i < ezVariantType::LastExtendedType; ++i)
  {
    auto type = static_cast<ezVariantType::Enum>(i);
    if (GetVariantTypeDisplayName(type, sName).Succeeded())
    {
      m_pTypeList->addItem(ezMakeQString(ezTranslate(sName)), i);
    }
  }

  connect(m_pTypeList, &QComboBox::currentIndexChanged,
    [this](int iIndex)
    {
      ChangeVariantType(static_cast<ezVariantType::Enum>(m_pTypeList->itemData(iIndex).toInt()));
    });
}

void ezQtVariantPropertyWidget::InternalSetValue(const ezVariant& value)
{
  ezVariantType::Enum commonType = ezVariantType::Invalid;
  const bool sameType = GetCommonVariantSubType(m_Items, m_pProp, commonType);
  const ezRTTI* pNewtSubType = commonType != ezVariantType::Invalid ? ezReflectionUtils::GetTypeFromVariant(commonType) : nullptr;
  if (pNewtSubType != m_pCurrentSubType || m_pWidget == nullptr)
  {
    if (m_pWidget)
    {
      m_pWidget->PrepareToDie();
      m_pWidget->deleteLater();
      m_pWidget = nullptr;
    }
    m_pCurrentSubType = pNewtSubType;
    if (pNewtSubType)
    {
      m_pWidget = ezQtPropertyGridWidget::GetFactory().CreateObject(pNewtSubType);

      if (!m_pWidget)
      {
        m_pWidget = new ezQtUnsupportedPropertyWidget("<Unsupported Type>");
      }
    }
    else if (!sameType)
    {
      m_pWidget = new ezQtUnsupportedPropertyWidget("Multi-selection has varying types");
    }
    else
    {
      m_pWidget = new ezQtUnsupportedPropertyWidget("<Invalid Type>");
    }

    m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_pWidget->setParent(this);
    m_pLayout->addWidget(m_pWidget);
    m_pWidget->Init(m_pGrid, m_pObjectAccessor, m_pType, m_pProp);

    UpdateTypeListSelection(commonType);
  }
  m_pWidget->SetSelection(m_Items);
}

void ezQtVariantPropertyWidget::DoPrepareToDie()
{
  if (m_pWidget)
    m_pWidget->PrepareToDie();
}

void ezQtVariantPropertyWidget::UpdateTypeListSelection(ezVariantType::Enum type)
{
  ezQtScopedBlockSignals bs(m_pTypeList);
  for (int i = 0; i < m_pTypeList->count(); ++i)
  {
    if (m_pTypeList->itemData(i).toInt() == type)
    {
      m_pTypeList->setCurrentIndex(i);
      break;
    }
  }
}

void ezQtVariantPropertyWidget::ChangeVariantType(ezVariantType::Enum type)
{
  m_pObjectAccessor->StartTransaction("Change variant type");
  // check if we have multiple values
  for (const auto& item : m_Items)
  {
    ezVariant value;
    EZ_VERIFY(m_pObjectAccessor->GetValue(item.m_pObject, m_pProp, value, item.m_Index).Succeeded(), "");
    if (value.CanConvertTo(type))
    {
      EZ_VERIFY(m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, value.ConvertTo(type), item.m_Index).Succeeded(), "");
    }
    else
    {
      EZ_VERIFY(
        m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, ezReflectionUtils::GetDefaultVariantFromType(type), item.m_Index).Succeeded(), "");
    }
  }
  m_pObjectAccessor->FinishTransaction();
}

ezResult ezQtVariantPropertyWidget::GetVariantTypeDisplayName(ezVariantType::Enum type, ezStringBuilder& out_sName) const
{
  if (type == ezVariantType::FirstStandardType || type >= ezVariantType::LastStandardType ||
      type == ezVariantType::StringView || type == ezVariantType::DataBuffer || type == ezVariantType::TempHashedString)
    return EZ_FAILURE;

  const ezRTTI* pVariantEnum = ezGetStaticRTTI<ezVariantType>();
  if (ezReflectionUtils::EnumerationToString(pVariantEnum, type, out_sName) == false)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}
