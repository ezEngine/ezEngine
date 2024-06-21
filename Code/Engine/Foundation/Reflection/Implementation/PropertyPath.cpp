#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/UniquePtr.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezPropertyPathStep, ezNoBase, 1, ezRTTIDefaultAllocator<ezPropertyPathStep>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Property", m_sProperty),
    EZ_MEMBER_PROPERTY("Index", m_Index),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezPropertyPath::ezPropertyPath() = default;
ezPropertyPath::~ezPropertyPath() = default;

bool ezPropertyPath::IsValid() const
{
  return m_bIsValid;
}

ezResult ezPropertyPath::InitializeFromPath(const ezRTTI& rootObjectRtti, const char* szPath)
{
  m_bIsValid = false;

  const ezStringBuilder sPathParts = szPath;
  ezStringBuilder sIndex;
  ezStringBuilder sFieldName;

  ezHybridArray<ezStringView, 4> parts;
  sPathParts.Split(false, parts, "/");

  // an empty path is valid as well

  m_PathSteps.Clear();
  m_PathSteps.Reserve(parts.GetCount());

  const ezRTTI* pCurRtti = &rootObjectRtti;

  for (const ezStringView& part : parts)
  {
    if (part.EndsWith("]"))
    {
      const char* szBracket = part.FindSubString("[");

      sIndex.SetSubString_FromTo(szBracket + 1, part.GetEndPointer() - 1);

      sFieldName.SetSubString_FromTo(part.GetStartPointer(), szBracket);
    }
    else
    {
      sFieldName = part;
      sIndex.Clear();
    }

    const ezAbstractProperty* pAbsProp = pCurRtti->FindPropertyByName(sFieldName);

    if (pAbsProp == nullptr)
      return EZ_FAILURE;

    auto& step = m_PathSteps.ExpandAndGetRef();
    step.m_pProperty = pAbsProp;

    if (pAbsProp->GetCategory() == ezPropertyCategory::Array)
    {
      if (sIndex.IsEmpty())
      {
        step.m_Index = ezVariant();
      }
      else
      {
        ezInt32 iIndex;
        EZ_SUCCEED_OR_RETURN(ezConversionUtils::StringToInt(sIndex, iIndex));
        step.m_Index = iIndex;
      }
    }
    else if (pAbsProp->GetCategory() == ezPropertyCategory::Set)
    {
      if (sIndex.IsEmpty())
      {
        step.m_Index = ezVariant();
      }
      else
      {
        return EZ_FAILURE;
      }
    }
    else if (pAbsProp->GetCategory() == ezPropertyCategory::Map)
    {
      step.m_Index = sIndex.IsEmpty() ? ezVariant() : ezVariant(sIndex.GetData());
    }

    pCurRtti = pAbsProp->GetSpecificType();
  }

  m_bIsValid = true;
  return EZ_SUCCESS;
}

ezResult ezPropertyPath::InitializeFromPath(const ezRTTI* pRootObjectRtti, const ezArrayPtr<const ezPropertyPathStep> path)
{
  m_bIsValid = false;

  m_PathSteps.Clear();
  m_PathSteps.Reserve(path.GetCount());

  const ezRTTI* pCurRtti = pRootObjectRtti;
  for (const ezPropertyPathStep& pathStep : path)
  {
    const ezAbstractProperty* pAbsProp = pCurRtti->FindPropertyByName(pathStep.m_sProperty);
    if (pAbsProp == nullptr)
      return EZ_FAILURE;

    auto& step = m_PathSteps.ExpandAndGetRef();
    step.m_pProperty = pAbsProp;
    step.m_Index = pathStep.m_Index;

    pCurRtti = pAbsProp->GetSpecificType();
  }

  m_bIsValid = true;
  return EZ_SUCCESS;
}

ezResult ezPropertyPath::WriteToLeafObject(void* pRootObject, const ezRTTI& type, ezDelegate<void(void* pLeaf, const ezRTTI& pType)> func) const
{
  EZ_ASSERT_DEBUG(
    m_PathSteps.IsEmpty() || m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetTypeFlags().IsSet(ezTypeFlags::Class),
    "To resolve the leaf object the path needs to be empty or end in a class.");
  return ResolvePath(pRootObject, &type, m_PathSteps.GetArrayPtr(), true, func);
}

ezResult ezPropertyPath::ReadFromLeafObject(void* pRootObject, const ezRTTI& type, ezDelegate<void(void* pLeaf, const ezRTTI& pType)> func) const
{
  EZ_ASSERT_DEBUG(
    m_PathSteps.IsEmpty() || m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetTypeFlags().IsSet(ezTypeFlags::Class),
    "To resolve the leaf object the path needs to be empty or end in a class.");
  return ResolvePath(pRootObject, &type, m_PathSteps.GetArrayPtr(), false, func);
}

ezResult ezPropertyPath::WriteProperty(
  void* pRootObject, const ezRTTI& type, ezDelegate<void(void* pLeafObject, const ezRTTI& pLeafType, const ezAbstractProperty* pProp, const ezVariant& index)> func) const
{
  EZ_ASSERT_DEBUG(!m_PathSteps.IsEmpty(), "Call InitializeFromPath before WriteToObject");
  return ResolvePath(pRootObject, &type, m_PathSteps.GetArrayPtr().GetSubArray(0, m_PathSteps.GetCount() - 1), true,
    [this, &func](void* pLeafObject, const ezRTTI& leafType)
    {
      auto& lastStep = m_PathSteps[m_PathSteps.GetCount() - 1];
      func(pLeafObject, leafType, lastStep.m_pProperty, lastStep.m_Index);
    });
}

ezResult ezPropertyPath::ReadProperty(
  void* pRootObject, const ezRTTI& type, ezDelegate<void(void* pLeafObject, const ezRTTI& pLeafType, const ezAbstractProperty* pProp, const ezVariant& index)> func) const
{
  EZ_ASSERT_DEBUG(m_bIsValid, "Call InitializeFromPath before WriteToObject");
  return ResolvePath(pRootObject, &type, m_PathSteps.GetArrayPtr().GetSubArray(0, m_PathSteps.GetCount() - 1), false,
    [this, &func](void* pLeafObject, const ezRTTI& leafType)
    {
      auto& lastStep = m_PathSteps[m_PathSteps.GetCount() - 1];
      func(pLeafObject, leafType, lastStep.m_pProperty, lastStep.m_Index);
    });
}

void ezPropertyPath::SetValue(void* pRootObject, const ezRTTI& type, const ezVariant& value) const
{
  // EZ_ASSERT_DEBUG(!m_PathSteps.IsEmpty() &&
  //                    value.CanConvertTo(m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetVariantType()),
  //                "The given value does not match the type at the given path.");

  WriteProperty(pRootObject, type, [&value](void* pLeaf, const ezRTTI& type, const ezAbstractProperty* pProp, const ezVariant& index)
    {
      EZ_IGNORE_UNUSED(type);

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Member:
        ezReflectionUtils::SetMemberPropertyValue(static_cast<const ezAbstractMemberProperty*>(pProp), pLeaf, value);
        break;
      case ezPropertyCategory::Array:
        ezReflectionUtils::SetArrayPropertyValue(static_cast<const ezAbstractArrayProperty*>(pProp), pLeaf, index.Get<ezInt32>(), value);
        break;
      case ezPropertyCategory::Map:
        ezReflectionUtils::SetMapPropertyValue(static_cast<const ezAbstractMapProperty*>(pProp), pLeaf, index.Get<ezString>(), value);
        break;
      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        break;
    } })
    .IgnoreResult();
}

void ezPropertyPath::GetValue(void* pRootObject, const ezRTTI& type, ezVariant& out_value) const
{
  // EZ_ASSERT_DEBUG(!m_PathSteps.IsEmpty() &&
  //                    m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetVariantType() != ezVariantType::Invalid,
  //                "The property path of value {} cannot be stored in an ezVariant.", m_PathSteps[m_PathSteps.GetCount() -
  //                1].m_pProperty->GetSpecificType()->GetTypeName());

  ReadProperty(pRootObject, type, [&out_value](void* pLeaf, const ezRTTI& type, const ezAbstractProperty* pProp, const ezVariant& index)
    {
      EZ_IGNORE_UNUSED(type);

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Member:
        out_value = ezReflectionUtils::GetMemberPropertyValue(static_cast<const ezAbstractMemberProperty*>(pProp), pLeaf);
        break;
      case ezPropertyCategory::Array:
        out_value = ezReflectionUtils::GetArrayPropertyValue(static_cast<const ezAbstractArrayProperty*>(pProp), pLeaf, index.Get<ezInt32>());
        break;
      case ezPropertyCategory::Map:
        out_value = ezReflectionUtils::GetMapPropertyValue(static_cast<const ezAbstractMapProperty*>(pProp), pLeaf, index.Get<ezString>());
        break;
      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        break;
    } })
    .IgnoreResult();
}

ezResult ezPropertyPath::ResolvePath(void* pCurrentObject, const ezRTTI* pType, const ezArrayPtr<const ResolvedStep> path, bool bWriteToObject,
  const ezDelegate<void(void* pLeaf, const ezRTTI& pType)>& func)
{
  if (path.IsEmpty())
  {
    func(pCurrentObject, *pType);
    return EZ_SUCCESS;
  }
  else // Recurse
  {
    const ezAbstractProperty* pProp = path[0].m_pProperty;
    const ezRTTI* pPropType = pProp->GetSpecificType();

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Member:
      {
        auto pSpecific = static_cast<const ezAbstractMemberProperty*>(pProp);
        if (pPropType->GetProperties().GetCount() > 0)
        {
          void* pSubObject = pSpecific->GetPropertyPointer(pCurrentObject);
          // Do we have direct access to the property?
          if (pSubObject != nullptr)
          {
            return ResolvePath(pSubObject, pProp->GetSpecificType(), path.GetSubArray(1), bWriteToObject, func);
          }
          // If the property is behind an accessor, we need to retrieve it first.
          else if (pPropType->GetAllocator()->CanAllocate())
          {
            void* pRetrievedSubObject = pPropType->GetAllocator()->Allocate<void>();
            pSpecific->GetValuePtr(pCurrentObject, pRetrievedSubObject);

            ezResult res = ResolvePath(pRetrievedSubObject, pProp->GetSpecificType(), path.GetSubArray(1), bWriteToObject, func);

            if (bWriteToObject)
              pSpecific->SetValuePtr(pCurrentObject, pRetrievedSubObject);

            pPropType->GetAllocator()->Deallocate(pRetrievedSubObject);
            return res;
          }
          else
          {
            EZ_REPORT_FAILURE("Non-allocatable property should not be part of an object chain!");
          }
        }
      }
      break;
      case ezPropertyCategory::Array:
      {
        auto pSpecific = static_cast<const ezAbstractArrayProperty*>(pProp);

        if (pPropType->GetAllocator()->CanAllocate())
        {
          const ezUInt32 uiIndex = path[0].m_Index.ConvertTo<ezUInt32>();
          if (uiIndex >= pSpecific->GetCount(pCurrentObject))
            return EZ_FAILURE;

          void* pSubObject = pPropType->GetAllocator()->Allocate<void>();
          pSpecific->GetValue(pCurrentObject, uiIndex, pSubObject);

          ezResult res = ResolvePath(pSubObject, pProp->GetSpecificType(), path.GetSubArray(1), bWriteToObject, func);

          if (bWriteToObject)
            pSpecific->SetValue(pCurrentObject, uiIndex, pSubObject);

          pPropType->GetAllocator()->Deallocate(pSubObject);
          return res;
        }
        else
        {
          EZ_REPORT_FAILURE("Non-allocatable property should not be part of an object chain!");
        }
      }
      break;
      case ezPropertyCategory::Map:
      {
        auto pSpecific = static_cast<const ezAbstractMapProperty*>(pProp);
        const ezString& sKey = path[0].m_Index.Get<ezString>();
        if (!pSpecific->Contains(pCurrentObject, sKey))
          return EZ_FAILURE;

        if (pPropType->GetAllocator()->CanAllocate())
        {
          void* pSubObject = pPropType->GetAllocator()->Allocate<void>();

          pSpecific->GetValue(pCurrentObject, sKey, pSubObject);

          ezResult res = ResolvePath(pSubObject, pProp->GetSpecificType(), path.GetSubArray(1), bWriteToObject, func);

          if (bWriteToObject)
            pSpecific->Insert(pCurrentObject, sKey, pSubObject);

          pPropType->GetAllocator()->Deallocate(pSubObject);
          return res;
        }
        else
        {
          EZ_REPORT_FAILURE("Non-allocatable property should not be part of an object chain!");
        }
      }
      break;
      case ezPropertyCategory::Set:
      default:
      {
        EZ_REPORT_FAILURE("Property of type Set should not be part of an object chain!");
      }
      break;
    }
    return EZ_FAILURE;
  }
}



EZ_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_PropertyPath);
