#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Scripting/ScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptData.h>

namespace
{
  static const char* s_DataOffsetSourceNames[] = {
    "Local",
    "Instance",
    "Constant",
  };
  static_assert(EZ_ARRAY_SIZE(s_DataOffsetSourceNames) == (size_t)ezVisualScriptDataDescription::DataOffset::Source::Count);
} // namespace

// Check that DataOffset fits in one uint32 and also check that we have enough bits for dataType and source.
static_assert(sizeof(ezVisualScriptDataDescription::DataOffset) == sizeof(ezUInt32));
static_assert(ezVisualScriptDataType::Count <= EZ_BIT(ezVisualScriptDataDescription::DataOffset::TYPE_BITS));
static_assert(ezVisualScriptDataDescription::DataOffset::Source::Count <= EZ_BIT(ezVisualScriptDataDescription::DataOffset::SOURCE_BITS));

// static
const char* ezVisualScriptDataDescription::DataOffset::Source::GetName(Enum source)
{
  EZ_ASSERT_DEBUG(source >= 0 && static_cast<ezUInt32>(source) < EZ_ARRAY_SIZE(s_DataOffsetSourceNames), "Out of bounds access");
  return s_DataOffsetSourceNames[source];
}

//////////////////////////////////////////////////////////////////////////

static const ezTypeVersion s_uiVisualScriptDataDescriptionVersion = 2;

ezResult ezVisualScriptDataDescription::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_uiVisualScriptDataDescriptionVersion);

  for (auto& typeInfo : m_PerTypeInfo)
  {
    inout_stream << typeInfo.m_uiCount;
  }

  return EZ_SUCCESS;
}

ezResult ezVisualScriptDataDescription::Deserialize(ezStreamReader& inout_stream)
{
  ezTypeVersion uiVersion = inout_stream.ReadVersion(s_uiVisualScriptDataDescriptionVersion);
  if (uiVersion < 2)
  {
    ezLog::Error("Invalid visual script data desc version. Expected >= 2 but got {}. Visual Script needs re-export", uiVersion);
    return EZ_FAILURE;
  }

  for (auto& typeInfo : m_PerTypeInfo)
  {
    inout_stream >> typeInfo.m_uiCount;
  }

  CalculatePerTypeStartOffsets();

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

void ezVisualScriptDataStorage::AllocateStorage(ezAllocator* pAllocator)
{
  EZ_ASSERT_DEV(IsAllocated() == false, "Storage already allocated");

  m_Storage = EZ_NEW_ARRAY(pAllocator, ezUInt8, m_pDesc->m_uiStorageSizeNeeded);
  ezMemoryUtils::ZeroFill(m_Storage.GetPtr(), m_Storage.GetCount());
  m_pAllocator = pAllocator;

  auto pData = m_Storage.GetPtr();

  for (ezUInt32 scriptDataType = 0; scriptDataType < ezVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == ezVisualScriptDataType::String)
    {
      auto pStrings = reinterpret_cast<ezString*>(pData + typeInfo.m_uiStartOffset);
      ezMemoryUtils::Construct<SkipTrivialTypes>(pStrings, typeInfo.m_uiCount);
    }
    if (scriptDataType == ezVisualScriptDataType::HashedString)
    {
      auto pStrings = reinterpret_cast<ezHashedString*>(pData + typeInfo.m_uiStartOffset);
      ezMemoryUtils::Construct<SkipTrivialTypes>(pStrings, typeInfo.m_uiCount);
    }
    else if (scriptDataType == ezVisualScriptDataType::Variant)
    {
      auto pVariants = reinterpret_cast<ezVariant*>(pData + typeInfo.m_uiStartOffset);
      ezMemoryUtils::Construct<SkipTrivialTypes>(pVariants, typeInfo.m_uiCount);
    }
    else if (scriptDataType == ezVisualScriptDataType::Array)
    {
      auto pVariantArrays = reinterpret_cast<ezVariantArray*>(pData + typeInfo.m_uiStartOffset);
      ezMemoryUtils::Construct<SkipTrivialTypes>(pVariantArrays, typeInfo.m_uiCount);
    }
    else if (scriptDataType == ezVisualScriptDataType::Map)
    {
      auto pVariantMaps = reinterpret_cast<ezVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      ezMemoryUtils::Construct<SkipTrivialTypes>(pVariantMaps, typeInfo.m_uiCount);
    }
  }
}

void ezVisualScriptDataStorage::DeallocateStorage()
{
  if (IsAllocated() == false)
    return;

  auto pData = m_Storage.GetPtr();

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
    else if (scriptDataType == ezVisualScriptDataType::HashedString)
    {
      auto pStrings = reinterpret_cast<ezHashedString*>(pData + typeInfo.m_uiStartOffset);
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

  EZ_DELETE_ARRAY(m_pAllocator, m_Storage);
  m_pAllocator = nullptr;
}

ezResult ezVisualScriptDataStorage::Serialize(ezStreamWriter& inout_stream) const
{
  auto pData = m_Storage.GetPtr();

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
    else if (scriptDataType == ezVisualScriptDataType::HashedString)
    {
      auto pStrings = reinterpret_cast<const ezHashedString*>(pData + typeInfo.m_uiStartOffset);
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
    else if (scriptDataType == ezVisualScriptDataType::GameObject ||
             scriptDataType == ezVisualScriptDataType::Component ||
             scriptDataType == ezVisualScriptDataType::TypedPointer ||
             scriptDataType == ezVisualScriptDataType::Coroutine)
    {
      ezLog::Error("Cannot serialize visual script data type '{}'", ezVisualScriptDataType::GetName(static_cast<ezVisualScriptDataType::Enum>(scriptDataType)));
      return EZ_FAILURE;
    }
    else
    {
      const ezUInt32 uiBytesToWrite = typeInfo.m_uiCount * ezVisualScriptDataType::GetStorageSize(static_cast<ezVisualScriptDataType::Enum>(scriptDataType));
      EZ_SUCCEED_OR_RETURN(inout_stream.WriteBytes(pData + typeInfo.m_uiStartOffset, uiBytesToWrite));
    }
  }

  return EZ_SUCCESS;
}

ezResult ezVisualScriptDataStorage::Deserialize(ezStreamReader& inout_stream, ezAllocator* pAllocator)
{
  if (IsAllocated() == false)
  {
    AllocateStorage(pAllocator);
  }

  auto pData = m_Storage.GetPtr();

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
    else if (scriptDataType == ezVisualScriptDataType::HashedString)
    {
      auto pStrings = reinterpret_cast<ezHashedString*>(pData + typeInfo.m_uiStartOffset);
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

ezTypedPointer ezVisualScriptDataStorage::GetPointerData(DataOffset dataOffset, ezUInt32 uiExecutionCounter) const
{
  m_pDesc->CheckOffset(dataOffset, nullptr);
  auto pData = m_Storage.GetPtr() + dataOffset.m_uiByteOffset;

  if (dataOffset.m_uiType == ezVisualScriptDataType::GameObject)
  {
    auto& gameObjectHandle = *reinterpret_cast<const ezVisualScriptGameObjectHandle*>(pData);
    return ezTypedPointer(gameObjectHandle.GetPtr(uiExecutionCounter), ezGetStaticRTTI<ezGameObject>());
  }
  else if (dataOffset.m_uiType == ezVisualScriptDataType::Component)
  {
    auto& componentHandle = *reinterpret_cast<const ezVisualScriptComponentHandle*>(pData);
    ezComponent* pComponent = componentHandle.GetPtr(uiExecutionCounter);
    return ezTypedPointer(pComponent, pComponent != nullptr ? pComponent->GetDynamicRTTI() : nullptr);
  }
  else if (dataOffset.m_uiType == ezVisualScriptDataType::TypedPointer)
  {
    return *reinterpret_cast<const ezTypedPointer*>(pData);
  }

  ezTypedPointer t;
  t.m_pObject = const_cast<ezUInt8*>(pData);
  t.m_pType = ezVisualScriptDataType::GetRtti(static_cast<ezVisualScriptDataType::Enum>(dataOffset.m_uiType));
  return t;
}

ezVariant ezVisualScriptDataStorage::GetDataAsVariant(DataOffset dataOffset, const ezRTTI* pExpectedType, ezUInt32 uiExecutionCounter) const
{
  auto scriptDataType = dataOffset.GetType();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // pExpectedType == nullptr means that the caller expects an ezVariant so we decide solely based on the scriptDataType.
  // We set the pExpectedType to the equivalent of the scriptDataType here so we don't need to check for pExpectedType == nullptr in all the asserts below.
  if (pExpectedType == nullptr || pExpectedType == ezGetStaticRTTI<ezVariant>())
  {
    pExpectedType = ezVisualScriptDataType::GetRtti(scriptDataType);
  }
#endif

  switch (scriptDataType)
  {
    case ezVisualScriptDataType::Invalid:
      return ezVariant();

    case ezVisualScriptDataType::Bool:
      EZ_ASSERT_DEBUG(pExpectedType == ezGetStaticRTTI<bool>(), "");
      return GetData<bool>(dataOffset);

    case ezVisualScriptDataType::Byte:
      EZ_ASSERT_DEBUG(pExpectedType == ezGetStaticRTTI<ezUInt8>(), "");
      return GetData<ezUInt8>(dataOffset);

    case ezVisualScriptDataType::Int:
      if (pExpectedType == ezGetStaticRTTI<ezInt16>())
      {
        return static_cast<ezInt16>(GetData<ezInt32>(dataOffset));
      }
      else if (pExpectedType == ezGetStaticRTTI<ezUInt16>())
      {
        return static_cast<ezUInt16>(GetData<ezInt32>(dataOffset));
      }
      else if (pExpectedType == ezGetStaticRTTI<ezInt32>())
      {
        return GetData<ezInt32>(dataOffset);
      }
      else
      {
        return static_cast<ezUInt32>(GetData<ezInt32>(dataOffset));
      }
      EZ_ASSERT_NOT_IMPLEMENTED;

    case ezVisualScriptDataType::Int64:
      EZ_ASSERT_DEBUG(pExpectedType->GetTypeFlags().IsSet(ezTypeFlags::IsEnum) || pExpectedType->GetTypeFlags().IsSet(ezTypeFlags::Bitflags) || pExpectedType == ezGetStaticRTTI<ezInt64>(), "");
      return GetData<ezInt64>(dataOffset);

    case ezVisualScriptDataType::Float:
      EZ_ASSERT_DEBUG(pExpectedType == ezGetStaticRTTI<float>(), "");
      return GetData<float>(dataOffset);

    case ezVisualScriptDataType::Double:
      EZ_ASSERT_DEBUG(pExpectedType == ezGetStaticRTTI<double>(), "");
      return GetData<double>(dataOffset);

    case ezVisualScriptDataType::Color:
      EZ_ASSERT_DEBUG(pExpectedType == ezGetStaticRTTI<ezColor>(), "");
      return GetData<ezColor>(dataOffset);

    case ezVisualScriptDataType::Vector3:
      EZ_ASSERT_DEBUG(pExpectedType == ezGetStaticRTTI<ezVec3>(), "");
      return GetData<ezVec3>(dataOffset);

    case ezVisualScriptDataType::Quaternion:
      EZ_ASSERT_DEBUG(pExpectedType == ezGetStaticRTTI<ezQuat>(), "");
      return GetData<ezQuat>(dataOffset);

    case ezVisualScriptDataType::Transform:
      EZ_ASSERT_DEBUG(pExpectedType == ezGetStaticRTTI<ezTransform>(), "");
      return GetData<ezTransform>(dataOffset);

    case ezVisualScriptDataType::Time:
      EZ_ASSERT_DEBUG(pExpectedType == ezGetStaticRTTI<ezTime>(), "");
      return GetData<ezTime>(dataOffset);

    case ezVisualScriptDataType::Angle:
      EZ_ASSERT_DEBUG(pExpectedType == ezGetStaticRTTI<ezAngle>(), "");
      return GetData<ezAngle>(dataOffset);

    case ezVisualScriptDataType::String:
      if (pExpectedType == nullptr || pExpectedType == ezGetStaticRTTI<ezString>() || pExpectedType == ezGetStaticRTTI<const char*>())
      {
        return GetData<ezString>(dataOffset);
      }
      else if (pExpectedType == ezGetStaticRTTI<ezStringView>())
      {
        return ezVariant(GetData<ezString>(dataOffset).GetView(), false);
      }
      EZ_ASSERT_NOT_IMPLEMENTED;

    case ezVisualScriptDataType::HashedString:
      if (pExpectedType == nullptr || pExpectedType == ezGetStaticRTTI<ezHashedString>())
      {
        return GetData<ezHashedString>(dataOffset);
      }
      else if (pExpectedType == ezGetStaticRTTI<ezTempHashedString>())
      {
        return ezTempHashedString(GetData<ezHashedString>(dataOffset));
      }
      EZ_ASSERT_NOT_IMPLEMENTED;

    case ezVisualScriptDataType::GameObject:
      if (pExpectedType == nullptr || pExpectedType == ezGetStaticRTTI<ezGameObject>())
      {
        return GetPointerData(dataOffset, uiExecutionCounter);
      }
      else if (pExpectedType == ezGetStaticRTTI<ezGameObjectHandle>())
      {
        return GetData<ezGameObjectHandle>(dataOffset);
      }
      EZ_ASSERT_NOT_IMPLEMENTED;

    case ezVisualScriptDataType::Component:
      EZ_ASSERT_DEBUG(pExpectedType == ezGetStaticRTTI<ezComponentHandle>(), "");
      return GetData<ezComponentHandle>(dataOffset);

    case ezVisualScriptDataType::TypedPointer:
      return GetPointerData(dataOffset, uiExecutionCounter);

    case ezVisualScriptDataType::Variant:
      return GetData<ezVariant>(dataOffset);

    case ezVisualScriptDataType::Array:
      EZ_ASSERT_DEBUG(pExpectedType == ezGetStaticRTTI<ezVariantArray>(), "");
      return GetData<ezVariantArray>(dataOffset);

    case ezVisualScriptDataType::Map:
      EZ_ASSERT_DEBUG(pExpectedType == ezGetStaticRTTI<ezVariantDictionary>(), "");
      return GetData<ezVariantDictionary>(dataOffset);

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return ezVariant();
}

void ezVisualScriptDataStorage::SetDataFromVariant(DataOffset dataOffset, const ezVariant& value, ezUInt32 uiExecutionCounter)
{
  if (dataOffset.IsValid() == false)
    return;

  auto scriptDataType = dataOffset.GetType();
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
    case ezVisualScriptDataType::HashedString:
      if (value.IsA<ezTempHashedString>())
      {
        EZ_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        SetData(dataOffset, value.Get<ezHashedString>());
      }
      break;
    case ezVisualScriptDataType::GameObject:
      if (value.IsA<ezGameObjectHandle>())
      {
        SetData(dataOffset, value.Get<ezGameObjectHandle>());
      }
      else
      {
        SetPointerData(dataOffset, value.Get<ezGameObject*>(), ezGetStaticRTTI<ezGameObject>(), uiExecutionCounter);
      }
      break;
    case ezVisualScriptDataType::Component:
      if (value.IsA<ezComponentHandle>())
      {
        SetData(dataOffset, value.Get<ezComponentHandle>());
      }
      else
      {
        SetPointerData(dataOffset, value.Get<ezComponent*>(), ezGetStaticRTTI<ezComponent>(), uiExecutionCounter);
      }
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
    case ezVisualScriptDataType::Coroutine:
      SetData(dataOffset, value.Get<ezScriptCoroutineHandle>());
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}
