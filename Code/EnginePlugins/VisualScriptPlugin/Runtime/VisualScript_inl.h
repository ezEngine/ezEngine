
// static
template <typename T, ezUInt32 Size>
void ezVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::AddAdditionalDataSize(ezArrayPtr<const T> a, ezUInt32& inout_uiAdditionalDataSize)
{
  if (a.GetCount() > Size)
  {
    inout_uiAdditionalDataSize = ezMemoryUtils::AlignSize<ezUInt32>(inout_uiAdditionalDataSize, alignof(T));
    inout_uiAdditionalDataSize += a.GetCount() * sizeof(T);
  }
}

// static
template <typename T, ezUInt32 Size>
void ezVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::AddAdditionalDataSize(ezUInt32 uiSize, ezUInt32 uiAlignment, ezUInt32& inout_uiAdditionalDataSize)
{
  if (uiSize > Size * sizeof(T))
  {
    inout_uiAdditionalDataSize = ezMemoryUtils::AlignSize<ezUInt32>(inout_uiAdditionalDataSize, uiAlignment);
    inout_uiAdditionalDataSize += uiSize;
  }
}

template <typename T, ezUInt32 Size>
T* ezVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::Init(ezUInt8 uiCount, ezUInt32 uiAlignment, ezUInt8*& inout_pAdditionalData)
{
  if (uiCount <= Size)
  {
    return m_Embedded;
  }

  inout_pAdditionalData = ezMemoryUtils::AlignForwards(inout_pAdditionalData, uiAlignment);
  m_Ptr = reinterpret_cast<T*>(inout_pAdditionalData);
  inout_pAdditionalData += uiCount * sizeof(T);
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

  T* pTargetPtr = Init(out_uiCount, alignof(T), inout_pAdditionalData);
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

EZ_ALWAYS_INLINE ezVisualScriptGraphDescription::DataOffset* ezVisualScriptGraphDescription::Node::GetInputDataOffsets()
{
  return m_NumInputDataOffsets <= EZ_ARRAY_SIZE(m_InputDataOffsets.m_Embedded) ? m_InputDataOffsets.m_Embedded : m_InputDataOffsets.m_Ptr;
}

EZ_ALWAYS_INLINE ezVisualScriptGraphDescription::DataOffset* ezVisualScriptGraphDescription::Node::GetOutputDataOffsets()
{
  return m_NumOutputDataOffsets <= EZ_ARRAY_SIZE(m_OutputDataOffsets.m_Embedded) ? m_OutputDataOffsets.m_Embedded : m_OutputDataOffsets.m_Ptr;
}

// static
template <typename T>
EZ_ALWAYS_INLINE constexpr ezUInt32 ezVisualScriptGraphDescription::Node::GetUserDataAlignment()
{
  // Ensures at least 8 byte alignment, thus making it compatible between 32 and 64 bit platforms.
  return ezMath::Max<ezUInt32>(alignof(T), 8u);
}

template <typename T>
EZ_ALWAYS_INLINE const T& ezVisualScriptGraphDescription::Node::GetUserData() const
{
  EZ_ASSERT_DEBUG(m_UserDataByteSize >= sizeof(T), "Invalid data");
  return *reinterpret_cast<const T*>(m_UserDataByteSize <= sizeof(m_UserData.m_Embedded) ? m_UserData.m_Embedded : m_UserData.m_Ptr);
}

template <typename T>
T& ezVisualScriptGraphDescription::Node::InitUserData(ezUInt8*& inout_pAdditionalData, ezUInt32 uiByteSize /*= sizeof(T)*/, ezUInt32 uiAlignment /*= GetUserDataAlignment<T>()*/)
{
  m_UserDataByteSize = uiByteSize;
  const ezUInt32 uiUserDataCount = uiByteSize / sizeof(ezUInt32);
  EZ_ASSERT_DEBUG(uiUserDataCount <= ezMath::MaxValue<ezUInt8>(), "User data is too big");
  auto pUserData = m_UserData.Init(static_cast<ezUInt8>(uiUserDataCount), uiAlignment, inout_pAdditionalData);
  EZ_CHECK_ALIGNMENT(pUserData, uiAlignment);
  return *reinterpret_cast<T*>(pUserData);
}

//////////////////////////////////////////////////////////////////////////

EZ_ALWAYS_INLINE const ezVisualScriptGraphDescription::Node* ezVisualScriptGraphDescription::GetNode(ezUInt32 uiIndex) const
{
  return uiIndex < m_Nodes.GetCount() ? &m_Nodes.GetPtr()[uiIndex] : nullptr;
}

EZ_ALWAYS_INLINE bool ezVisualScriptGraphDescription::IsCoroutine() const
{
  auto entryNodeType = GetNode(0)->m_Type;
  return entryNodeType == ezVisualScriptNodeDescription::Type::EntryCall_Coroutine || entryNodeType == ezVisualScriptNodeDescription::Type::MessageHandler_Coroutine;
}

EZ_ALWAYS_INLINE const ezSharedPtr<const ezVisualScriptDataDescription>& ezVisualScriptGraphDescription::GetLocalDataDesc() const
{
  return m_pLocalDataDesc;
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
EZ_FORCE_INLINE const T& ezVisualScriptExecutionContext::GetData(DataOffset dataOffset) const
{
  return m_DataStorage[dataOffset.m_uiSource]->GetData<T>(dataOffset);
}

template <typename T>
EZ_FORCE_INLINE T& ezVisualScriptExecutionContext::GetWritableData(DataOffset dataOffset)
{
  EZ_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Can't write to constant data");
  return m_DataStorage[dataOffset.m_uiSource]->GetWritableData<T>(dataOffset);
}

template <typename T>
EZ_FORCE_INLINE void ezVisualScriptExecutionContext::SetData(DataOffset dataOffset, const T& value)
{
  EZ_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Outputs can't set constant data");
  return m_DataStorage[dataOffset.m_uiSource]->SetData<T>(dataOffset, value);
}

EZ_FORCE_INLINE ezTypedPointer ezVisualScriptExecutionContext::GetPointerData(DataOffset dataOffset)
{
  EZ_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Pointers can't be constant data");
  return m_DataStorage[dataOffset.m_uiSource]->GetPointerData(dataOffset, m_uiExecutionCounter);
}

template <typename T>
EZ_FORCE_INLINE void ezVisualScriptExecutionContext::SetPointerData(DataOffset dataOffset, T ptr, const ezRTTI* pType)
{
  EZ_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Pointers can't be constant data");
  m_DataStorage[dataOffset.m_uiSource]->SetPointerData(dataOffset, ptr, pType, m_uiExecutionCounter);
}

EZ_FORCE_INLINE ezVariant ezVisualScriptExecutionContext::GetDataAsVariant(DataOffset dataOffset, const ezRTTI* pExpectedType) const
{
  return m_DataStorage[dataOffset.m_uiSource]->GetDataAsVariant(dataOffset, pExpectedType, m_uiExecutionCounter);
}

EZ_FORCE_INLINE void ezVisualScriptExecutionContext::SetDataFromVariant(DataOffset dataOffset, const ezVariant& value)
{
  EZ_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Outputs can't set constant data");
  return m_DataStorage[dataOffset.m_uiSource]->SetDataFromVariant(dataOffset, value, m_uiExecutionCounter);
}

EZ_ALWAYS_INLINE void ezVisualScriptExecutionContext::SetCurrentCoroutine(ezScriptCoroutine* pCoroutine)
{
  m_pCurrentCoroutine = pCoroutine;
}

inline ezTime ezVisualScriptExecutionContext::GetDeltaTimeSinceLastExecution()
{
  EZ_ASSERT_DEBUG(m_pDesc->IsCoroutine(), "Delta time is only valid for coroutines");
  return m_DeltaTimeSinceLastExecution;
}
