#include <EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/GUI/ExposedParametersTypeRegistry.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/GroupBoxBase.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezExposedParameterCommandAccessor::ezExposedParameterCommandAccessor(ezObjectAccessorBase* pSource,
                                                                     const ezAbstractProperty* pParameterProp,
                                                                     const ezAbstractProperty* pParameterSourceProp)
    : ezObjectProxyAccessor(pSource)
    , m_pParameterProp(pParameterProp)
    , m_pParameterSourceProp(pParameterSourceProp)
{
}

ezStatus ezExposedParameterCommandAccessor::GetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value,
                                                     ezVariant index /*= ezVariant()*/)
{
  if (IsExposedProperty(pObject, pProp))
    pProp = m_pParameterProp;

  ezStatus res = ezObjectProxyAccessor::GetValue(pObject, pProp, out_value, index);
  if (res.Failed() && m_pParameterProp == pProp && index.IsA<ezString>())
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

ezStatus ezExposedParameterCommandAccessor::SetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp,
                                                     const ezVariant& newValue, ezVariant index /*= ezVariant()*/)
{
  if (IsExposedProperty(pObject, pProp))
    pProp = m_pParameterProp;

  ezStatus res = ezObjectProxyAccessor::SetValue(pObject, pProp, newValue, index);
  // As we pretend the exposed params always exist the actual SetValue will fail if this is not actually true,
  // so we redirect to insert to make it true.
  if (res.Failed() && m_pParameterProp == pProp && index.IsA<ezString>())
  {
    return InsertValue(pObject, pProp, newValue, index);
  }
  return res;
}

ezStatus ezExposedParameterCommandAccessor::RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp,
                                                        ezVariant index /*= ezVariant()*/)
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
    GetKeys(pObject, pProp, keys);
    out_iCount = keys.GetCount();
    return ezStatus(EZ_SUCCESS);
  }
  return ezObjectProxyAccessor::GetCount(pObject, pProp, out_iCount);
}

ezStatus ezExposedParameterCommandAccessor::GetKeys(const ezDocumentObject* pObject, const ezAbstractProperty* pProp,
                                                    ezHybridArray<ezVariant, 16>& out_keys)
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

ezStatus ezExposedParameterCommandAccessor::GetValues(const ezDocumentObject* pObject, const ezAbstractProperty* pProp,
                                                      ezHybridArray<ezVariant, 16>& out_values)
{
  if (m_pParameterProp == pProp)
  {
    ezHybridArray<ezVariant, 16> keys;
    GetKeys(pObject, pProp, keys);
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
    return std::any_of(cbegin(props), cend(props), [pProp](const ezAbstractProperty* prop) { return prop == pProp; });
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////

void ezQtExposedParameterPropertyWidget::InternalSetValue(const ezVariant& value)
{
  ezVariantType::Enum commonType = ezVariantType::Invalid;
  const bool sameType = GetCommonVariantSubType(m_Items, m_pProp, commonType);
  const ezRTTI* pNewtSubType = commonType != ezVariantType::Invalid ? ezReflectionUtils::GetTypeFromVariant(commonType) : nullptr;

  ezExposedParameterCommandAccessor* proxy = static_cast<ezExposedParameterCommandAccessor*>(m_pObjectAccessor);
  if (auto type = proxy->GetCommonExposedParamsType(m_Items))
  {
    if (auto prop = type->FindPropertyByName(m_Items[0].m_Index.Get<ezString>()))
    {
      auto paramDefault = ezToolsReflectionUtils::GetStorageDefault(prop);
      if (paramDefault.GetType() == commonType)
      {
        if (prop->GetSpecificType() != m_pCurrentSubType || m_pWidget == nullptr)
        {
          if (m_pWidget)
          {
            m_pWidget->PrepareToDie();
            m_pWidget->deleteLater();
            m_pWidget = nullptr;
          }
          m_pCurrentSubType = pNewtSubType;
          m_pWidget = ezQtPropertyGridWidget::CreateMemberPropertyWidget(prop);
          if (!m_pWidget)
            m_pWidget = new ezQtUnsupportedPropertyWidget("Unsupported type");

          m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
          m_pWidget->setParent(this);
          m_pLayout->addWidget(m_pWidget);
          m_pWidget->Init(m_pGrid, m_pObjectAccessor, type, prop);
        }
        m_pWidget->SetSelection(m_Items);
        return;
      }
    }
  }
  ezQtVariantPropertyWidget::InternalSetValue(value);
}

//////////////////////////////////////////////////////////////////////////

ezQtExposedParametersPropertyWidget::ezQtExposedParametersPropertyWidget() {}

ezQtExposedParametersPropertyWidget::~ezQtExposedParametersPropertyWidget()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(
      ezMakeDelegate(&ezQtExposedParametersPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.RemoveEventHandler(
      ezMakeDelegate(&ezQtExposedParametersPropertyWidget::CommandHistoryEventHandler, this));
}

void ezQtExposedParametersPropertyWidget::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  ezQtPropertyStandardTypeContainerWidget::SetSelection(items);
  UpdateActionState();
}

void ezQtExposedParametersPropertyWidget::OnInit()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.AddEventHandler(
      ezMakeDelegate(&ezQtExposedParametersPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.AddEventHandler(
      ezMakeDelegate(&ezQtExposedParametersPropertyWidget::CommandHistoryEventHandler, this));

  const auto* pAttrib = m_pProp->GetAttributeByType<ezExposedParametersAttribute>();
  EZ_ASSERT_DEV(pAttrib,
                "ezQtExposedParametersPropertyWidget was created for a property that does not have the ezExposedParametersAttribute.");
  m_sExposedParamProperty = pAttrib->GetParametersSource();
  const ezAbstractProperty* pParameterSourceProp = m_pType->FindPropertyByName(m_sExposedParamProperty);
  EZ_ASSERT_DEV(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", m_sExposedParamProperty,
                m_pType->GetTypeName());
  m_pSourceObjectAccessor = m_pObjectAccessor;
  m_Proxy = EZ_DEFAULT_NEW(ezExposedParameterCommandAccessor, m_pSourceObjectAccessor, m_pProp, pParameterSourceProp);
  m_pObjectAccessor = m_Proxy.Borrow();

  ezQtPropertyStandardTypeContainerWidget::OnInit();

  {
    // Help button to indicate exposed parameter mismatches.
    m_pFixMeButton = new QToolButton();
    m_pFixMeButton->setAutoRaise(true);
    m_pFixMeButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
    m_pFixMeButton->setIcon(ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Attention16.png"));
    auto sp = m_pFixMeButton->sizePolicy();
    sp.setVerticalPolicy(QSizePolicy::Ignored);
    m_pFixMeButton->setSizePolicy(sp);
    QMenu* pFixMeMenu = new QMenu(m_pFixMeButton);
    {
      m_pRemoveUnusedAction = pFixMeMenu->addAction(QStringLiteral("Remove unused keys"));
      m_pRemoveUnusedAction->setToolTip(
          QStringLiteral("The map contains keys that are no longer used by the asset's exposed parameters and thus can be removed."));
      connect(m_pRemoveUnusedAction, &QAction::triggered, this, [this](bool checked) { RemoveUnusedKeys(false); });
    }
    {
      m_pFixTypesAction = pFixMeMenu->addAction(QStringLiteral("Fix keys with wrong types"));
      connect(m_pFixTypesAction, &QAction::triggered, this, [this](bool checked) { FixKeyTypes(false); });
    }
    m_pFixMeButton->setMenu(pFixMeMenu);

    auto layout = qobject_cast<QHBoxLayout*>(m_pGroup->GetHeader()->layout());
    layout->insertWidget(layout->count() - 1, m_pFixMeButton);
  }
}

ezQtPropertyWidget* ezQtExposedParametersPropertyWidget::CreateWidget(ezUInt32 index)
{
  return new ezQtExposedParameterPropertyWidget();
}


void ezQtExposedParametersPropertyWidget::UpdateElement(ezUInt32 index)
{
  ezQtPropertyStandardTypeContainerWidget::UpdateElement(index);
  Element& elem = m_Elements[index];
  const auto& selection = elem.m_pWidget->GetSelection();
  bool isDefault = true;
  for (const auto& item : selection)
  {
    ezVariant value;
    ezStatus res = m_pSourceObjectAccessor->GetValue(item.m_pObject, m_pProp, value, item.m_Index);
    if (res.Succeeded())
    {
      // In case we successfully read the value from the source accessor (not the proxy that pretends all exposed params exist)
      // we now the value is overwritten as in the default case the map index would not exist.
      isDefault = false;
      break;
    }
  }
  elem.m_pWidget->SetIsDefault(isDefault);
  elem.m_pSubGroup->SetBoldTitle(!isDefault);
}

void ezQtExposedParametersPropertyWidget::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (IsUndead())
    return;

  if (std::none_of(cbegin(m_Items), cend(m_Items), [=](const ezPropertySelection& sel) { return e.m_pObject == sel.m_pObject; }))
    return;

  if (!m_bNeedsUpdate && m_sExposedParamProperty == e.m_sProperty)
  {
    m_bNeedsUpdate = true;
    // In case the change happened outside the command history we have to update at once.
    if (!m_pGrid->GetCommandHistory()->IsInTransaction() && !m_pGrid->GetCommandHistory()->IsInUndoRedo())
      FlushQueuedChanges();
  }
  if (!m_bNeedsMetaDataUpdate && m_pProp->GetPropertyName() == e.m_sProperty)
  {
    m_bNeedsMetaDataUpdate = true;
    if (!m_pGrid->GetCommandHistory()->IsInTransaction() && !m_pGrid->GetCommandHistory()->IsInUndoRedo())
      FlushQueuedChanges();
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
      FlushQueuedChanges();
    }
    break;

    default:
      break;
  }
}

void ezQtExposedParametersPropertyWidget::FlushQueuedChanges()
{
  if (m_bNeedsUpdate)
  {
    m_bNeedsUpdate = false;
    SetSelection(m_Items);
  }
  if (m_bNeedsMetaDataUpdate)
  {
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
    if (const ezExposedParameters* pParams = m_Proxy->GetExposedParams(item.m_pObject))
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
    if (const ezExposedParameters* pParams = m_Proxy->GetExposedParams(item.m_pObject))
    {
      ezHybridArray<ezVariant, 16> keys;
      EZ_VERIFY(m_pSourceObjectAccessor->GetKeys(item.m_pObject, m_pProp, keys).Succeeded(), "");
      for (auto& key : keys)
      {
        if (const auto* pParam = pParams->Find(key.Get<ezString>()))
        {
          ezVariant value;
          const ezVariantType::Enum type = pParam->m_DefaultValue.GetType();
          EZ_VERIFY(m_pSourceObjectAccessor->GetValue(item.m_pObject, m_pProp, value, key).Succeeded(), "");
          if (value.GetType() != type)
          {
            if (!bTestOnly)
            {
              bStuffDone = true;
              if (value.CanConvertTo(type))
              {
                m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, value.ConvertTo(type), key).LogFailure();
              }
              else
              {
                m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, ezReflectionUtils::GetDefaultVariantFromType(type), key).LogFailure();
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
