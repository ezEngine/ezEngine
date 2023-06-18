
EZ_FORCE_INLINE void ezVisualScriptDataDescription::CheckOffset(DataOffset dataOffset, const ezRTTI* pType) const
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  auto expectedDataType = dataOffset.GetType();
  auto& offsetAndCount = m_PerTypeInfo[expectedDataType];
  const ezUInt32 uiLastOffset = offsetAndCount.m_uiStartOffset + (offsetAndCount.m_uiCount - 1) * ezVisualScriptDataType::GetStorageSize(expectedDataType);
  EZ_ASSERT_DEBUG(dataOffset.m_uiByteOffset >= offsetAndCount.m_uiStartOffset && dataOffset.m_uiByteOffset <= uiLastOffset, "Invalid data offset");

  if (pType != nullptr)
  {
    auto givenDataType = ezVisualScriptDataType::FromRtti(pType);
    EZ_ASSERT_DEBUG(expectedDataType == givenDataType, "Data type mismatch, expected '{}' but got '{}'({})", ezVisualScriptDataType::GetName(expectedDataType), ezVisualScriptDataType::GetName(givenDataType), pType->GetTypeName());
  }
#endif
}

EZ_FORCE_INLINE ezVisualScriptDataDescription::DataOffset ezVisualScriptDataDescription::GetOffset(ezVisualScriptDataType::Enum dataType, ezUInt32 uiIndex, DataOffset::Source::Enum source) const
{
  auto& offsetAndCount = m_PerTypeInfo[dataType];
  ezUInt32 uiByteOffset = ezInvalidIndex;
  if (uiIndex < offsetAndCount.m_uiCount)
  {
    uiByteOffset = offsetAndCount.m_uiStartOffset + uiIndex * ezVisualScriptDataType::GetStorageSize(dataType);
  }

  return DataOffset(uiByteOffset, dataType, source);
}

//////////////////////////////////////////////////////////////////////////

EZ_ALWAYS_INLINE bool ezVisualScriptDataStorage::IsAllocated() const
{
  return m_Storage.GetByteBlobPtr().IsEmpty() == false;
}

template <typename T>
const T& ezVisualScriptDataStorage::GetData(DataOffset dataOffset) const
{
  static_assert(!std::is_pointer<T>::value && !std::is_same<T, ezTypedPointer>::value, "Use GetPointerData instead");

  m_pDesc->CheckOffset(dataOffset, ezGetStaticRTTI<T>());

  return *reinterpret_cast<const T*>(m_Storage.GetByteBlobPtr().GetPtr() + dataOffset.m_uiByteOffset);
}

template <typename T>
T& ezVisualScriptDataStorage::GetWritableData(DataOffset dataOffset)
{
  static_assert(!std::is_pointer<T>::value && !std::is_same<T, ezTypedPointer>::value, "Use GetPointerData instead");

  m_pDesc->CheckOffset(dataOffset, ezGetStaticRTTI<T>());

  return *reinterpret_cast<T*>(m_Storage.GetByteBlobPtr().GetPtr() + dataOffset.m_uiByteOffset);
}

template <typename T>
void ezVisualScriptDataStorage::SetData(DataOffset dataOffset, const T& value)
{
  static_assert(!std::is_pointer<T>::value, "Use SetPointerData instead");

  if (dataOffset.m_uiByteOffset < m_Storage.GetByteBlobPtr().GetCount())
  {
    m_pDesc->CheckOffset(dataOffset, ezGetStaticRTTI<T>());

    auto pData = m_Storage.GetByteBlobPtr().GetPtr() + dataOffset.m_uiByteOffset;

    if constexpr (std::is_same<T, ezGameObjectHandle>::value)
    {
      auto& storedHandle = *reinterpret_cast<ezVisualScriptGameObjectHandle*>(pData);
      storedHandle.AssignHandle(value);
    }
    else if constexpr (std::is_same<T, ezComponentHandle>::value)
    {
      auto& storedHandle = *reinterpret_cast<ezVisualScriptComponentHandle*>(pData);
      storedHandle.AssignHandle(value);
    }
    else if constexpr (std::is_same<T, ezStringView>::value)
    {
      *reinterpret_cast<ezString*>(pData) = value;
    }
    else
    {
      *reinterpret_cast<T*>(pData) = value;
    }
  }
}

template <typename T>
void ezVisualScriptDataStorage::SetPointerData(DataOffset dataOffset, T ptr, const ezRTTI* pType, ezUInt32 uiExecutionCounter)
{
  static_assert(std::is_pointer<T>::value);

  if (dataOffset.m_uiByteOffset < m_Storage.GetByteBlobPtr().GetCount())
  {
    auto pData = m_Storage.GetByteBlobPtr().GetPtr() + dataOffset.m_uiByteOffset;

    if constexpr (std::is_same<T, ezGameObject*>::value)
    {
      m_pDesc->CheckOffset(dataOffset, ezGetStaticRTTI<ezGameObject>());

      auto& storedHandle = *reinterpret_cast<ezVisualScriptGameObjectHandle*>(pData);
      storedHandle.AssignPtr(ptr, uiExecutionCounter);
    }
    else if constexpr (std::is_same<T, ezComponent*>::value)
    {
      m_pDesc->CheckOffset(dataOffset, ezGetStaticRTTI<ezComponent>());

      auto& storedHandle = *reinterpret_cast<ezVisualScriptComponentHandle*>(pData);
      storedHandle.AssignPtr(ptr, uiExecutionCounter);
    }
    else
    {
      EZ_ASSERT_DEBUG(!pType || pType->IsDerivedFrom<ezComponent>() == false, "Component type '{}' is stored as typed pointer, cast to ezComponent first to ensure correct storage", pType->GetTypeName());

      m_pDesc->CheckOffset(dataOffset, pType);

      auto& typedPointer = *reinterpret_cast<ezTypedPointer*>(pData);
      typedPointer.m_pObject = ptr;
      typedPointer.m_pType = pType;
    }
  }
}
