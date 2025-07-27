#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GuiFoundation/PropertyGrid/AttributeDefaultStateProvider.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/PrefabDefaultStateProvider.h>
#include <GuiFoundation/PropertyGrid/VariantSubDefaultStateProvider.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, DefaultState)
  ON_CORESYSTEMS_STARTUP
  {
    ezDefaultState::RegisterDefaultStateProvider(ezAttributeDefaultStateProvider::CreateProvider);
    ezDefaultState::RegisterDefaultStateProvider(ezPrefabDefaultStateProvider::CreateProvider);
    ezDefaultState::RegisterDefaultStateProvider(ezVariantSubDefaultStateProvider::CreateProvider);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezDefaultState::UnregisterDefaultStateProvider(ezAttributeDefaultStateProvider::CreateProvider);
    ezDefaultState::UnregisterDefaultStateProvider(ezPrefabDefaultStateProvider::CreateProvider);
    ezDefaultState::UnregisterDefaultStateProvider(ezVariantSubDefaultStateProvider::CreateProvider);
  }
EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on


ezDynamicArray<ezDefaultState::CreateStateProviderFunc> ezDefaultState::s_Factories;

void ezDefaultState::RegisterDefaultStateProvider(CreateStateProviderFunc func)
{
  s_Factories.PushBack(func);
}

void ezDefaultState::UnregisterDefaultStateProvider(CreateStateProviderFunc func)
{
  s_Factories.RemoveAndCopy(func);
}

//////////////////////////////////////////////////////////////////////////

ezDefaultObjectState::ezDefaultObjectState(ezObjectAccessorBase* pAccessor, const ezArrayPtr<ezPropertySelection> selection)
{
  m_pAccessor = pAccessor;
  m_Selection = selection;
  m_Providers.Reserve(m_Selection.GetCount());
  for (const ezPropertySelection& sel : m_Selection)
  {
    auto& pProviders = m_Providers.ExpandAndGetRef();
    for (auto& func : ezDefaultState::s_Factories)
    {
      ezSharedPtr<ezDefaultStateProvider> pProvider = func(pAccessor, sel.m_pObject, nullptr);
      if (pProvider != nullptr)
      {
        pProviders.PushBack(std::move(pProvider));
      }
      pProviders.Sort([](const ezSharedPtr<ezDefaultStateProvider>& pA, const ezSharedPtr<ezDefaultStateProvider>& pB) -> bool
        { return pA->GetRootDepth() > pB->GetRootDepth(); });
    }
  }
}

ezColorGammaUB ezDefaultObjectState::GetBackgroundColor() const
{
  return m_Providers[0][0]->GetBackgroundColor();
}

ezString ezDefaultObjectState::GetStateProviderName() const
{
  return m_Providers[0][0]->GetStateProviderName();
}

bool ezDefaultObjectState::IsDefaultValue(const char* szProperty) const
{
  const ezAbstractProperty* pProp = m_Selection[0].m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty);
  return IsDefaultValue(pProp);
}

bool ezDefaultObjectState::IsDefaultValue(const ezAbstractProperty* pProp) const
{
  const ezUInt32 uiObjects = m_Providers.GetCount();
  for (ezUInt32 i = 0; i < uiObjects; i++)
  {
    ezDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    const bool bNewDefault = m_Providers[i][0]->IsDefaultValue(super, m_pAccessor, m_Selection[i].m_pObject, pProp);
    if (!bNewDefault)
      return false;
  }
  return true;
}

ezStatus ezDefaultObjectState::RevertProperty(const char* szProperty)
{
  const ezAbstractProperty* pProp = m_Selection[0].m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty);
  return RevertProperty(pProp);
}

ezStatus ezDefaultObjectState::RevertProperty(const ezAbstractProperty* pProp)
{
  const ezUInt32 uiObjects = m_Providers.GetCount();
  for (ezUInt32 i = 0; i < uiObjects; i++)
  {
    ezDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    ezStatus res = m_Providers[i][0]->RevertProperty(super, m_pAccessor, m_Selection[i].m_pObject, pProp);
    if (res.Failed())
      return res;
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezDefaultObjectState::RevertObject()
{
  const ezUInt32 uiObjects = m_Providers.GetCount();
  for (ezUInt32 i = 0; i < uiObjects; i++)
  {
    ezDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);

    ezHybridArray<const ezAbstractProperty*, 32> properties;
    m_Selection[i].m_pObject->GetType()->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetFlags().IsAnySet(ezPropertyFlags::Hidden | ezPropertyFlags::ReadOnly))
        continue;
      ezStatus res = m_Providers[i][0]->RevertProperty(super, m_pAccessor, m_Selection[i].m_pObject, pProp);
      if (res.Failed())
        return res;
    }
  }
  return ezStatus(EZ_SUCCESS);
}

ezVariant ezDefaultObjectState::GetDefaultValue(const char* szProperty, ezUInt32 uiSelectionIndex) const
{
  const ezAbstractProperty* pProp = m_Selection[0].m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty);
  return GetDefaultValue(pProp, uiSelectionIndex);
}

ezVariant ezDefaultObjectState::GetDefaultValue(const ezAbstractProperty* pProp, ezUInt32 uiSelectionIndex) const
{
  EZ_ASSERT_DEBUG(uiSelectionIndex < m_Selection.GetCount(), "Selection index is out of bounds.");
  ezDefaultStateProvider::SuperArray super = m_Providers[uiSelectionIndex].GetArrayPtr().GetSubArray(1);
  return m_Providers[uiSelectionIndex][0]->GetDefaultValue(super, m_pAccessor, m_Selection[uiSelectionIndex].m_pObject, pProp);
}

//////////////////////////////////////////////////////////////////////////

ezDefaultContainerState::ezDefaultContainerState(ezObjectAccessorBase* pAccessor, const ezArrayPtr<ezPropertySelection> selection, const char* szProperty)
{
  m_pAccessor = pAccessor;
  m_Selection = selection;
  // We assume selections can only contain objects of the same (base) type.
  m_pProp = szProperty ? selection[0].m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty) : nullptr;
  m_Providers.Reserve(m_Selection.GetCount());
  for (const ezPropertySelection& sel : m_Selection)
  {
    auto& pProviders = m_Providers.ExpandAndGetRef();
    for (auto& func : ezDefaultState::s_Factories)
    {
      ezSharedPtr<ezDefaultStateProvider> pProvider = func(pAccessor, sel.m_pObject, m_pProp);
      if (pProvider != nullptr)
      {
        pProviders.PushBack(std::move(pProvider));
      }
      pProviders.Sort([](const ezSharedPtr<ezDefaultStateProvider>& pA, const ezSharedPtr<ezDefaultStateProvider>& pB) -> bool
        { return pA->GetRootDepth() > pB->GetRootDepth(); });
    }
  }
}

ezColorGammaUB ezDefaultContainerState::GetBackgroundColor() const
{
  return m_Providers[0][0]->GetBackgroundColor();
}

ezString ezDefaultContainerState::GetStateProviderName() const
{
  return m_Providers[0][0]->GetStateProviderName();
}

bool ezDefaultContainerState::IsDefaultElement(ezVariant index) const
{
  const ezUInt32 uiObjects = m_Providers.GetCount();
  for (ezUInt32 i = 0; i < uiObjects; i++)
  {
    ezDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    EZ_ASSERT_DEBUG(index.IsValid() || m_Selection[i].m_Index.IsValid(), "If ezDefaultContainerState is constructed without giving an indices in the selection, one must be provided on the IsDefaultElement call.");
    const bool bNewDefault = m_Providers[i][0]->IsDefaultValue(super, m_pAccessor, m_Selection[i].m_pObject, m_pProp, index.IsValid() ? index : m_Selection[i].m_Index);
    if (!bNewDefault)
      return false;
  }
  return true;
}

bool ezDefaultContainerState::IsDefaultContainer() const
{
  const ezUInt32 uiObjects = m_Providers.GetCount();
  for (ezUInt32 i = 0; i < uiObjects; i++)
  {
    ezDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    const bool bNewDefault = m_Providers[i][0]->IsDefaultValue(super, m_pAccessor, m_Selection[i].m_pObject, m_pProp);
    if (!bNewDefault)
      return false;
  }
  return true;
}

ezStatus ezDefaultContainerState::RevertElement(ezVariant index)
{
  const ezUInt32 uiObjects = m_Providers.GetCount();
  for (ezUInt32 i = 0; i < uiObjects; i++)
  {
    ezDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    EZ_ASSERT_DEBUG(index.IsValid() || m_Selection[i].m_Index.IsValid(), "If ezDefaultContainerState is constructed without giving an indices in the selection, one must be provided on the RevertElement call.");
    ezStatus res = m_Providers[i][0]->RevertProperty(super, m_pAccessor, m_Selection[i].m_pObject, m_pProp, index.IsValid() ? index : m_Selection[i].m_Index);
    if (res.Failed())
      return res;
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezDefaultContainerState::RevertContainer()
{
  const ezUInt32 uiObjects = m_Providers.GetCount();
  for (ezUInt32 i = 0; i < uiObjects; i++)
  {
    ezDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    ezStatus res = m_Providers[i][0]->RevertProperty(super, m_pAccessor, m_Selection[i].m_pObject, m_pProp);
    if (res.Failed())
      return res;
  }
  return ezStatus(EZ_SUCCESS);
}

ezVariant ezDefaultContainerState::GetDefaultElement(ezVariant index, ezUInt32 uiSelectionIndex) const
{
  EZ_ASSERT_DEBUG(uiSelectionIndex < m_Selection.GetCount(), "Selection index is out of bounds.");
  ezDefaultStateProvider::SuperArray super = m_Providers[uiSelectionIndex].GetArrayPtr().GetSubArray(1);
  return m_Providers[uiSelectionIndex][0]->GetDefaultValue(super, m_pAccessor, m_Selection[uiSelectionIndex].m_pObject, m_pProp, index);
}

ezVariant ezDefaultContainerState::GetDefaultContainer(ezUInt32 uiSelectionIndex) const
{
  EZ_ASSERT_DEBUG(uiSelectionIndex < m_Selection.GetCount(), "Selection index is out of bounds.");
  ezDefaultStateProvider::SuperArray super = m_Providers[uiSelectionIndex].GetArrayPtr().GetSubArray(1);
  return m_Providers[uiSelectionIndex][0]->GetDefaultValue(super, m_pAccessor, m_Selection[uiSelectionIndex].m_pObject, m_pProp);
}

//////////////////////////////////////////////////////////////////////////


bool ezDefaultStateProvider::IsDefaultValue(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  const ezVariant def = GetDefaultValue(superPtr, pAccessor, pObject, pProp, index);
  ezVariant value;
  pAccessor->GetValue(pObject, pProp, value, index).LogFailure();

  const bool bIsValueType = ezReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags);
  if (index.IsValid() && !bIsValueType)
  {
    // #TODO we do not support reverting entire objects just yet.
    return true;
  }

  return def == value;
}

ezStatus ezDefaultStateProvider::RevertProperty(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  const bool bIsValueType = ezReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags);
  if (!bIsValueType)
  {
    EZ_ASSERT_DEBUG(!index.IsValid(), "Reverting non-value type container elements is not supported yet. IsDefaultValue should have returned true to prevent this call from being allowed.");

    return RevertObjectContainer(superPtr, pAccessor, pObject, pProp);
  }

  ezDeque<ezAbstractGraphDiffOperation> diff;
  auto& op = diff.ExpandAndGetRef();
  op.m_Node = pObject->GetGuid();
  op.m_Operation = ezAbstractGraphDiffOperation::Op::PropertyChanged;
  op.m_sProperty = pProp->GetPropertyName();
  op.m_uiTypeVersion = 0;
  if (index.IsValid())
  {
    ezVariant def = GetDefaultValue(superPtr, pAccessor, pObject, pProp, index);
    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      {
        EZ_ASSERT_DEBUG(index.CanConvertTo<ezInt32>(), "Array / Set indices must be integers.");
        EZ_SUCCEED_OR_RETURN(pAccessor->GetValue(pObject, pProp, op.m_Value));
        EZ_ASSERT_DEBUG(op.m_Value.IsA<ezVariantArray>(), "");

        ezVariantArray& currentValue2 = op.m_Value.GetWritable<ezVariantArray>();
        currentValue2[index.ConvertTo<ezUInt32>()] = def;
      }
      break;
      case ezPropertyCategory::Map:
      {
        EZ_ASSERT_DEBUG(index.IsString(), "Map indices must be strings.");
        EZ_SUCCEED_OR_RETURN(pAccessor->GetValue(pObject, pProp, op.m_Value));
        EZ_ASSERT_DEBUG(op.m_Value.IsA<ezVariantDictionary>(), "");

        ezVariantDictionary& currentValue2 = op.m_Value.GetWritable<ezVariantDictionary>();
        currentValue2[index.ConvertTo<ezString>()] = def;
      }
      break;
      default:
        break;
    }
  }
  else
  {
    ezVariant def = GetDefaultValue(superPtr, pAccessor, pObject, pProp, index);
    op.m_Value = def;
  }

  ezDocumentObjectConverterReader::ApplyDiffToObject(pAccessor, pObject, diff);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezDefaultStateProvider::RevertObjectContainer(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
{
  ezDeque<ezAbstractGraphDiffOperation> diff;
  ezStatus res = CreateRevertContainerDiff(superPtr, pAccessor, pObject, pProp, diff);
  if (res.Succeeded())
  {
    ezDocumentObjectConverterReader::ApplyDiffToObject(pAccessor, pObject, diff);
  }
  return res;
}

bool ezDefaultStateProvider::DoesVariantMatchProperty(const ezVariant& value, const ezAbstractProperty* pProp, ezVariant index)
{
  const bool bIsValueType = ezReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags);

  if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
    return true;

  auto MatchesElementType = [&](const ezVariant& value2) -> bool
  {
    if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
    {
      return value2.IsNumber() && !value2.IsFloatingPoint();
    }
    else if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType))
    {
      return value2.CanConvertTo(pProp->GetSpecificType()->GetVariantType());
    }
    else if (bIsValueType)
    {
      return value2.GetReflectedType() == pProp->GetSpecificType();
    }
    else
    {
      return value2.IsA<ezUuid>();
    }
  };

  switch (pProp->GetCategory())
  {
    case ezPropertyCategory::Member:
    {
      return MatchesElementType(value);
    }
    break;
    case ezPropertyCategory::Array:
    case ezPropertyCategory::Set:
    {
      if (index.IsValid())
      {
        return MatchesElementType(value);
      }
      else
      {
        if (value.IsA<ezVariantArray>())
        {
          const ezVariantArray& valueArray = value.Get<ezVariantArray>();
          return std::all_of(cbegin(valueArray), cend(valueArray), MatchesElementType);
        }
      }
    }
    break;
    case ezPropertyCategory::Map:
    {
      if (index.IsValid())
      {
        return MatchesElementType(value);
      }
      else
      {
        if (value.IsA<ezVariantDictionary>())
        {
          const ezVariantDictionary& valueDict = value.Get<ezVariantDictionary>();
          return std::all_of(cbegin(valueDict), cend(valueDict), [&](const auto& it)
            { return MatchesElementType(it.Value()); });
        }
      }
    }
    break;
    default:
      break;
  }
  return false;
}
