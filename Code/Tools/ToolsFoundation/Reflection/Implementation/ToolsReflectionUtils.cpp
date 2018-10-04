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

ezVariant ezToolsReflectionUtils::GetDefaultVariantFromType(ezVariant::Type::Enum type)
{
  switch (type)
  {
    case ezVariant::Type::Invalid:
      return ezVariant();
    case ezVariant::Type::Bool:
      return ezVariant(false);
    case ezVariant::Type::Int8:
      return ezVariant((ezInt8)0);
    case ezVariant::Type::UInt8:
      return ezVariant((ezUInt8)0);
    case ezVariant::Type::Int16:
      return ezVariant((ezInt16)0);
    case ezVariant::Type::UInt16:
      return ezVariant((ezUInt16)0);
    case ezVariant::Type::Int32:
      return ezVariant((ezInt32)0);
    case ezVariant::Type::UInt32:
      return ezVariant((ezUInt32)0);
    case ezVariant::Type::Int64:
      return ezVariant((ezInt64)0);
    case ezVariant::Type::UInt64:
      return ezVariant((ezUInt64)0);
    case ezVariant::Type::Float:
      return ezVariant(0.0f);
    case ezVariant::Type::Double:
      return ezVariant(0.0);
    case ezVariant::Type::Color:
      return ezVariant(ezColor(1.0f, 1.0f, 1.0f));
    case ezVariant::Type::ColorGamma:
      return ezVariant(ezColorGammaUB(255, 255, 255));
    case ezVariant::Type::Vector2:
      return ezVariant(ezVec2(0.0f, 0.0f));
    case ezVariant::Type::Vector3:
      return ezVariant(ezVec3(0.0f, 0.0f, 0.0f));
    case ezVariant::Type::Vector4:
      return ezVariant(ezVec4(0.0f, 0.0f, 0.0f, 0.0f));
    case ezVariant::Type::Vector2I:
      return ezVariant(ezVec2I32(0, 0));
    case ezVariant::Type::Vector3I:
      return ezVariant(ezVec3I32(0, 0, 0));
    case ezVariant::Type::Vector4I:
      return ezVariant(ezVec4I32(0, 0, 0, 0));
    case ezVariant::Type::Vector2U:
      return ezVariant(ezVec2U32(0, 0));
    case ezVariant::Type::Vector3U:
      return ezVariant(ezVec3U32(0, 0, 0));
    case ezVariant::Type::Vector4U:
      return ezVariant(ezVec4U32(0, 0, 0, 0));
    case ezVariant::Type::Quaternion:
      return ezVariant(ezQuat(0.0f, 0.0f, 0.0f, 1.0f));
    case ezVariant::Type::Matrix3:
      return ezVariant(ezMat3::IdentityMatrix());
    case ezVariant::Type::Matrix4:
      return ezVariant(ezMat4::IdentityMatrix());
    case ezVariant::Type::Transform:
      return ezVariant(ezTransform::IdentityTransform());
    case ezVariant::Type::String:
      return ezVariant("");
    case ezVariant::Type::StringView:
      return ezVariant("");
    case ezVariant::Type::DataBuffer:
      return ezVariant(ezDataBuffer());
    case ezVariant::Type::Time:
      return ezVariant(ezTime());
    case ezVariant::Type::Uuid:
      return ezVariant(ezUuid());
    case ezVariant::Type::Angle:
      return ezVariant(ezAngle());
    case ezVariant::Type::VariantArray:
      return ezVariantArray();
    case ezVariant::Type::VariantDictionary:
      return ezVariantDictionary();
    case ezVariant::Type::ReflectedPointer:
      return ezVariant();
    case ezVariant::Type::VoidPointer:
      return ezVariant();

    default:
      EZ_REPORT_FAILURE("Invalid case statement");
      return ezVariant();
  }
  return ezVariant();
}

ezVariant ezToolsReflectionUtils::GetStorageDefault(const ezAbstractProperty* pProperty)
{
  const ezDefaultValueAttribute* pAttrib = pProperty->GetAttributeByType<ezDefaultValueAttribute>();
  auto type =
      pProperty->GetFlags().IsSet(ezPropertyFlags::StandardType) ? pProperty->GetSpecificType()->GetVariantType() : ezVariantType::Uuid;

  switch (pProperty->GetCategory())
  {
    case ezPropertyCategory::Member:
    {
      return ezToolsReflectionUtils::GetDefaultValue(pProperty);
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


ezVariant ezToolsReflectionUtils::GetDefaultValue(const ezAbstractProperty* pProperty)
{
  const ezDefaultValueAttribute* pAttrib = pProperty->GetAttributeByType<ezDefaultValueAttribute>();
  auto type =
      pProperty->GetFlags().IsSet(ezPropertyFlags::StandardType) ? pProperty->GetSpecificType()->GetVariantType() : ezVariantType::Uuid;
  if (pProperty->GetSpecificType()->GetTypeFlags().IsSet(ezTypeFlags::StandardType))
  {
    if (pAttrib)
    {
      if (pAttrib->GetValue().CanConvertTo(type))
      {
        return pAttrib->GetValue().ConvertTo(type);
      }
    }
    return GetDefaultVariantFromType(type);
  }
  else if (pProperty->GetSpecificType()->GetTypeFlags().IsAnySet(ezTypeFlags::IsEnum | ezTypeFlags::Bitflags))
  {
    ezInt64 iValue = ezReflectionUtils::DefaultEnumerationValue(pProperty->GetSpecificType());
    if (pAttrib)
    {
      if (pAttrib->GetValue().CanConvertTo(ezVariantType::Int64))
        iValue = pAttrib->GetValue().ConvertTo<ezInt64>();
    }

    return ezReflectionUtils::MakeEnumerationValid(pProperty->GetSpecificType(), iValue);
  }
  else if (pProperty->GetFlags().IsAnySet(ezPropertyFlags::Class))
  {
    return ezUuid();
  }
  EZ_REPORT_FAILURE("Not reachable.");
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
