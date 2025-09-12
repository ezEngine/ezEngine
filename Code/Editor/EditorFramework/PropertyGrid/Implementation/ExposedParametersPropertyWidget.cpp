#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/GUI/ExposedParametersTypeRegistry.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/GroupBoxBase.moc.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExposedParameterCommandAccessor, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExposedParametersAsTypeCommandAccessor, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool ezQtExposedParametersPropertyWidget::s_bRawMode = false;

ezExposedParameterCommandAccessor::ezExposedParameterCommandAccessor(
  ezObjectAccessorBase* pSource, const ezAbstractProperty* pParameterProp, const ezAbstractProperty* pParameterSourceProp)
  : ezObjectProxyAccessor(pSource)
  , m_pParameterProp(pParameterProp)
  , m_pParameterSourceProp(pParameterSourceProp)
{
}

ezStatus ezExposedParameterCommandAccessor::GetValue(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value, ezVariant index /*= ezVariant()*/)
{
  if (IsExposedProperty(pObject, pProp))
    pProp = m_pParameterProp;

  ezStatus res = ezObjectProxyAccessor::GetValue(pObject, pProp, out_value, index);
  if (res.Succeeded() && !index.IsValid() && m_pParameterProp == pProp)
  {
    ezVariantDictionary defaultDict;
    if (const ezExposedParameters* pParams = GetExposedParams(pObject))
    {
      for (ezExposedParameter* pParam : pParams->m_Parameters)
      {
        defaultDict.Insert(pParam->m_sName, pParam->m_DefaultValue);
      }
    }
    const ezVariantDictionary& overwrittenDict = out_value.Get<ezVariantDictionary>();
    for (auto it : overwrittenDict)
    {
      defaultDict[it.Key()] = it.Value();
    }
    out_value = defaultDict;
  }
  else if (res.Failed() && m_pParameterProp == pProp && index.IsA<ezString>())
  {
    // If the actual GetValue fails but the key is an exposed param, return its default value instead.
    if (const ezExposedParameter* pParam = GetExposedParam(pObject, index.Get<ezString>()))
    {
      out_value = pParam->m_DefaultValue;
      return ezStatus(EZ_SUCCESS);
    }
  }
  return res;
}

ezStatus ezExposedParameterCommandAccessor::SetValue(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index /*= ezVariant()*/)
{
  if (IsExposedProperty(pObject, pProp))
    pProp = m_pParameterProp;

  ezStatus res = ezObjectProxyAccessor::SetValue(pObject, pProp, newValue, index);
  // As we pretend the exposed params always exist the actual SetValue will fail if this is not actually true,
  // so we redirect to insert to make it true.
  if (res.Failed() && m_pParameterProp == pProp && index.IsA<ezString>())
  {
    return ezExposedParameterCommandAccessor::InsertValue(pObject, pProp, newValue, index);
  }
  return res;
}

ezStatus ezExposedParameterCommandAccessor::RemoveValue(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index /*= ezVariant()*/)
{
  ezStatus res = ezObjectProxyAccessor::RemoveValue(pObject, pProp, index);
  if (res.Failed() && m_pParameterProp == pProp && index.IsA<ezString>())
  {
    // It this is one of the exposed params, pretend we removed it successfully to suppress error messages.
    if (const ezExposedParameter* pParam = GetExposedParam(pObject, index.Get<ezString>()))
    {
      return ezStatus(EZ_SUCCESS);
    }
  }
  return res;
}

ezStatus ezExposedParameterCommandAccessor::GetCount(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezInt32& out_iCount)
{
  if (m_pParameterProp == pProp)
  {
    ezHybridArray<ezVariant, 16> keys;
    GetKeys(pObject, pProp, keys).AssertSuccess();
    out_iCount = keys.GetCount();
    return ezStatus(EZ_SUCCESS);
  }
  return ezObjectProxyAccessor::GetCount(pObject, pProp, out_iCount);
}

ezStatus ezExposedParameterCommandAccessor::GetKeys(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_keys)
{
  if (m_pParameterProp == pProp)
  {
    if (const ezExposedParameters* pParams = GetExposedParams(pObject))
    {
      for (const auto& pParam : pParams->m_Parameters)
      {
        out_keys.PushBack(ezVariant(pParam->m_sName));
      }

      ezHybridArray<ezVariant, 16> realKeys;
      ezStatus res = ezObjectProxyAccessor::GetKeys(pObject, pProp, realKeys);
      for (const auto& key : realKeys)
      {
        if (!out_keys.Contains(key))
        {
          out_keys.PushBack(key);
        }
      }
      return ezStatus(EZ_SUCCESS);
    }
  }
  return ezObjectProxyAccessor::GetKeys(pObject, pProp, out_keys);
}

ezStatus ezExposedParameterCommandAccessor::GetValues(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_values)
{
  if (m_pParameterProp == pProp)
  {
    ezHybridArray<ezVariant, 16> keys;
    GetKeys(pObject, pProp, keys).AssertSuccess();
    for (const auto& key : keys)
    {
      auto& var = out_values.ExpandAndGetRef();
      EZ_VERIFY(GetValue(pObject, pProp, var, key).Succeeded(), "GetValue to valid a key should be not fail.");
    }
    return ezStatus(EZ_SUCCESS);
  }
  return ezObjectProxyAccessor::GetValues(pObject, pProp, out_values);
}


const ezExposedParameters* ezExposedParameterCommandAccessor::GetExposedParams(const ezDocumentObject* pObject)
{
  ezVariant value;
  if (ezObjectProxyAccessor::GetValue(pObject, m_pParameterSourceProp, value).Succeeded())
  {
    if (value.IsA<ezString>())
    {
      const auto& sValue = value.Get<ezString>();
      if (const auto asset = ezAssetCurator::GetSingleton()->FindSubAsset(sValue.GetData()))
      {
        return asset->m_pAssetInfo->m_Info->GetMetaInfo<ezExposedParameters>();
      }
    }
  }
  return nullptr;
}


const ezExposedParameter* ezExposedParameterCommandAccessor::GetExposedParam(const ezDocumentObject* pObject, const char* szParamName)
{
  if (const ezExposedParameters* pParams = GetExposedParams(pObject))
  {
    return pParams->Find(szParamName);
  }
  return nullptr;
}


const ezRTTI* ezExposedParameterCommandAccessor::GetExposedParamsType(const ezDocumentObject* pObject)
{
  ezVariant value;
  if (ezObjectProxyAccessor::GetValue(pObject, m_pParameterSourceProp, value).Succeeded())
  {
    if (value.IsA<ezString>())
    {
      const auto& sValue = value.Get<ezString>();
      if (const auto asset = ezAssetCurator::GetSingleton()->FindSubAsset(sValue.GetData()))
      {
        return ezExposedParametersTypeRegistry::GetSingleton()->GetExposedParametersType(sValue);
      }
    }
  }
  return nullptr;
}

const ezRTTI* ezExposedParameterCommandAccessor::GetCommonExposedParamsType(const ezHybridArray<ezPropertySelection, 8>& items)
{
  const ezRTTI* type = nullptr;
  bool bFirst = true;
  // check if we have multiple values
  for (const auto& item : items)
  {
    if (bFirst)
    {
      type = GetExposedParamsType(item.m_pObject);
      bFirst = false;
    }
    else
    {
      auto type2 = GetExposedParamsType(item.m_pObject);
      if (type != type2)
      {
        return nullptr;
      }
    }
  }
  return type;
}

bool ezExposedParameterCommandAccessor::IsExposedProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
{
  if (auto type = GetExposedParamsType(pObject))
  {
    auto props = type->GetProperties();
    return std::any_of(cbegin(props), cend(props), [&](const ezAbstractProperty* pOtherProp)
      { return pOtherProp == pProp; });
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////

ezExposedParametersAsTypeCommandAccessor::ezExposedParametersAsTypeCommandAccessor(ezExposedParameterCommandAccessor* pSource)
  : ezObjectProxyAccessor(pSource)
{
}

ezStatus ezExposedParametersAsTypeCommandAccessor::GetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value, ezVariant index)
{
  EZ_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, out_value));

  ezStatus result(EZ_SUCCESS);
  out_value = ezVariantStorageAccessor(pProp->GetPropertyName(), out_value).GetValue(index, &result);
  return result;
}

ezStatus ezExposedParametersAsTypeCommandAccessor::SetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index)
{
  return SetSubValue(pObject, pProp, [&](ezVariant& subValue) -> ezStatus
    { return ezVariantStorageAccessor(pProp->GetPropertyName(), subValue).SetValue(newValue, index); });
}

ezStatus ezExposedParametersAsTypeCommandAccessor::InsertValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index)
{
  return SetSubValue(pObject, pProp, [&](ezVariant& subValue) -> ezStatus
    { return ezVariantStorageAccessor(pProp->GetPropertyName(), subValue).InsertValue(index, newValue); });
}

ezStatus ezExposedParametersAsTypeCommandAccessor::RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  return SetSubValue(pObject, pProp, [&](ezVariant& subValue) -> ezStatus
    { return ezVariantStorageAccessor(pProp->GetPropertyName(), subValue).RemoveValue(index); });
}

ezStatus ezExposedParametersAsTypeCommandAccessor::MoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex)
{
  return SetSubValue(pObject, pProp, [&](ezVariant& subValue) -> ezStatus
    { return ezVariantStorageAccessor(pProp->GetPropertyName(), subValue).MoveValue(oldIndex, newIndex); });
}

ezStatus ezExposedParametersAsTypeCommandAccessor::GetCount(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezInt32& out_iCount)
{
  ezVariant subValue;
  EZ_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  out_iCount = ezVariantStorageAccessor(pProp->GetPropertyName(), subValue).GetCount();
  return EZ_SUCCESS;
}

ezStatus ezExposedParametersAsTypeCommandAccessor::GetKeys(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_keys)
{
  ezVariant subValue;
  EZ_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  return ezVariantStorageAccessor(pProp->GetPropertyName(), subValue).GetKeys(out_keys);
}

ezStatus ezExposedParametersAsTypeCommandAccessor::GetValues(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_values)
{
  ezVariant subValue;
  EZ_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  ezHybridArray<ezVariant, 16> keys;
  ezVariantStorageAccessor accessor(pProp->GetPropertyName(), subValue);
  EZ_SUCCEED_OR_RETURN(accessor.GetKeys(keys));
  out_values.Clear();
  out_values.Reserve(keys.GetCount());
  for (const ezVariant& key : keys)
  {
    out_values.PushBack(accessor.GetValue(key));
  }
  return EZ_SUCCESS;
}

ezObjectAccessorBase* ezExposedParametersAsTypeCommandAccessor::ResolveProxy(const ezDocumentObject*& ref_pObject, const ezRTTI*& ref_pType, const ezAbstractProperty*& ref_pProp, ezDynamicArray<ezVariant>& ref_indices)
{
  const ezRTTI* pType = GetSourceAccessor()->GetExposedParamsType(ref_pObject);
  if (pType == ref_pType)
  {
    EZ_ASSERT_DEBUG(pType && pType->FindPropertyByName(ref_pProp->GetPropertyName()) == ref_pProp, "");
    ref_indices.InsertAt(0, ref_pProp->GetPropertyName());
    ref_pType = ref_pObject->GetType();
    ref_pProp = GetSourceAccessor()->m_pParameterProp;
  }
  return m_pSource->ResolveProxy(ref_pObject, ref_pType, ref_pProp, ref_indices);
}

ezStatus ezExposedParametersAsTypeCommandAccessor::GetSubValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value)
{
  const ezRTTI* pType = GetSourceAccessor()->GetExposedParamsType(pObject);
  EZ_ASSERT_DEBUG(pType && pType->FindPropertyByName(pProp->GetPropertyName()) == pProp, "");

  ezStatus result = GetSourceAccessor()->GetValue(pObject, GetSourceAccessor()->m_pParameterProp, out_value, pProp->GetPropertyName());
  if (result.Failed())
    return result;

  PatchPropertyType(out_value, pProp);

  return result;
}

ezStatus ezExposedParametersAsTypeCommandAccessor::SetSubValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezDelegate<ezStatus(ezVariant&)>& func)
{
  ezVariant currentValue;
  EZ_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, currentValue));
  EZ_SUCCEED_OR_RETURN(func(currentValue));
  return GetSourceAccessor()->SetValue(pObject, pProp, currentValue, pProp->GetPropertyName());
}

void ezExposedParametersAsTypeCommandAccessor::PatchPropertyType(ezVariant& ref_value, const ezAbstractProperty* pProp)
{
  if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>() && pProp->GetCategory() == ezPropertyCategory::Member)
    return;

  const ezVariantType::Enum propType = ezToolsReflectionUtils::GetStorageType(pProp);
  const ezVariantType::Enum valueType = ref_value.GetType();
  if (propType != valueType)
  {
    if (ref_value.CanConvertTo(propType))
    {
      ref_value = ref_value.ConvertTo(propType);
    }
    else
    {
      ref_value = ezToolsReflectionUtils::GetStorageDefault(pProp);
    }
  }

  if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
    return;

  switch (pProp->GetCategory())
  {
    case ezPropertyCategory::Array:
      if (const ezVariantType::Enum propElementType = pProp->GetSpecificType()->GetVariantType(); propElementType != ezVariantType::Invalid)
      {
        const ezVariantArray& array = ref_value.Get<ezVariantArray>();
        for (ezUInt32 i = 0; i < array.GetCount(); ++i)
        {
          const ezVariant& element = array[i];
          const ezVariantType::Enum valueElementType = element.GetType();
          if (propElementType != valueElementType)
          {
            ezVariantArray& arrayWritable = ref_value.GetWritable<ezVariantArray>();
            ezVariant& elementWritable = arrayWritable[i];
            if (elementWritable.CanConvertTo(propElementType))
            {
              elementWritable = elementWritable.ConvertTo(propType);
            }
            else
            {
              elementWritable = ezReflectionUtils::GetDefaultVariantFromType(propElementType);
            }
          }
        }
      }
      break;
    case ezPropertyCategory::Map:
      if (const ezVariantType::Enum propElementType = pProp->GetSpecificType()->GetVariantType(); propElementType != ezVariantType::Invalid)
      {
        const ezVariantDictionary& map = ref_value.Get<ezVariantDictionary>();
        for (auto it : map)
        {
          const ezVariant& element = it.Value();
          const ezVariantType::Enum valueElementType = element.GetType();
          if (propElementType != valueElementType)
          {
            ezVariantDictionary& mapWritable = ref_value.GetWritable<ezVariantDictionary>();
            ezVariant* pElementWritable = nullptr;
            if (mapWritable.TryGetValue(it.Key(), pElementWritable))
            {
              if (pElementWritable->CanConvertTo(propElementType))
              {
                *pElementWritable = pElementWritable->ConvertTo(propType);
              }
              else
              {
                *pElementWritable = ezReflectionUtils::GetDefaultVariantFromType(propElementType);
              }
            }
          }
        }
      }
      break;

    default:
      break;
  }
}

//////////////////////////////////////////////////////////////////////////

ezQtExposedParametersPropertyWidget::ezQtExposedParametersPropertyWidget()
  : ezQtPropertyStandardTypeContainerWidget()
{
  // Replace the container layout so we can prepend the type widget before all container elements
  delete m_pGroupLayout;
  m_pGroupLayout = new QVBoxLayout(nullptr);
  m_pGroupLayout->setSpacing(1);
  m_pGroupLayout->setContentsMargins(5, 0, 0, 0);

  m_pTypeViewLayout = new QVBoxLayout(nullptr);
  m_pTypeViewLayout->addLayout(m_pGroupLayout);
  m_pTypeViewLayout->setSpacing(0);
  m_pTypeViewLayout->setContentsMargins(0, 0, 0, 0);

  m_pGroup->GetContent()->setLayout(m_pTypeViewLayout);
}

ezQtExposedParametersPropertyWidget::~ezQtExposedParametersPropertyWidget()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtExposedParametersPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtExposedParametersPropertyWidget::CommandHistoryEventHandler, this));
  ezPhantomRttiManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtExposedParametersPropertyWidget::PhantomTypeRegistryEventHandler, this));
}

void ezQtExposedParametersPropertyWidget::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  const ezRTTI* pCommonType = m_pProxy->GetCommonExposedParamsType(items);
  if (m_pTypeWidget && m_pTypeWidget->GetType() != pCommonType)
  {
    m_pTypeWidget->PrepareToDie();
    m_pTypeWidget->deleteLater();
    m_pTypeWidget = nullptr;
  }

  if (m_pTypeWidget == nullptr && pCommonType != nullptr)
  {
    m_pTypeWidget = new ezQtTypeWidget(m_pGroup->GetContent(), m_pGrid, m_pTypeProxy.Borrow(), pCommonType, nullptr, nullptr);
    m_pTypeViewLayout->insertWidget(0, m_pTypeWidget);
  }

  if (m_pTypeWidget)
  {
    m_pTypeWidget->setVisible(!s_bRawMode);
    m_pTypeWidget->SetSelection(items);
  }


  ezQtPropertyStandardTypeContainerWidget::SetSelection(items);
  UpdateActionState();
}

void ezQtExposedParametersPropertyWidget::OnInit()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtExposedParametersPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtExposedParametersPropertyWidget::CommandHistoryEventHandler, this));
  ezPhantomRttiManager::s_Events.AddEventHandler(ezMakeDelegate(&ezQtExposedParametersPropertyWidget::PhantomTypeRegistryEventHandler, this));

  const auto* pAttrib = m_pProp->GetAttributeByType<ezExposedParametersAttribute>();
  EZ_ASSERT_DEV(pAttrib, "ezQtExposedParametersPropertyWidget was created for a property that does not have the ezExposedParametersAttribute.");
  m_sExposedParamProperty = pAttrib->GetParametersSource();
  const ezAbstractProperty* pParameterSourceProp = m_pType->FindPropertyByName(m_sExposedParamProperty);
  EZ_ASSERT_DEV(
    pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", m_sExposedParamProperty, m_pType->GetTypeName());
  m_pSourceObjectAccessor = m_pObjectAccessor;
  m_pProxy = EZ_DEFAULT_NEW(ezExposedParameterCommandAccessor, m_pSourceObjectAccessor, m_pProp, pParameterSourceProp);
  m_pTypeProxy = EZ_DEFAULT_NEW(ezExposedParametersAsTypeCommandAccessor, m_pProxy.Borrow());
  // Overwriting this will display the exposed parameter map as before, i.e. each property will be shown in the map even if not present. As this is now obsolete given the phantom type widget, this is probably no longer needed?
  // m_pObjectAccessor = m_pProxy.Borrow();

  ezQtPropertyStandardTypeContainerWidget::OnInit();

  auto layout = qobject_cast<QHBoxLayout*>(m_pGroup->GetHeader()->layout());

  layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));

  layout->setStretch(0, 1);

  {
    // Help button to indicate exposed parameter mismatches.
    m_pFixMeButton = new QToolButton();
    m_pFixMeButton->setAutoRaise(true);
    m_pFixMeButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
    m_pFixMeButton->setIcon(ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Attention.svg"));
    auto sp = m_pFixMeButton->sizePolicy();
    sp.setVerticalPolicy(QSizePolicy::Ignored);
    m_pFixMeButton->setSizePolicy(sp);
    QMenu* pFixMeMenu = new QMenu(m_pFixMeButton);
    {
      m_pRemoveUnusedAction = pFixMeMenu->addAction(QStringLiteral("Remove unused keys"));
      m_pRemoveUnusedAction->setToolTip(
        QStringLiteral("The map contains keys that are no longer used by the asset's exposed parameters and thus can be removed."));
      connect(m_pRemoveUnusedAction, &QAction::triggered, this, [this](bool bChecked)
        { RemoveUnusedKeys(false); });
    }
    {
      m_pFixTypesAction = pFixMeMenu->addAction(QStringLiteral("Fix keys with wrong types"));
      connect(m_pFixTypesAction, &QAction::triggered, this, [this](bool bChecked)
        { FixKeyTypes(false); });
    }
    m_pFixMeButton->setMenu(pFixMeMenu);

    auto layout = qobject_cast<QHBoxLayout*>(m_pGroup->GetHeader()->layout());
    layout->insertWidget(layout->count() - 1, m_pFixMeButton);
  }

  {
    // Button to toggle between type-based view and the raw dictionary-based view of the exposed parameters.
    m_pToggleRawModeButton = new QToolButton();
    m_pToggleRawModeButton->setAutoRaise(true);
    m_pToggleRawModeButton->setCheckable(true);
    m_pToggleRawModeButton->setChecked(s_bRawMode);
    m_pToggleRawModeButton->setIcon(ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/ExposedParameterViewToggle.svg"));
    auto sp = m_pToggleRawModeButton->sizePolicy();
    sp.setVerticalPolicy(QSizePolicy::Ignored);
    m_pToggleRawModeButton->setSizePolicy(sp);
    m_pToggleRawModeButton->setToolTip("Toggle between Type or RAW dictionary view");

    connect(m_pToggleRawModeButton, &QToolButton::toggled, this, [this](bool checked)
      {
        s_bRawMode = checked;
        ezQtScopedUpdatesDisabled _(this);
        SetSelection(m_Items); });

    auto layout = qobject_cast<QHBoxLayout*>(m_pGroup->GetHeader()->layout());
    layout->insertWidget(layout->count() - 1, m_pToggleRawModeButton);
  }
}

void ezQtExposedParametersPropertyWidget::UpdateElement(ezUInt32 index)
{
  ezQtPropertyStandardTypeContainerWidget::UpdateElement(index);
}

void ezQtExposedParametersPropertyWidget::UpdatePropertyMetaState()
{
  ezQtPropertyStandardTypeContainerWidget::UpdatePropertyMetaState();
}

void ezQtExposedParametersPropertyWidget::GetRequiredElements(ezDynamicArray<ezVariant>& out_keys) const
{
  if (!s_bRawMode)
  {
    out_keys.Clear();
  }
  else
  {
    ezQtPropertyContainerWidget::GetRequiredElements(out_keys);
  }
}

void ezQtExposedParametersPropertyWidget::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (IsUndead())
    return;

  if (std::none_of(cbegin(m_Items), cend(m_Items), [=](const ezPropertySelection& sel)
        { return e.m_pObject == sel.m_pObject; }))
    return;

  if (!m_bNeedsUpdate && m_sExposedParamProperty == e.m_sProperty)
  {
    FlushOrQueueChanges(true, false);
  }
  if (!m_bNeedsMetaDataUpdate && m_pProp->GetPropertyName() == e.m_sProperty)
  {
    FlushOrQueueChanges(false, true);
  }
}

void ezQtExposedParametersPropertyWidget::CommandHistoryEventHandler(const ezCommandHistoryEvent& e)
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
      FlushOrQueueChanges(false, false);
    }
    break;

    default:
      break;
  }
}

void ezQtExposedParametersPropertyWidget::PhantomTypeRegistryEventHandler(const ezPhantomRttiManagerEvent& e)
{
  if (const ezRTTI* pCommonType = m_pProxy->GetCommonExposedParamsType(m_Items))
  {
    if (e.m_pChangedType->IsDerivedFrom(pCommonType))
    {
      if (m_pTypeWidget)
      {
        // The type widget stores pointer to properties which have been destroyed by the phantom type update so we need to destroy it and recreate it.
        m_pTypeWidget->PrepareToDie();
        m_pTypeWidget->deleteLater();
        m_pTypeWidget = nullptr;
      }
      FlushOrQueueChanges(true, true);
    }
  }
}

void ezQtExposedParametersPropertyWidget::FlushOrQueueChanges(bool bNeedsUpdate, bool bNeedsMetaDataUpdate)
{
  m_bNeedsUpdate |= bNeedsUpdate;
  m_bNeedsMetaDataUpdate |= bNeedsMetaDataUpdate;
  // Wait until the transaction is done and this function will be called again inside CommandHistoryEventHandler.
  if (m_pGrid->GetCommandHistory()->IsInTransaction() || m_pGrid->GetCommandHistory()->IsInUndoRedo())
    return;

  if (m_bNeedsUpdate)
  {
    m_bNeedsUpdate = false;
    SetSelection(m_Items);
  }
  if (m_bNeedsMetaDataUpdate)
  {
    // m_bNeedsMetaDataUpdate is reset inside UpdateActionState as it can be called from other places.
    UpdateActionState();
  }
}

bool ezQtExposedParametersPropertyWidget::RemoveUnusedKeys(bool bTestOnly)
{
  bool bStuffDone = false;
  if (!bTestOnly)
    m_pSourceObjectAccessor->StartTransaction("Remove unused keys");
  for (const auto& item : m_Items)
  {
    if (const ezExposedParameters* pParams = m_pProxy->GetExposedParams(item.m_pObject))
    {
      ezHybridArray<ezVariant, 16> keys;
      EZ_VERIFY(m_pSourceObjectAccessor->GetKeys(item.m_pObject, m_pProp, keys).Succeeded(), "");
      for (auto& key : keys)
      {
        if (!pParams->Find(key.Get<ezString>()))
        {
          if (!bTestOnly)
          {
            bStuffDone = true;
            m_pSourceObjectAccessor->RemoveValue(item.m_pObject, m_pProp, key).LogFailure();
          }
          else
          {
            return true;
          }
        }
      }
    }
  }
  if (!bTestOnly)
    m_pSourceObjectAccessor->FinishTransaction();
  return bStuffDone;
}

bool ezQtExposedParametersPropertyWidget::FixKeyTypes(bool bTestOnly)
{
  bool bStuffDone = false;
  if (!bTestOnly)
    m_pSourceObjectAccessor->StartTransaction("Remove unused keys");
  for (const auto& item : m_Items)
  {
    if (const ezExposedParameters* pParams = m_pProxy->GetExposedParams(item.m_pObject))
    {
      ezHybridArray<ezVariant, 16> keys;
      EZ_VERIFY(m_pSourceObjectAccessor->GetKeys(item.m_pObject, m_pProp, keys).Succeeded(), "");
      for (auto& key : keys)
      {
        if (const auto* pParam = pParams->Find(key.Get<ezString>()))
        {
          ezVariant value;
          const ezRTTI* pType = pParam->m_DefaultValue.GetReflectedType();
          EZ_VERIFY(m_pSourceObjectAccessor->GetValue(item.m_pObject, m_pProp, value, key).Succeeded(), "");
          if (value.GetReflectedType() != pType)
          {
            if (!bTestOnly)
            {
              bStuffDone = true;
              ezVariantType::Enum type = pParam->m_DefaultValue.GetType();
              if (value.CanConvertTo(type))
              {
                m_pProxy->SetValue(item.m_pObject, m_pProp, value.ConvertTo(type), key).LogFailure();
              }
              else
              {
                m_pProxy->SetValue(item.m_pObject, m_pProp, pParam->m_DefaultValue, key).LogFailure();
              }
            }
            else
            {
              return true;
            }
          }
        }
      }
    }
  }
  if (!bTestOnly)
    m_pSourceObjectAccessor->FinishTransaction();
  return bStuffDone;
}

void ezQtExposedParametersPropertyWidget::UpdateActionState()
{
  m_bNeedsMetaDataUpdate = false;
  m_pRemoveUnusedAction->setEnabled(RemoveUnusedKeys(true));
  m_pFixTypesAction->setEnabled(FixKeyTypes(true));
  m_pFixMeButton->setVisible(m_pRemoveUnusedAction->isEnabled() || m_pFixTypesAction->isEnabled());
}
