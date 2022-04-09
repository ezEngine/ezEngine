#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Foundation/IO/StringDeduplicationContext.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>
#include <VisualScriptPlugin/Runtime/VisualScriptNodeFunctions.h>
#include <VisualScriptPlugin/Runtime/VisualScriptNodeUserData.h>

namespace
{
  static const char* s_NodeDescTypeNames[] = {
    "", // Invalid,
    "EntryCall",
    "MessageHandler",
    "ReflectedFunction",
    "GetScriptOwner",

    "", // FirstBuiltin,

    "Builtin_Branch",
    "Builtin_And",
    "Builtin_Or",
    "Builtin_Not",
    "Builtin_Compare",
    "Builtin_IsValid",

    "Builtin_Add",
    "Builtin_Subtract",
    "Builtin_Multiply",
    "Builtin_Divide",

    "Builtin_ToBool",
    "Builtin_ToByte",
    "Builtin_ToInt",
    "Builtin_ToInt64",
    "Builtin_ToFloat",
    "Builtin_ToDouble",
    "Builtin_ToString",
    "Builtin_ToVariant",
    "Builtin_Variant_ConvertTo",

    "Builtin_MakeArray",

    "Builtin_TryGetComponentOfBaseType",

    "", // LastBuiltin,
  };
  static_assert(EZ_ARRAY_SIZE(s_NodeDescTypeNames) == (size_t)ezVisualScriptNodeDescription::Type::Count);

  template <typename T>
  ezResult WriteNodeArray(ezArrayPtr<T> a, ezStreamWriter& inout_stream)
  {
    ezUInt16 uiCount = static_cast<ezUInt16>(a.GetCount());
    inout_stream << uiCount;

    return inout_stream.WriteBytes(a.GetPtr(), a.GetCount() * sizeof(T));
  }

} // namespace

// static
ezVisualScriptNodeDescription::Type::Enum ezVisualScriptNodeDescription::Type::GetConversionType(ezVisualScriptDataType::Enum targetDataType)
{
  static_assert(Builtin_ToBool + (ezVisualScriptDataType::Bool - ezVisualScriptDataType::Bool) == Builtin_ToBool);
  static_assert(Builtin_ToBool + (ezVisualScriptDataType::Byte - ezVisualScriptDataType::Bool) == Builtin_ToByte);
  static_assert(Builtin_ToBool + (ezVisualScriptDataType::Int - ezVisualScriptDataType::Bool) == Builtin_ToInt);
  static_assert(Builtin_ToBool + (ezVisualScriptDataType::Int64 - ezVisualScriptDataType::Bool) == Builtin_ToInt64);
  static_assert(Builtin_ToBool + (ezVisualScriptDataType::Float - ezVisualScriptDataType::Bool) == Builtin_ToFloat);
  static_assert(Builtin_ToBool + (ezVisualScriptDataType::Double - ezVisualScriptDataType::Bool) == Builtin_ToDouble);

  if (ezVisualScriptDataType::IsNumber(targetDataType))
    return static_cast<Enum>(Builtin_ToBool + (targetDataType - ezVisualScriptDataType::Bool));

  if (targetDataType == ezVisualScriptDataType::String)
    return Builtin_ToString;

  if (targetDataType == ezVisualScriptDataType::Variant)
    return Builtin_ToVariant;

  EZ_ASSERT_NOT_IMPLEMENTED;
  return Invalid;
}

// static
const char* ezVisualScriptNodeDescription::Type::GetName(Enum type)
{
  EZ_ASSERT_DEBUG(type >= 0 && type < EZ_ARRAY_SIZE(s_NodeDescTypeNames), "Out of bounds access");
  return s_NodeDescTypeNames[type];
}

void ezVisualScriptNodeDescription::AppendUserDataName(ezStringBuilder& out_sResult) const
{
  if (auto func = GetUserDataContext(m_Type).m_ToStringFunc)
  {
    out_sResult.Append(" ");

    func(*this, out_sResult);
  }
}

//////////////////////////////////////////////////////////////////////////

ezVisualScriptGraphDescription::ezVisualScriptGraphDescription()
{
  static_assert(sizeof(Node) == 64);
}

ezVisualScriptGraphDescription::~ezVisualScriptGraphDescription() = default;

static const ezTypeVersion s_uiVisualScriptGraphDescriptionVersion = 1;

// static
ezResult ezVisualScriptGraphDescription::Serialize(ezArrayPtr<const ezVisualScriptNodeDescription> nodes, ezStreamWriter& inout_stream)
{
  inout_stream.WriteVersion(s_uiVisualScriptGraphDescriptionVersion);

  ezDefaultMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter stream(&streamStorage);
  ezUInt32 additionalDataSize = 0;
  {
    for (auto& nodeDesc : nodes)
    {
      stream << nodeDesc.m_Type;
      stream << nodeDesc.m_DeductedDataType;
      EZ_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_ExecutionIndices.GetArrayPtr(), stream));
      EZ_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_InputDataOffsets.GetArrayPtr(), stream));
      EZ_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_OutputDataOffsets.GetArrayPtr(), stream));

      ExecutionIndicesArray::AddAdditionalDataSize(nodeDesc.m_ExecutionIndices, additionalDataSize);
      InputDataOffsetsArray::AddAdditionalDataSize(nodeDesc.m_InputDataOffsets, additionalDataSize);
      OutputDataOffsetsArray::AddAdditionalDataSize(nodeDesc.m_OutputDataOffsets, additionalDataSize);

      if (auto func = GetUserDataContext(nodeDesc.m_Type).m_SerializeFunc)
      {
        ezUInt32 uiSize = 0;
        ezUInt32 uiAlignment = 0;
        EZ_SUCCEED_OR_RETURN(func(nodeDesc, stream, uiSize, uiAlignment));

        UserDataArray::AddAdditionalDataSize(uiSize, uiAlignment, additionalDataSize);
      }
    }
  }

  const ezUInt32 uiRequiredStorageSize = nodes.GetCount() * sizeof(Node) + additionalDataSize;
  inout_stream << uiRequiredStorageSize;
  inout_stream << nodes.GetCount();

  return streamStorage.CopyToStream(inout_stream);
}

ezResult ezVisualScriptGraphDescription::Deserialize(ezStreamReader& inout_stream)
{
  ezTypeVersion uiVersion = inout_stream.ReadVersion(s_uiVisualScriptGraphDescriptionVersion);

  {
    ezUInt32 uiStorageSize;
    inout_stream >> uiStorageSize;

    m_Storage.SetCountUninitialized(uiStorageSize);
    m_Storage.ZeroFill();
  }

  ezUInt32 uiNumNodes;
  inout_stream >> uiNumNodes;

  auto pData = m_Storage.GetByteBlobPtr().GetPtr();
  auto nodes = ezMakeArrayPtr(reinterpret_cast<Node*>(pData), uiNumNodes);

  ezUInt8* pAdditionalData = pData + uiNumNodes * sizeof(Node);

  for (auto& node : nodes)
  {
    inout_stream >> node.m_Type;
    inout_stream >> node.m_DeductedDataType;

    node.m_Function = GetExecuteFunction(node.m_Type, node.m_DeductedDataType);

    EZ_SUCCEED_OR_RETURN(node.m_ExecutionIndices.ReadFromStream(node.m_NumExecutionIndices, inout_stream, pAdditionalData));
    EZ_SUCCEED_OR_RETURN(node.m_InputDataOffsets.ReadFromStream(node.m_NumInputDataOffsets, inout_stream, pAdditionalData));
    EZ_SUCCEED_OR_RETURN(node.m_OutputDataOffsets.ReadFromStream(node.m_NumOutputDataOffsets, inout_stream, pAdditionalData));

    if (auto func = GetUserDataContext(node.m_Type).m_DeserializeFunc)
    {
      EZ_SUCCEED_OR_RETURN(func(node, inout_stream, pAdditionalData));
    }
  }

  m_Nodes = nodes;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

static const ezTypeVersion s_uiVisualScriptDataDescriptionVersion = 1;

ezResult ezVisualScriptDataDescription::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_uiVisualScriptDataDescriptionVersion);

  for (auto& typeInfo : m_PerTypeInfo)
  {
    inout_stream << typeInfo.m_uiStartOffset;
    inout_stream << typeInfo.m_uiCount;
  }

  inout_stream << m_uiStorageSizeNeeded;

  return EZ_SUCCESS;
}

ezResult ezVisualScriptDataDescription::Deserialize(ezStreamReader& inout_stream)
{
  ezTypeVersion uiVersion = inout_stream.ReadVersion(s_uiVisualScriptDataDescriptionVersion);

  for (auto& typeInfo : m_PerTypeInfo)
  {
    inout_stream >> typeInfo.m_uiStartOffset;
    inout_stream >> typeInfo.m_uiCount;
  }

  inout_stream >> m_uiStorageSizeNeeded;

  return EZ_SUCCESS;
}

void ezVisualScriptDataDescription::Clear()
{
  ezMemoryUtils::ZeroFillArray(m_PerTypeInfo);
  m_uiStorageSizeNeeded = 0;
}

void ezVisualScriptDataDescription::CalculatePerTypeStartOffsets()
{
  ezUInt32 uiOffset = 0;
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_PerTypeInfo); ++i)
  {
    auto dataType = static_cast<ezVisualScriptDataType::Enum>(i);
    auto& typeInfo = m_PerTypeInfo[i];

    if (typeInfo.m_uiCount > 0)
    {
      uiOffset = ezMemoryUtils::AlignSize(uiOffset, ezVisualScriptDataType::GetStorageAlignment(dataType));
      typeInfo.m_uiStartOffset = uiOffset;

      uiOffset += ezVisualScriptDataType::GetStorageSize(dataType) * typeInfo.m_uiCount;
    }
  }

  m_uiStorageSizeNeeded = uiOffset;
}

//////////////////////////////////////////////////////////////////////////

ezVisualScriptDataStorage::ezVisualScriptDataStorage(const ezSharedPtr<const ezVisualScriptDataDescription>& pDesc)
  : m_pDesc(pDesc)
{
}

ezVisualScriptDataStorage::~ezVisualScriptDataStorage()
{
  DeallocateStorage();
}

void ezVisualScriptDataStorage::AllocateStorage()
{
  m_Storage.SetCountUninitialized(m_pDesc->m_uiStorageSizeNeeded);
  m_Storage.ZeroFill();

  auto pData = m_Storage.GetByteBlobPtr().GetPtr();

  for (ezUInt32 scriptDataType = 0; scriptDataType < ezVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == ezVisualScriptDataType::String)
    {
      auto pStrings = reinterpret_cast<ezString*>(pData + typeInfo.m_uiStartOffset);
      ezMemoryUtils::Construct(pStrings, typeInfo.m_uiCount);
    }
    else if (scriptDataType == ezVisualScriptDataType::Variant)
    {
      auto pVariants = reinterpret_cast<ezVariant*>(pData + typeInfo.m_uiStartOffset);
      ezMemoryUtils::Construct(pVariants, typeInfo.m_uiCount);
    }
    else if (scriptDataType == ezVisualScriptDataType::Array)
    {
      auto pVariantArrays = reinterpret_cast<ezVariantArray*>(pData + typeInfo.m_uiStartOffset);
      ezMemoryUtils::Construct(pVariantArrays, typeInfo.m_uiCount);
    }
    else if (scriptDataType == ezVisualScriptDataType::Map)
    {
      auto pVariantMaps = reinterpret_cast<ezVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      ezMemoryUtils::Construct(pVariantMaps, typeInfo.m_uiCount);
    }
  }
}

void ezVisualScriptDataStorage::DeallocateStorage()
{
  if (m_Storage.GetByteBlobPtr().IsEmpty())
    return;

  auto pData = m_Storage.GetByteBlobPtr().GetPtr();

  for (ezUInt32 scriptDataType = 0; scriptDataType < ezVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == ezVisualScriptDataType::String)
    {
      auto pStrings = reinterpret_cast<ezString*>(pData + typeInfo.m_uiStartOffset);
      ezMemoryUtils::Destruct(pStrings, typeInfo.m_uiCount);
    }
    else if (scriptDataType == ezVisualScriptDataType::Variant)
    {
      auto pVariants = reinterpret_cast<ezVariant*>(pData + typeInfo.m_uiStartOffset);
      ezMemoryUtils::Destruct(pVariants, typeInfo.m_uiCount);
    }
    else if (scriptDataType == ezVisualScriptDataType::Array)
    {
      auto pVariantArrays = reinterpret_cast<ezVariantArray*>(pData + typeInfo.m_uiStartOffset);
      ezMemoryUtils::Destruct(pVariantArrays, typeInfo.m_uiCount);
    }
    else if (scriptDataType == ezVisualScriptDataType::Map)
    {
      auto pVariantMaps = reinterpret_cast<ezVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      ezMemoryUtils::Destruct(pVariantMaps, typeInfo.m_uiCount);
    }
  }

  m_Storage.Clear();
}

ezResult ezVisualScriptDataStorage::Serialize(ezStreamWriter& inout_stream) const
{
  auto pData = m_Storage.GetByteBlobPtr().GetPtr();

  for (ezUInt32 scriptDataType = 0; scriptDataType < ezVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == ezVisualScriptDataType::String)
    {
      auto pStrings = reinterpret_cast<const ezString*>(pData + typeInfo.m_uiStartOffset);
      auto pStringsEnd = pStrings + typeInfo.m_uiCount;
      while (pStrings < pStringsEnd)
      {
        inout_stream << *pStrings;
        ++pStrings;
      }
    }
    else if (scriptDataType == ezVisualScriptDataType::Variant)
    {
      auto pVariants = reinterpret_cast<const ezVariant*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantsEnd = pVariants + typeInfo.m_uiCount;
      while (pVariants < pVariantsEnd)
      {
        inout_stream << *pVariants;
        ++pVariants;
      }
    }
    else if (scriptDataType == ezVisualScriptDataType::Array)
    {
      auto pVariantArrays = reinterpret_cast<const ezVariantArray*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantArraysEnd = pVariantArrays + typeInfo.m_uiCount;
      while (pVariantArrays < pVariantArraysEnd)
      {
        EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(*pVariantArrays));
        ++pVariantArrays;
      }
    }
    else if (scriptDataType == ezVisualScriptDataType::Map)
    {
      auto pVariantMaps = reinterpret_cast<const ezVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantMapsEnd = pVariantMaps + typeInfo.m_uiCount;
      while (pVariantMaps < pVariantMapsEnd)
      {
        EZ_SUCCEED_OR_RETURN(inout_stream.WriteHashTable(*pVariantMaps));
        ++pVariantMaps;
      }
    }
    else
    {
      const ezUInt32 uiBytesToWrite = typeInfo.m_uiCount * ezVisualScriptDataType::GetStorageSize(static_cast<ezVisualScriptDataType::Enum>(scriptDataType));
      EZ_SUCCEED_OR_RETURN(inout_stream.WriteBytes(pData + typeInfo.m_uiStartOffset, uiBytesToWrite));
    }
  }

  return EZ_SUCCESS;
}

ezResult ezVisualScriptDataStorage::Deserialize(ezStreamReader& inout_stream)
{
  if (m_Storage.GetByteBlobPtr().IsEmpty())
  {
    AllocateStorage();
  }

  auto pData = m_Storage.GetByteBlobPtr().GetPtr();

  for (ezUInt32 scriptDataType = 0; scriptDataType < ezVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == ezVisualScriptDataType::String)
    {
      auto pStrings = reinterpret_cast<ezString*>(pData + typeInfo.m_uiStartOffset);
      auto pStringsEnd = pStrings + typeInfo.m_uiCount;
      while (pStrings < pStringsEnd)
      {
        inout_stream >> *pStrings;
        ++pStrings;
      }
    }
    else if (scriptDataType == ezVisualScriptDataType::Variant)
    {
      auto pVariants = reinterpret_cast<ezVariant*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantsEnd = pVariants + typeInfo.m_uiCount;
      while (pVariants < pVariantsEnd)
      {
        inout_stream >> *pVariants;
        ++pVariants;
      }
    }
    else if (scriptDataType == ezVisualScriptDataType::Array)
    {
      auto pVariantArrays = reinterpret_cast<ezVariantArray*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantArraysEnd = pVariantArrays + typeInfo.m_uiCount;
      while (pVariantArrays < pVariantArraysEnd)
      {
        EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(*pVariantArrays));
        ++pVariantArrays;
      }
    }
    else if (scriptDataType == ezVisualScriptDataType::Map)
    {
      auto pVariantMaps = reinterpret_cast<ezVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantMapsEnd = pVariantMaps + typeInfo.m_uiCount;
      while (pVariantMaps < pVariantMapsEnd)
      {
        EZ_SUCCEED_OR_RETURN(inout_stream.ReadHashTable(*pVariantMaps));
        ++pVariantMaps;
      }
    }
    else
    {
      const ezUInt32 uiBytesToRead = typeInfo.m_uiCount * ezVisualScriptDataType::GetStorageSize(static_cast<ezVisualScriptDataType::Enum>(scriptDataType));
      inout_stream.ReadBytes(pData + typeInfo.m_uiStartOffset, uiBytesToRead);
    }
  }

  return EZ_SUCCESS;
}

ezTypedPointer ezVisualScriptDataStorage::GetPointerData(DataOffset dataOffset, ezUInt32 uiExecutionCounter)
{
  m_pDesc->CheckOffset(dataOffset, nullptr);
  auto pData = m_Storage.GetByteBlobPtr().GetPtr() + dataOffset.m_uiByteOffset;

  if (dataOffset.m_uiDataType == ezVisualScriptDataType::GameObject)
  {
    auto& gameObjectHandle = *reinterpret_cast<const ezVisualScriptGameObjectHandle*>(pData);
    return ezTypedPointer(gameObjectHandle.GetPtr(uiExecutionCounter), ezGetStaticRTTI<ezGameObject>());
  }
  else if (dataOffset.m_uiDataType == ezVisualScriptDataType::Component)
  {
    auto& componentHandle = *reinterpret_cast<const ezVisualScriptComponentHandle*>(pData);
    ezComponent* pComponent = componentHandle.GetPtr(uiExecutionCounter);
    return ezTypedPointer(pComponent, pComponent != nullptr ? pComponent->GetDynamicRTTI() : nullptr);
  }
  else if (dataOffset.m_uiDataType == ezVisualScriptDataType::TypedPointer)
  {
    return *reinterpret_cast<const ezTypedPointer*>(pData);
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezTypedPointer();
}

ezVariant ezVisualScriptDataStorage::GetDataAsVariant(DataOffset dataOffset, ezVariantType::Enum expectedType, ezUInt32 uiExecutionCounter) const
{
  auto scriptDataType = static_cast<ezVisualScriptDataType::Enum>(dataOffset.m_uiDataType);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // expectedType == Invalid means that the caller expects an ezVariant so we decide solely based on the scriptDataType.
  // We set the expectedType to the equivalent of the scriptDataType here so we don't need to check for expectedType == Invalid in all the asserts below.
  if (expectedType == ezVariantType::Invalid)
  {
    expectedType = ezVisualScriptDataType::GetVariantType(scriptDataType);
  }
#endif

  switch (scriptDataType)
  {
    case ezVisualScriptDataType::Bool:
      EZ_ASSERT_DEBUG(expectedType == ezVariantType::Bool, "");
      return GetData<bool>(dataOffset);
    case ezVisualScriptDataType::Byte:
      EZ_ASSERT_DEBUG(expectedType == ezVariantType::UInt8, "");
      return GetData<ezUInt8>(dataOffset);
    case ezVisualScriptDataType::Int:
      EZ_ASSERT_DEBUG(expectedType == ezVariantType::Int32, "");
      return GetData<ezInt32>(dataOffset);
    case ezVisualScriptDataType::Int64:
      EZ_ASSERT_DEBUG(expectedType == ezVariantType::Int64, "");
      return GetData<ezInt64>(dataOffset);
    case ezVisualScriptDataType::Float:
      EZ_ASSERT_DEBUG(expectedType == ezVariantType::Float, "");
      return GetData<float>(dataOffset);
    case ezVisualScriptDataType::Double:
      EZ_ASSERT_DEBUG(expectedType == ezVariantType::Double, "");
      return GetData<double>(dataOffset);
    case ezVisualScriptDataType::String:
      if (expectedType == ezVariantType::Invalid || expectedType == ezVariantType::String)
      {
        return GetData<ezString>(dataOffset);
      }
      else if (expectedType == ezVariantType::StringView)
      {
        return GetData<ezString>(dataOffset).GetView();
      }
      EZ_ASSERT_NOT_IMPLEMENTED;
    case ezVisualScriptDataType::Variant:
      return GetData<ezVariant>(dataOffset);
    case ezVisualScriptDataType::Array:
      EZ_ASSERT_DEBUG(expectedType == ezVariantType::VariantArray, "");
      return GetData<ezVariantArray>(dataOffset);
    case ezVisualScriptDataType::Map:
      EZ_ASSERT_DEBUG(expectedType == ezVariantType::VariantDictionary, "");
      return GetData<ezVariantDictionary>(dataOffset);
      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return ezVariant();
}

void ezVisualScriptDataStorage::SetDataFromVariant(DataOffset dataOffset, const ezVariant& value, ezUInt32 uiExecutionCounter)
{
  if (dataOffset.IsValid() == false)
    return;

  auto scriptDataType = static_cast<ezVisualScriptDataType::Enum>(dataOffset.m_uiDataType);
  switch (scriptDataType)
  {
    case ezVisualScriptDataType::Bool:
      SetData(dataOffset, value.Get<bool>());
      break;
    case ezVisualScriptDataType::Byte:
      if (value.IsA<ezInt8>())
      {
        SetData(dataOffset, ezUInt8(value.Get<ezInt8>()));
      }
      else
      {
        SetData(dataOffset, value.Get<ezUInt8>());
      }      
      break;
    case ezVisualScriptDataType::Int:
      if (value.IsA<ezInt16>())
      {
        SetData(dataOffset, ezInt32(value.Get<ezInt16>()));
      }
      else if (value.IsA<ezUInt16>())
      {
        SetData(dataOffset, ezInt32(value.Get<ezUInt16>()));
      }
      else if (value.IsA<ezInt32>())
      {
        SetData(dataOffset, value.Get<ezInt32>());
      }
      else
      {
        SetData(dataOffset, ezInt32(value.Get<ezUInt32>()));
      }
      break;
    case ezVisualScriptDataType::Int64:
      if (value.IsA<ezInt64>())
      {
        SetData(dataOffset, value.Get<ezInt64>());
      }
      else
      {
        SetData(dataOffset, ezInt64(value.Get<ezUInt64>()));
      }
      break;
    case ezVisualScriptDataType::Float:
      SetData(dataOffset, value.Get<float>());
      break;
    case ezVisualScriptDataType::Double:
      SetData(dataOffset, value.Get<double>());
      break;
    case ezVisualScriptDataType::Color:
      SetData(dataOffset, value.Get<ezColor>());
      break;
    case ezVisualScriptDataType::Vector3:
      SetData(dataOffset, value.Get<ezVec3>());
      break;
    case ezVisualScriptDataType::Quaternion:
      SetData(dataOffset, value.Get<ezQuat>());
      break;
    case ezVisualScriptDataType::Transform:
      SetData(dataOffset, value.Get<ezTransform>());
      break;
    case ezVisualScriptDataType::Time:
      SetData(dataOffset, value.Get<ezTime>());
      break;
    case ezVisualScriptDataType::Angle:
      SetData(dataOffset, value.Get<ezAngle>());
      break;
    case ezVisualScriptDataType::String:
      if (value.IsA<ezStringView>())
      {
        SetData(dataOffset, ezString(value.Get<ezStringView>()));
      }
      else
      {
        SetData(dataOffset, value.Get<ezString>());
      }
      break;
    case ezVisualScriptDataType::GameObject:
      SetPointerData(dataOffset, value.Get<ezGameObject*>(), ezGetStaticRTTI<ezGameObject>(), uiExecutionCounter);
      break;
    case ezVisualScriptDataType::Component:
      SetPointerData(dataOffset, value.Get<ezComponent*>(), ezGetStaticRTTI<ezComponent>(), uiExecutionCounter);
      break;
    case ezVisualScriptDataType::TypedPointer:
    {
      ezTypedPointer typedPtr = value.Get<ezTypedPointer>();
      SetPointerData(dataOffset, typedPtr.m_pObject, typedPtr.m_pType, uiExecutionCounter);
    }
    break;
    case ezVisualScriptDataType::Variant:
      SetData(dataOffset, value);
      break;
    case ezVisualScriptDataType::Array:
      SetData(dataOffset, value.Get<ezVariantArray>());
      break;
    case ezVisualScriptDataType::Map:
      SetData(dataOffset, value.Get<ezVariantDictionary>());
      break;
      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}
