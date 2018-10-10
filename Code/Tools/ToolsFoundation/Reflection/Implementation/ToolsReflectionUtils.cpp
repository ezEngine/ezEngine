#include <PCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
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
    m_Value = ezAngle::Degree((float)m_fValue);
    m_bValid = true;
  }

  template <>
  void GetVariantFunc::operator()<ezTime>()
  {
    m_Value = ezTime::Seconds(m_fValue);
    m_bValid = true;
  }
}
////////////////////////////////////////////////////////////////////////
// ezToolsReflectionUtils public functions
////////////////////////////////////////////////////////////////////////

ezVariant ezToolsReflectionUtils::GetStorageDefault(const ezAbstractProperty* pProperty)
{
  const ezDefaultValueAttribute* pAttrib = pProperty->GetAttributeByType<ezDefaultValueAttribute>();
  auto type =
      pProperty->GetFlags().IsSet(ezPropertyFlags::StandardType) ? pProperty->GetSpecificType()->GetVariantType() : ezVariantType::Uuid;

  switch (pProperty->GetCategory())
  {
    case ezPropertyCategory::Member:
    {
      return ezReflectionUtils::GetDefaultValue(pProperty);
    }
    break;
    case ezPropertyCategory::Array:
    case ezPropertyCategory::Set:
    {
      if (pProperty->GetSpecificType()->GetTypeFlags().IsSet(ezTypeFlags::StandardType) && pAttrib &&
          pAttrib->GetValue().IsA<ezVariantArray>())
      {
        const ezVariantArray& value = pAttrib->GetValue().Get<ezVariantArray>();
        ezVariantArray ret;
        ret.SetCount(value.GetCount());
        for (ezUInt32 i = 0; i < value.GetCount(); i++)
        {
          ret[i] = value[i].ConvertTo(type);
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

  const ezArrayPtr<ezAbstractProperty*>& rttiProps = pRtti->GetProperties();
  const ezUInt32 uiCount = rttiProps.GetCount();
  out_desc.m_Properties.Reserve(uiCount);
  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    ezAbstractProperty* prop = rttiProps[i];

    switch (prop->GetCategory())
    {
      case ezPropertyCategory::Constant:
      {
        ezAbstractConstantProperty* constantProp = static_cast<ezAbstractConstantProperty*>(prop);
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
        out_desc.m_Properties.PushBack(ezReflectedPropertyDescriptor(prop->GetCategory(), prop->GetPropertyName(), pPropRtti->GetTypeName(),
                                                                     prop->GetFlags(), prop->GetAttributes()));
      }
      break;

      case ezPropertyCategory::Function:
        break;

      default:
        break;
    }
  }

  const ezArrayPtr<ezAbstractFunctionProperty*>& rttiFunc = pRtti->GetFunctions();
  const ezUInt32 uiFuncCount = rttiFunc.GetCount();
  out_desc.m_Functions.Reserve(uiFuncCount);

  for (ezUInt32 i = 0; i < uiFuncCount; ++i)
  {
    ezAbstractFunctionProperty* prop = rttiFunc[i];
    out_desc.m_Functions.PushBack(
        ezReflectedFunctionDescriptor(prop->GetPropertyName(), prop->GetFlags(), prop->GetFunctionType(), prop->GetAttributes()));
    ezReflectedFunctionDescriptor& desc = out_desc.m_Functions.PeekBack();
    desc.m_ReturnValue =
        ezFunctionArgumentDescriptor(prop->GetReturnType() ? prop->GetReturnType()->GetTypeName() : "", prop->GetReturnFlags());
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
  out_desc.m_uiTypeSize = pRtti->GetTypeSize();
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

void ezToolsReflectionUtils::SerializeTypes(const ezSet<const ezRTTI*>& types, ezAbstractObjectGraph& typesGraph)
{
  ezRttiConverterContext context;
  ezRttiConverterWriter rttiConverter(&typesGraph, &context, true, true);
  for (const ezRTTI* pType : types)
  {
    ezReflectedTypeDescriptor desc;
    if (pType->GetTypeFlags().IsSet(ezTypeFlags::Phantom))
    {
      ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pType, desc);
    }
    else
    {
      ezToolsReflectionUtils::GetMinimalReflectedTypeDescriptorFromRtti(pType, desc);
    }

    context.RegisterObject(ezUuid::StableUuidForString(pType->GetTypeName()), ezGetStaticRTTI<ezReflectedTypeDescriptor>(), &desc);
    rttiConverter.AddObjectToGraph(ezGetStaticRTTI<ezReflectedTypeDescriptor>(), &desc);
  }
}

bool ezToolsReflectionUtils::DependencySortTypeDescriptorArray(ezDynamicArray<ezReflectedTypeDescriptor*>& descriptors)
{
  ezMap<ezReflectedTypeDescriptor*, ezSet<ezString>> dependencies;

  ezSet<ezString> typesInArray;
  // Gather all types in array
  for (ezReflectedTypeDescriptor* desc : descriptors)
  {
    typesInArray.Insert(desc->m_sTypeName);
  }

  // Find all direct dependencies to types in the array for each type.
  for (ezReflectedTypeDescriptor* desc : descriptors)
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
  sorted.Reserve(descriptors.GetCount());
  // Build new sorted types array.
  while (!descriptors.IsEmpty())
  {
    bool bDeadEnd = true;
    for (ezReflectedTypeDescriptor* desc : descriptors)
    {
      // Are the types dependencies met?
      if (accu.ContainsSet(dependencies[desc]))
      {
        sorted.PushBack(desc);
        bDeadEnd = false;
        descriptors.RemoveAndCopy(desc);
        accu.Insert(desc->m_sTypeName);
        break;
      }
    }

    if (bDeadEnd)
    {
      return false;
    }
  }

  descriptors = sorted;
  return true;
}

