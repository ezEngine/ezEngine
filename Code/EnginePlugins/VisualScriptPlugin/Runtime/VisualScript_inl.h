#include "VisualScript.h"

// static
template <typename T, ezUInt32 Size>
void ezVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::AddAdditionalDataSize(ezArrayPtr<const T> a, ezUInt32& inout_additionalDataSize)
{
  if (a.GetCount() > Size)
  {
    inout_additionalDataSize = ezMemoryUtils::AlignSize<ezUInt32>(inout_additionalDataSize, EZ_ALIGNMENT_OF(T));
    inout_additionalDataSize += a.GetCount() * sizeof(T);
  }
}

// static
template <typename T, ezUInt32 Size>
void ezVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::AddAdditionalDataSize(ezUInt32 uiSize, ezUInt32 uiAlignment, ezUInt32& inout_additionalDataSize)
{
  if (uiSize > Size * sizeof(T))
  {
    inout_additionalDataSize = ezMemoryUtils::AlignSize<ezUInt32>(inout_additionalDataSize, uiAlignment);
    inout_additionalDataSize += uiSize;
  }
}

template <typename T, ezUInt32 Size>
T* ezVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::Init(ezUInt8 uiCount, ezUInt8*& inout_pAdditionalData)
{
  if (uiCount <= Size)
  {
    return m_Embedded;
  }

  inout_pAdditionalData = ezMemoryUtils::AlignForwards(inout_pAdditionalData, EZ_ALIGNMENT_OF(T));
  inout_pAdditionalData += uiCount * sizeof(T);

  m_Ptr = reinterpret_cast<T*>(inout_pAdditionalData);
  return m_Ptr;
}

template <typename T, ezUInt32 Size>
ezResult ezVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::ReadFromStream(ezUInt8& out_uiCount, ezStreamReader& inout_stream, ezUInt8*& inout_pAdditionalData)
{
  ezUInt16 uiCount = 0;
  inout_stream >> uiCount;

  if (uiCount > ezMath::MaxValue<ezUInt8>())
  {
    return EZ_FAILURE;
  }
  out_uiCount = static_cast<ezUInt8>(uiCount);

  T* pTargetPtr = Init(out_uiCount, inout_pAdditionalData);
  const ezUInt64 uiNumBytesToRead = uiCount * sizeof(T);
  if (inout_stream.ReadBytes(pTargetPtr, uiNumBytesToRead) != uiNumBytesToRead)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

EZ_ALWAYS_INLINE ezUInt32 ezVisualScriptGraphDescription::Node::GetExecutionIndex(ezUInt32 uiSlot) const
{
  if (uiSlot < m_NumExecutionIndices)
  {
    return m_NumExecutionIndices <= EZ_ARRAY_SIZE(m_ExecutionIndices.m_Embedded) ? m_ExecutionIndices.m_Embedded[uiSlot] : m_ExecutionIndices.m_Ptr[uiSlot];
  }

  return ezInvalidIndex;
}

EZ_ALWAYS_INLINE ezVisualScriptGraphDescription::DataOffset ezVisualScriptGraphDescription::Node::GetInputDataOffset(ezUInt32 uiSlot) const
{
  if (uiSlot < m_NumInputDataOffsets)
  {
    return m_NumInputDataOffsets <= EZ_ARRAY_SIZE(m_InputDataOffsets.m_Embedded) ? m_InputDataOffsets.m_Embedded[uiSlot] : m_InputDataOffsets.m_Ptr[uiSlot];
  }

  return {};
}

EZ_ALWAYS_INLINE ezVisualScriptGraphDescription::DataOffset ezVisualScriptGraphDescription::Node::GetOutputDataOffset(ezUInt32 uiSlot) const
{
  if (uiSlot < m_NumOutputDataOffsets)
  {
    return m_NumOutputDataOffsets <= EZ_ARRAY_SIZE(m_OutputDataOffsets.m_Embedded) ? m_OutputDataOffsets.m_Embedded[uiSlot] : m_OutputDataOffsets.m_Ptr[uiSlot];
  }

  return {};
}

template <typename T>
EZ_ALWAYS_INLINE const T& ezVisualScriptGraphDescription::Node::GetUserData() const
{
  EZ_ASSERT_DEBUG(m_UserDataByteSize == sizeof(T), "Invalid data");
  return *reinterpret_cast<const T*>(m_UserDataByteSize <= sizeof(m_UserData.m_Embedded) ? m_UserData.m_Embedded : m_UserData.m_Ptr);
}

template <typename T>
void ezVisualScriptGraphDescription::Node::SetUserData(const T& data, ezUInt8*& inout_pAdditionalData)
{
  m_UserDataByteSize = sizeof(T);
  auto pUserData = m_UserData.Init(m_UserDataByteSize / sizeof(ezUInt32), inout_pAdditionalData);
  EZ_CHECK_ALIGNMENT(pUserData, EZ_ALIGNMENT_OF(T));
  *reinterpret_cast<T*>(pUserData) = data;
}

//////////////////////////////////////////////////////////////////////////

EZ_ALWAYS_INLINE const ezVisualScriptGraphDescription::Node* ezVisualScriptGraphDescription::GetNode(ezUInt32 uiIndex)
{
  return uiIndex < m_Nodes.GetCount() ? &m_Nodes.GetPtr()[uiIndex] : nullptr;
}

//////////////////////////////////////////////////////////////////////////

EZ_FORCE_INLINE void ezVisualScriptDataDescription::CheckOffset(DataOffset dataOffset, const ezRTTI* pType) const
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  auto expectedDataType = static_cast<ezVisualScriptDataType::Enum>(dataOffset.m_uiDataType);
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

EZ_FORCE_INLINE ezVisualScriptDataDescription::DataOffset ezVisualScriptDataDescription::GetOffset(ezVisualScriptDataType::Enum dataType, ezUInt32 uiIndex, bool bIsConstant) const
{
  auto& offsetAndCount = m_PerTypeInfo[dataType];
  ezUInt32 uiByteOffset = ezInvalidIndex;
  if (uiIndex < offsetAndCount.m_uiCount)
  {
    uiByteOffset = offsetAndCount.m_uiStartOffset + uiIndex * ezVisualScriptDataType::GetStorageSize(dataType);
  }

  return DataOffset(uiByteOffset, dataType, bIsConstant);
}

//////////////////////////////////////////////////////////////////////////

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
      EZ_ASSERT_DEBUG(pType->IsDerivedFrom<ezComponent>() == false, "Component type '{}' is stored as typed pointer, cast to ezComponent first to ensure correct storage", pType->GetTypeName());

      m_pDesc->CheckOffset(dataOffset, pType);

      auto& typedPointer = *reinterpret_cast<ezTypedPointer*>(pData);
      typedPointer.m_pObject = ptr;
      typedPointer.m_pType = pType;
    }
  }
}
