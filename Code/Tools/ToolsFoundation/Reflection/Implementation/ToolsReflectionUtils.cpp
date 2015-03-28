#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/ExtendedJSONWriter.h>
#include <Foundation/IO/ExtendedJSONReader.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>


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
  case ezVariant::Type::Vector2:
    return ezVariant(ezVec2(0.0f, 0.0f));
  case ezVariant::Type::Vector3:
    return ezVariant(ezVec3(0.0f, 0.0f, 0.0f));
  case ezVariant::Type::Vector4:
    return ezVariant(ezVec4(0.0f, 0.0f, 0.0f, 0.0f));
  case ezVariant::Type::Quaternion:
    return ezVariant(ezQuat(0.0f, 0.0f, 0.0f, 1.0f));
  case ezVariant::Type::Matrix3:
    return ezVariant(ezMat3::IdentityMatrix());
  case ezVariant::Type::Matrix4:
    return ezVariant(ezMat4::IdentityMatrix());
  case ezVariant::Type::String:
    return ezVariant("");
  case ezVariant::Type::Time:
    return ezVariant(ezTime());
  case ezVariant::Type::Uuid:
    return ezVariant(ezUuid());
  case ezVariant::Type::VariantArray:
    return ezVariant();
  case ezVariant::Type::VariantDictionary:
    return ezVariant();
  case ezVariant::Type::ReflectedPointer:
    return ezVariant();
  case ezVariant::Type::VoidPointer:
    return ezVariant();
  case ezVariant::Type::ENUM_COUNT:
    EZ_REPORT_FAILURE("Invalid case statement");
    return ezVariant();
  }
  return ezVariant();
}

void ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(const ezRTTI* pRtti, ezReflectedTypeDescriptor& out_desc)
{
  EZ_ASSERT_DEV(pRtti != nullptr, "Type to process must not be null!");
  out_desc.m_sTypeName = pRtti->GetTypeName();
  out_desc.m_sPluginName = pRtti->GetPluginName();
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
    case ezAbstractProperty::Constant:
      {
        ezAbstractConstantProperty* constantProp = static_cast<ezAbstractConstantProperty*>(prop);
        const ezRTTI* pMemberPropRtti = constantProp->GetPropertyType();
        ezVariant::Type::Enum memberType = pMemberPropRtti->GetVariantType();

        if (memberType >= ezVariant::Type::Bool && memberType <= ezVariant::Type::Uuid)
        {
          ezVariant value = ezReflectionUtils::GetConstantPropertyValue(constantProp);
          out_desc.m_Properties.PushBack(ezReflectedPropertyDescriptor(constantProp->GetPropertyName(), memberType, value));
        }
        else
        {
          EZ_ASSERT_DEV(false, "Non-pod constants are not supported yet!");
        }
      }
      break;
    case ezAbstractProperty::Member:
      {
        ezAbstractMemberProperty* memberProp = static_cast<ezAbstractMemberProperty*>(prop);
        const ezRTTI* pMemberPropRtti = memberProp->GetPropertyType();
        ezVariant::Type::Enum memberType = pMemberPropRtti->GetVariantType();

        ezBitflags<PropertyFlags> memberFlags;
        if (memberProp->IsReadOnly())
          memberFlags.Add(PropertyFlags::IsReadOnly);

        if (pMemberPropRtti->IsDerivedFrom<ezEnumBase>())
          memberFlags.Add(PropertyFlags::IsEnum);
        else if (pMemberPropRtti->IsDerivedFrom<ezBitflagsBase>())
          memberFlags.Add(PropertyFlags::IsBitflags);

        if (memberType == ezVariant::Type::Invalid || memberType == ezVariant::Type::ReflectedPointer)
        {
          out_desc.m_Properties.PushBack(ezReflectedPropertyDescriptor(memberProp->GetPropertyName(), pMemberPropRtti->GetTypeName(), memberFlags));
        }
        else if (memberType >= ezVariant::Type::Bool && memberType <= ezVariant::Type::Uuid)
        {
          memberFlags.Add(PropertyFlags::IsPOD);
          out_desc.m_Properties.PushBack(ezReflectedPropertyDescriptor(memberProp->GetPropertyName(), memberType, memberFlags));
        }
        else
        {
          EZ_ASSERT_DEV(false, "Arrays and pointers are not supported yet!");
        }
      }
      break;
    case ezAbstractProperty::Function:
    case ezAbstractProperty::Array:
      break;
    }
  }
}

void ezToolsReflectionUtils::RegisterType(const ezRTTI* pRtti, bool bIncludeDerived)
{
  EZ_ASSERT_DEV(pRtti != nullptr, "Invalid type !!");

  ezSet<const ezRTTI*> types;
  types.Insert(pRtti);
  if (bIncludeDerived)
  {
    ezReflectionUtils::GatherTypesDerivedFromClass(pRtti, types, true);
  }
  else
  {
    ezReflectionUtils::GatherDependentTypes(pRtti, types);
  }

  ezDynamicArray<const ezRTTI*> sortedTypes;
  ezReflectionUtils::CreateDependencySortedTypeArray(types, sortedTypes);

  for (auto type : sortedTypes)
  {
    ezReflectedTypeDescriptor desc;
    ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(type, desc);
    ezReflectedTypeManager::RegisterType(desc);
  }
}

ezPropertyPath ezToolsReflectionUtils::CreatePropertyPath(const char* pData1, const char* pData2, const char* pData3, const char* pData4, const char* pData5, const char* pData6)
{
  const ezUInt32 uiMaxParams = 6;
  const char* pStrings[uiMaxParams] = { pData1, pData2, pData3, pData4, pData5, pData6 };
  ezUInt32 uiUsedParams = 0;
  for (ezUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (ezStringUtils::IsNullOrEmpty(pStrings[i]))
      break;

    uiUsedParams++;
  }

  return ezPropertyPath(ezArrayPtr<const char*>(pStrings, uiUsedParams));
}

ezAbstractMemberProperty* ezToolsReflectionUtils::GetMemberPropertyByPath(const ezRTTI*& inout_pRtti, void*& inout_pData, const ezPropertyPath& path)
{
  EZ_ASSERT_DEV(path.GetCount() > 0, "ezReflectedTypeDirectAccessor: the given property path is empty!");

  ezAbstractMemberProperty* pCurrentProp = ezReflectionUtils::GetMemberProperty(inout_pRtti, path[0]);

  // Find pointer that contains the final data property.
  const ezUInt32 uiPathSize = path.GetCount();
  for (ezUInt32 i = 1; i < uiPathSize; ++i)
  {
    if (pCurrentProp == nullptr)
      return nullptr;

    inout_pRtti = pCurrentProp->GetPropertyType();
    inout_pData = pCurrentProp->GetPropertyPointer(inout_pData);
    if (inout_pData == nullptr)
      return nullptr;
    
    pCurrentProp = ezReflectionUtils::GetMemberProperty(inout_pRtti, path[i]);
  }

  // pCurrentRtti should now be a type that contains the property that we can access directly.
  // pData should point to the object of type pCurrentRtti.
  // pCurrentProp should be a property defined in 'pCurrentRtti' and it should be a type that is handled by ezVariant.
  return pCurrentProp;
}

void ezToolsReflectionUtils::GetPropertyPathFromString(const char* szPath, ezPropertyPath& out_Path, ezHybridArray<ezString, 6>& out_Storage)
{
  ezStringBuilder temp = szPath;
  temp.Split(false, out_Storage, "/");

  const ezUInt32 uiCount = out_Storage.GetCount();

  for (ezUInt32 i = 0; i < uiCount; i++)
  {
    out_Path.PushBack(out_Storage[i]);
  }
}

ezString ezToolsReflectionUtils::GetStringFromPropertyPath(const ezPropertyPath& Path)
{
  EZ_ASSERT_DEV(Path.GetCount() > 0, "Path must not be empty");

  ezStringBuilder pathBuilder(Path[0]);

  const ezUInt32 pathLength = Path.GetCount();
  for (ezUInt32 i = 1; i < pathLength; ++i)
  {
    pathBuilder.Append("/", Path[i]);
  }

  return pathBuilder;
}



#define IF_HANDLE_TYPE(VAR_TYPE, TYPE, FUNC) \
  if (pProp->m_Type == VAR_TYPE) \
  { \
    ParentPath.PushBack(pProp->m_sPropertyName); \
    writer.BeginObject(); \
    writer.AddVariableString("t", #TYPE); \
    writer.AddVariableString("n", pProp->m_sPropertyName); \
    writer.FUNC("v", et.GetValue(ParentPath).ConvertTo<TYPE>()); \
    writer.EndObject(); \
    ParentPath.PopBack(); \
  } 

#define IF_HANDLE_TYPE_STRING(VAR_TYPE, TYPE, FUNC) \
  if (pProp->m_Type == VAR_TYPE) \
  { \
    ParentPath.PushBack(pProp->m_sPropertyName); \
    writer.BeginObject(); \
    writer.AddVariableString("t", #TYPE); \
    writer.AddVariableString("n", pProp->m_sPropertyName); \
    writer.FUNC("v", et.GetValue(ParentPath).ConvertTo<ezString>()); \
    writer.EndObject(); \
    ParentPath.PopBack(); \
  } 

static void WriteJSONObject(ezJSONWriter& writer, const ezIReflectedTypeAccessor& et, const ezReflectedType* pType, ezPropertyPath& ParentPath, const char* szObjectName);

static void WriteProperties(ezJSONWriter& writer, const ezIReflectedTypeAccessor& et, const ezReflectedType* pType, ezPropertyPath& ParentPath)
{
  if (!pType->GetParentTypeHandle().IsInvalidated())
  {
    WriteProperties(writer, et, pType->GetParentTypeHandle().GetType(), ParentPath);
  }

  for (ezUInt32 p = 0; p < pType->GetPropertyCount(); ++p)
  {
    const ezReflectedProperty* pProp = pType->GetPropertyByIndex(p);

    if (pProp->m_Flags.IsAnySet(PropertyFlags::IsReadOnly))
      continue;

    if (pProp->m_Flags.IsAnySet(PropertyFlags::IsPOD))
    {

      IF_HANDLE_TYPE(ezVariant::Type::Bool, bool, AddVariableBool)

      else IF_HANDLE_TYPE(ezVariant::Type::Int8, ezInt8, AddVariableInt32)
      else IF_HANDLE_TYPE(ezVariant::Type::Int16, ezInt16, AddVariableInt32)
      else IF_HANDLE_TYPE(ezVariant::Type::Int32, ezInt32, AddVariableInt32)
      else IF_HANDLE_TYPE(ezVariant::Type::Int64, ezInt64, AddVariableInt64)

      else IF_HANDLE_TYPE(ezVariant::Type::UInt8, ezUInt8, AddVariableUInt32)
      else IF_HANDLE_TYPE(ezVariant::Type::UInt16, ezUInt16, AddVariableUInt32)
      else IF_HANDLE_TYPE(ezVariant::Type::UInt32, ezUInt32, AddVariableUInt32)
      else IF_HANDLE_TYPE(ezVariant::Type::UInt64, ezUInt64, AddVariableUInt64)

      else IF_HANDLE_TYPE(ezVariant::Type::Float, float, AddVariableFloat)
      else IF_HANDLE_TYPE(ezVariant::Type::Double, double, AddVariableDouble)

      else IF_HANDLE_TYPE(ezVariant::Type::Matrix3, ezMat3, AddVariableMat3)
      else IF_HANDLE_TYPE(ezVariant::Type::Matrix4, ezMat4, AddVariableMat4)
      else IF_HANDLE_TYPE(ezVariant::Type::Quaternion, ezQuat, AddVariableQuat)

      else IF_HANDLE_TYPE(ezVariant::Type::Vector2, ezVec2, AddVariableVec2)
      else IF_HANDLE_TYPE(ezVariant::Type::Vector3, ezVec3, AddVariableVec3)
      else IF_HANDLE_TYPE(ezVariant::Type::Vector4, ezVec4, AddVariableVec4)

      else IF_HANDLE_TYPE(ezVariant::Type::Color, ezColor, AddVariableColor)
      else IF_HANDLE_TYPE(ezVariant::Type::Time, ezTime, AddVariableTime)
      else IF_HANDLE_TYPE(ezVariant::Type::Uuid, ezUuid, AddVariableUuid)
      else IF_HANDLE_TYPE_STRING(ezVariant::Type::String, ezConstCharPtr, AddVariableString)
    }
    else if (pProp->m_Flags.IsAnySet(PropertyFlags::IsEnum | PropertyFlags::IsBitflags))
    {
      ParentPath.PushBack(pProp->m_sPropertyName);
      writer.BeginObject();
      writer.AddVariableString("t", pProp->m_Flags.IsSet(PropertyFlags::IsEnum) ? "ezEnum" : "ezBitflags");
      writer.AddVariableString("n", pProp->m_sPropertyName);
      writer.AddVariableInt64("v", et.GetValue(ParentPath).ConvertTo<ezInt64>());
      writer.EndObject();
      ParentPath.PopBack();
    }
    else
    {
      writer.BeginObject();
      writer.AddVariableString("t", "$s"); // struct property
      writer.AddVariableString("n", pProp->m_sPropertyName);

      ParentPath.PushBack(pProp->m_sPropertyName);
      WriteJSONObject(writer, et, pProp->m_hTypeHandle.GetType(), ParentPath, "v");
      ParentPath.PopBack();

      writer.EndObject();
    }
  }
}

static void WriteJSONObject(ezJSONWriter& writer, const ezIReflectedTypeAccessor& et, const ezReflectedType* pType, ezPropertyPath& ParentPath, const char* szObjectName)
{
  writer.BeginObject(szObjectName);

  writer.AddVariableString("t", et.GetReflectedTypeHandle().GetType()->GetTypeName().GetData());

  writer.BeginArray("p");

  WriteProperties(writer, et, pType, ParentPath);

  writer.EndArray();

  writer.EndObject();
}


void ezToolsReflectionUtils::WriteObjectToJSON(ezStreamWriterBase& stream, const ezIReflectedTypeAccessor& accessor, ezJSONWriter::WhitespaceMode::Enum WhitespaceMode)
{
  ezExtendedJSONWriter writer;
  writer.SetOutputStream(&stream);
  writer.SetWhitespaceMode(WhitespaceMode);

  ezPropertyPath path;
  WriteJSONObject(writer, accessor, accessor.GetReflectedTypeHandle().GetType(), path, nullptr);
}

static void ReadJSONObject(const ezVariantDictionary& root, ezIReflectedTypeAccessor& et, ezPropertyPath& ParentPath)
{
  ezVariant* pVal;

  if (!root.TryGetValue("p", pVal) || !pVal->IsA<ezVariantArray>())
    return;

  ezVariantArray va = pVal->ConvertTo<ezVariantArray>();
  
  for (ezUInt32 prop = 0; prop < va.GetCount(); ++prop)
  {
    if (va[prop].GetType() != ezVariant::Type::VariantDictionary)
      continue;

    const ezVariantDictionary obj = va[prop].ConvertTo<ezVariantDictionary>();

    ezVariant* pName;
    if (!obj.TryGetValue("n", pName))
      continue;

    ezVariant* pType;
    if (!obj.TryGetValue("t", pType))
      continue;

    ezVariant* pValue;
    if (!obj.TryGetValue("v", pValue))
      continue;

    ezString sName = pName->ConvertTo<ezString>();

    ParentPath.PushBack(sName);

    const ezString sType = pType->ConvertTo<ezString>();

    if (sType != "$s") // not a struct property
    {
      et.SetValue(ParentPath, *pValue);
    }
    else
    {
      if (!pValue->IsA<ezVariantDictionary>())
      {
        ParentPath.PopBack();
        continue;
      }

      ReadJSONObject(pValue->ConvertTo<ezVariantDictionary>(), et, ParentPath);
    }

    ParentPath.PopBack();
  }
}

void ezToolsReflectionUtils::ReadObjectPropertiesFromJSON(ezStreamReaderBase& stream, ezIReflectedTypeAccessor& accessor)
{
  ezExtendedJSONReader reader;
  if (reader.Parse(stream).Failed())
    return;

  const ezVariantDictionary& root = reader.GetTopLevelObject();

  ezPropertyPath path;
  ReadJSONObject(root, accessor, path);
}

bool ezToolsReflectionUtils::EnumerationToString(const ezReflectedType* pEnumerationRtti, ezInt64 iValue, ezStringBuilder& out_sOutput)
{
  out_sOutput.Clear();
  const ezReflectedType* pParentRtti = pEnumerationRtti->GetParentTypeHandle().GetType();
  EZ_ASSERT_DEV(pParentRtti != nullptr, "Parent type does not exist!");
  
  // TODO: make this faster.
  if (ezStringUtils::IsEqual(pParentRtti->GetTypeName().GetData(), "ezEnumBase"))
  {
    for (const ezReflectedConstant& constant : pEnumerationRtti->GetConstants())
    {
      if (constant.m_ConstantValue.ConvertTo<ezInt64>() == iValue)
      {
        out_sOutput = constant.m_sPropertyName.GetString();
        return true;
      }
    }
    return false;
  }
  else if (ezStringUtils::IsEqual(pParentRtti->GetTypeName().GetData(), "ezBitflagsBase"))
  {
    for (const ezReflectedConstant& constant : pEnumerationRtti->GetConstants())
    {
      if ((constant.m_ConstantValue.ConvertTo<ezInt64>() & iValue) != 0)
      {
        out_sOutput.Append(constant.m_sPropertyName.GetString(), "|");
      }
    }
    out_sOutput.Shrink(0, 1);
    return true;
  }
  else
  {
    EZ_ASSERT_DEV(false, "The RTTI class '%s' is not an enum or bitflags class", pEnumerationRtti->GetTypeName().GetData());
    return false;
  }
}

bool ezToolsReflectionUtils::StringToEnumeration(const ezReflectedType* pEnumerationRtti, const char* szValue, ezInt64& out_iValue)
{
  out_iValue = 0;
  const ezReflectedType* pParentRtti = pEnumerationRtti->GetParentTypeHandle().GetType();
  EZ_ASSERT_DEV(pParentRtti != nullptr, "Parent type does not exist!");
  
  // TODO: make this faster.
  if (ezStringUtils::IsEqual(pParentRtti->GetTypeName().GetData(), "ezEnumBase"))
  {
    for (const ezReflectedConstant& constant : pEnumerationRtti->GetConstants())
    {
      if (ezStringUtils::IsEqual(constant.m_sPropertyName, szValue))
      {
        out_iValue = constant.m_ConstantValue.ConvertTo<ezInt64>();
        return true;
      }
    }
    return false;
  }
  else if (ezStringUtils::IsEqual(pParentRtti->GetTypeName().GetData(), "ezBitflagsBase"))
  {
    ezStringBuilder temp = szValue;
    ezHybridArray<ezStringView, 32> values;
    temp.Split(false, values, "|");
    for (auto sValue : values)
    {
      for (const ezReflectedConstant& constant : pEnumerationRtti->GetConstants())
      {
        if (sValue.IsEqual(constant.m_sPropertyName))
        {
          out_iValue |= constant.m_ConstantValue.ConvertTo<ezInt64>();
        }
      }
    }
    return true;
  }
  else
  {
    EZ_ASSERT_DEV(false, "The RTTI class '%s' is not an enum or bitflags class", pEnumerationRtti->GetTypeName().GetData());
    return false;
  }
}

