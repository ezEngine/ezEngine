#include <PCH.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

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
  case ezVariant::Type::Quaternion:
    return ezVariant(ezQuat(0.0f, 0.0f, 0.0f, 1.0f));
  case ezVariant::Type::Matrix3:
    return ezVariant(ezMat3::IdentityMatrix());
  case ezVariant::Type::Matrix4:
    return ezVariant(ezMat4::IdentityMatrix());
  case ezVariant::Type::String:
    return ezVariant("");
  case ezVariant::Type::StringView:
    return ezVariant("");
  case ezVariant::Type::Time:
    return ezVariant(ezTime());
  case ezVariant::Type::Uuid:
    return ezVariant(ezUuid());
  case ezVariant::Type::Angle:
    return ezVariant(ezAngle());
  case ezVariant::Type::DataBuffer:
    return ezVariant(ezDataBuffer());
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

ezVariant ezToolsReflectionUtils::GetDefaultValue(const ezAbstractProperty* pProperty)
{
  const ezDefaultValueAttribute* pAttrib = pProperty->GetAttributeByType<ezDefaultValueAttribute>();
  auto type = pProperty->GetFlags().IsSet(ezPropertyFlags::StandardType) ? pProperty->GetSpecificType()->GetVariantType() : ezVariantType::Uuid;
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
  else if (pProperty->GetFlags().IsAnySet(ezPropertyFlags::Pointer | ezPropertyFlags::EmbeddedClass))
  {
    return ezUuid();
  }

  return ezVariant();
}

void ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(const ezRTTI* pRtti, ezReflectedTypeDescriptor& out_desc)
{
  EZ_ASSERT_DEV(pRtti != nullptr, "Type to process must not be null!");
  out_desc.m_sTypeName = pRtti->GetTypeName();
  out_desc.m_sPluginName = pRtti->GetPluginName();
  out_desc.m_Flags = pRtti->GetTypeFlags();
  out_desc.m_uiTypeSize = pRtti->GetTypeSize();
  out_desc.m_uiTypeVersion = pRtti->GetTypeVersion();
  const ezRTTI* pParentRtti = pRtti->GetParentType();
  out_desc.m_sParentTypeName = pParentRtti ? pParentRtti->GetTypeName() : nullptr;

  out_desc.m_Properties.Clear();
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

  out_desc.m_ReferenceAttributes = pRtti->GetAttributes();
}

static void GatherObjectTypesInternal(const ezDocumentObject* pObject, ezSet<const ezRTTI*>& inout_types)
{
  inout_types.Insert(pObject->GetTypeAccessor().GetType());
  ezReflectionUtils::GatherDependentTypes(pObject->GetTypeAccessor().GetType(), inout_types);

  for (const ezDocumentObject* pChild : pObject->GetChildren())
  {
    GatherObjectTypesInternal(pChild, inout_types);
  }
}

void ezToolsReflectionUtils::GatherObjectTypes(const ezDocumentObject* pObject, ezSet<const ezRTTI*>& inout_types, bool bOnlyPhantomTypes)
{
  if (!bOnlyPhantomTypes)
  {
    GatherObjectTypesInternal(pObject, inout_types);
    return;
  }

  ezSet<const ezRTTI*> types;
  GatherObjectTypesInternal(pObject, types);

  for (const ezRTTI* pType : types)
  {
    if (pType->GetTypeFlags().IsSet(ezTypeFlags::Phantom))
    {
      inout_types.Insert(pType);
    }
  }
}

bool ezToolsReflectionUtils::DependencySortTypeDescriptorArray(ezDynamicArray<ezReflectedTypeDescriptor*>& descriptors)
{
  ezMap<ezReflectedTypeDescriptor*, ezSet<ezString> > dependencies;

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
      if (accu.Contains(dependencies[desc]))
      {
        sorted.PushBack(desc);
        bDeadEnd = false;
        descriptors.Remove(desc);
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


