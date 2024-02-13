#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

namespace
{
  struct GetDoubleFunc
  {
    GetDoubleFunc(const ezVariant& value)
      : m_Value(value)
    {
    }
    template <typename T>
    void operator()()
    {
      if (m_Value.CanConvertTo<double>())
      {
        m_fValue = m_Value.ConvertTo<double>();
        m_bValid = true;
      }
    }

    const ezVariant& m_Value;
    double m_fValue = 0;
    bool m_bValid = false;
  };

  template <>
  void GetDoubleFunc::operator()<ezAngle>()
  {
    m_fValue = m_Value.Get<ezAngle>().GetDegree();
    m_bValid = true;
  }

  template <>
  void GetDoubleFunc::operator()<ezTime>()
  {
    m_fValue = m_Value.Get<ezTime>().GetSeconds();
    m_bValid = true;
  }

  struct GetVariantFunc
  {
    GetVariantFunc(double fValue, ezVariantType::Enum type, ezVariant& out_value)
      : m_fValue(fValue)
      , m_Type(type)
      , m_Value(out_value)
    {
    }
    template <typename T>
    void operator()()
    {
      m_Value = m_fValue;
      if (m_Value.CanConvertTo(m_Type))
      {
        m_Value = m_Value.ConvertTo(m_Type);
        m_bValid = true;
      }
      else
      {
        m_Value = ezVariant();
      }
    }

    double m_fValue;
    ezVariantType::Enum m_Type;
    ezVariant& m_Value;
    bool m_bValid = false;
  };

  template <>
  void GetVariantFunc::operator()<ezAngle>()
  {
    m_Value = ezAngle::MakeFromDegree((float)m_fValue);
    m_bValid = true;
  }

  template <>
  void GetVariantFunc::operator()<ezTime>()
  {
    m_Value = ezTime::MakeFromSeconds(m_fValue);
    m_bValid = true;
  }
} // namespace
////////////////////////////////////////////////////////////////////////
// ezToolsReflectionUtils public functions
////////////////////////////////////////////////////////////////////////

ezVariantType::Enum ezToolsReflectionUtils::GetStorageType(const ezAbstractProperty* pProperty)
{
  ezVariantType::Enum type = ezVariantType::Uuid;

  const bool bIsValueType = ezReflectionUtils::IsValueType(pProperty);

  switch (pProperty->GetCategory())
  {
    case ezPropertyCategory::Member:
    {
      if (bIsValueType)
        type = pProperty->GetSpecificType()->GetVariantType();
      else if (pProperty->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
        type = ezVariantType::Int64;
    }
    break;
    case ezPropertyCategory::Array:
    case ezPropertyCategory::Set:
    {
      type = ezVariantType::VariantArray;
    }
    break;
    case ezPropertyCategory::Map:
    {
      type = ezVariantType::VariantDictionary;
    }
    break;
    default:
      break;
  }

  // We can't 'store' a string view as it has no ownership of its own. Thus, all string views are stored as strings instead.
  if (type == ezVariantType::StringView)
    type = ezVariantType::String;

  return type;
}

ezVariant ezToolsReflectionUtils::GetStorageDefault(const ezAbstractProperty* pProperty)
{
  const ezDefaultValueAttribute* pAttrib = pProperty->GetAttributeByType<ezDefaultValueAttribute>();
  const bool bIsValueType = ezReflectionUtils::IsValueType(pProperty);

  switch (pProperty->GetCategory())
  {
    case ezPropertyCategory::Member:
    {
      const ezVariantType::Enum memberType = GetStorageType(pProperty);
      ezVariant value = ezReflectionUtils::GetDefaultValue(pProperty);
      // Sometimes, the default value does not match the storage type, e.g. ezStringView is stored as ezString as it needs to be stored in the editor representation, but the reflection can still return default values matching ezStringView (constants for example).
      if (bIsValueType && value.GetType() != memberType)
        value = value.ConvertTo(memberType);

      EZ_ASSERT_DEBUG(!value.IsValid() || memberType == value.GetType(), "Default value type does not match the storage type of the property");
      return value;
    }
    break;
    case ezPropertyCategory::Array:
    case ezPropertyCategory::Set:
    {
      if (bIsValueType && pAttrib && pAttrib->GetValue().IsA<ezVariantArray>())
      {
        auto elementType = pProperty->GetFlags().IsSet(ezPropertyFlags::StandardType) ? pProperty->GetSpecificType()->GetVariantType() : ezVariantType::Uuid;

        const ezVariantArray& value = pAttrib->GetValue().Get<ezVariantArray>();
        ezVariantArray ret;
        ret.SetCount(value.GetCount());
        for (ezUInt32 i = 0; i < value.GetCount(); i++)
        {
          ret[i] = value[i].ConvertTo(elementType);
        }
        return ret;
      }
      return ezVariantArray();
    }
    break;
    case ezPropertyCategory::Map:
    {
      return ezVariantDictionary();
    }
    break;
    case ezPropertyCategory::Constant:
    case ezPropertyCategory::Function:
      break; // no defaults
  }
  return ezVariant();
}

bool ezToolsReflectionUtils::GetFloatFromVariant(const ezVariant& val, double& out_fValue)
{
  if (val.IsValid())
  {
    GetDoubleFunc func(val);
    ezVariant::DispatchTo(func, val.GetType());
    out_fValue = func.m_fValue;
    return func.m_bValid;
  }
  return false;
}


bool ezToolsReflectionUtils::GetVariantFromFloat(double fValue, ezVariantType::Enum type, ezVariant& out_val)
{
  GetVariantFunc func(fValue, type, out_val);
  ezVariant::DispatchTo(func, type);

  return func.m_bValid;
}

void ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(const ezRTTI* pRtti, ezReflectedTypeDescriptor& out_desc)
{
  GetMinimalReflectedTypeDescriptorFromRtti(pRtti, out_desc);
  out_desc.m_Flags.Remove(ezTypeFlags::Minimal);

  auto rttiProps = pRtti->GetProperties();
  const ezUInt32 uiCount = rttiProps.GetCount();
  out_desc.m_Properties.Reserve(uiCount);
  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    const ezAbstractProperty* prop = rttiProps[i];

    switch (prop->GetCategory())
    {
      case ezPropertyCategory::Constant:
      {
        auto constantProp = static_cast<const ezAbstractConstantProperty*>(prop);
        const ezRTTI* pPropRtti = constantProp->GetSpecificType();
        if (ezReflectionUtils::IsBasicType(pPropRtti))
        {
          ezVariant value = constantProp->GetConstant();
          EZ_ASSERT_DEV(pPropRtti->GetVariantType() == value.GetType(), "Variant value type and property type should always match!");
          out_desc.m_Properties.PushBack(ezReflectedPropertyDescriptor(constantProp->GetPropertyName(), value, prop->GetAttributes()));
        }
        else
        {
          EZ_ASSERT_DEV(false, "Non-pod constants are not supported yet!");
        }
      }
      break;

      case ezPropertyCategory::Member:
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      case ezPropertyCategory::Map:
      {
        const ezRTTI* pPropRtti = prop->GetSpecificType();
        out_desc.m_Properties.PushBack(ezReflectedPropertyDescriptor(prop->GetCategory(), prop->GetPropertyName(), pPropRtti->GetTypeName(), prop->GetFlags(), prop->GetAttributes()));
      }
      break;

      case ezPropertyCategory::Function:
        break;

      default:
        break;
    }
  }

  auto rttiFunc = pRtti->GetFunctions();
  const ezUInt32 uiFuncCount = rttiFunc.GetCount();
  out_desc.m_Functions.Reserve(uiFuncCount);

  for (ezUInt32 i = 0; i < uiFuncCount; ++i)
  {
    const ezAbstractFunctionProperty* prop = rttiFunc[i];
    out_desc.m_Functions.PushBack(ezReflectedFunctionDescriptor(prop->GetPropertyName(), prop->GetFlags(), prop->GetFunctionType(), prop->GetAttributes()));
    ezReflectedFunctionDescriptor& desc = out_desc.m_Functions.PeekBack();
    desc.m_ReturnValue = ezFunctionArgumentDescriptor(prop->GetReturnType() ? prop->GetReturnType()->GetTypeName() : "", prop->GetReturnFlags());
    const ezUInt32 uiArguments = prop->GetArgumentCount();
    desc.m_Arguments.Reserve(uiArguments);
    for (ezUInt32 a = 0; a < uiArguments; ++a)
    {
      desc.m_Arguments.PushBack(ezFunctionArgumentDescriptor(prop->GetArgumentType(a)->GetTypeName(), prop->GetArgumentFlags(a)));
    }
  }

  out_desc.m_ReferenceAttributes = pRtti->GetAttributes();
}


void ezToolsReflectionUtils::GetMinimalReflectedTypeDescriptorFromRtti(const ezRTTI* pRtti, ezReflectedTypeDescriptor& out_desc)
{
  EZ_ASSERT_DEV(pRtti != nullptr, "Type to process must not be null!");
  out_desc.m_sTypeName = pRtti->GetTypeName();
  out_desc.m_sPluginName = pRtti->GetPluginName();
  out_desc.m_Flags = pRtti->GetTypeFlags() | ezTypeFlags::Minimal;
  out_desc.m_uiTypeVersion = pRtti->GetTypeVersion();
  const ezRTTI* pParentRtti = pRtti->GetParentType();
  out_desc.m_sParentTypeName = pParentRtti ? pParentRtti->GetTypeName() : nullptr;

  out_desc.m_Properties.Clear();
  out_desc.m_Functions.Clear();
  out_desc.m_Attributes.Clear();
  out_desc.m_ReferenceAttributes = ezArrayPtr<ezPropertyAttribute* const>();
}

static void GatherObjectTypesInternal(const ezDocumentObject* pObject, ezSet<const ezRTTI*>& inout_types)
{
  inout_types.Insert(pObject->GetTypeAccessor().GetType());
  ezReflectionUtils::GatherDependentTypes(pObject->GetTypeAccessor().GetType(), inout_types);

  for (const ezDocumentObject* pChild : pObject->GetChildren())
  {
    if (pChild->GetParentPropertyType()->GetAttributeByType<ezTemporaryAttribute>() != nullptr)
      continue;

    GatherObjectTypesInternal(pChild, inout_types);
  }
}

void ezToolsReflectionUtils::GatherObjectTypes(const ezDocumentObject* pObject, ezSet<const ezRTTI*>& inout_types)
{
  GatherObjectTypesInternal(pObject, inout_types);
}

bool ezToolsReflectionUtils::DependencySortTypeDescriptorArray(ezDynamicArray<ezReflectedTypeDescriptor*>& ref_descriptors)
{
  ezMap<ezReflectedTypeDescriptor*, ezSet<ezString>> dependencies;

  ezSet<ezString> typesInArray;
  // Gather all types in array
  for (ezReflectedTypeDescriptor* desc : ref_descriptors)
  {
    typesInArray.Insert(desc->m_sTypeName);
  }

  // Find all direct dependencies to types in the array for each type.
  for (ezReflectedTypeDescriptor* desc : ref_descriptors)
  {
    auto it = dependencies.Insert(desc, ezSet<ezString>());

    if (typesInArray.Contains(desc->m_sParentTypeName))
    {
      it.Value().Insert(desc->m_sParentTypeName);
    }
    for (ezReflectedPropertyDescriptor& propDesc : desc->m_Properties)
    {
      if (typesInArray.Contains(propDesc.m_sType))
      {
        it.Value().Insert(propDesc.m_sType);
      }
    }
  }

  ezSet<ezString> accu;
  ezDynamicArray<ezReflectedTypeDescriptor*> sorted;
  sorted.Reserve(ref_descriptors.GetCount());
  // Build new sorted types array.
  while (!ref_descriptors.IsEmpty())
  {
    bool bDeadEnd = true;
    for (ezReflectedTypeDescriptor* desc : ref_descriptors)
    {
      // Are the types dependencies met?
      if (accu.ContainsSet(dependencies[desc]))
      {
        sorted.PushBack(desc);
        bDeadEnd = false;
        ref_descriptors.RemoveAndCopy(desc);
        accu.Insert(desc->m_sTypeName);
        break;
      }
    }

    if (bDeadEnd)
    {
      return false;
    }
  }

  ref_descriptors = sorted;
  return true;
}
