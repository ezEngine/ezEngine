
template <typename T>
EZ_FORCE_INLINE const T& ezVisualScriptInstance::GetData(DataOffset dataOffset) const
{
  if (dataOffset.m_uiIsConstant)
  {
    return m_pConstantDataStorage->GetData<T>(dataOffset);
  }

  return m_pVariableDataStorage->GetData<T>(dataOffset);
}

template <typename T>
EZ_FORCE_INLINE T& ezVisualScriptInstance::GetWritableData(DataOffset dataOffset)
{
  EZ_ASSERT_DEBUG(dataOffset.m_uiIsConstant == 0, "Can't write to constant data");
  return m_pVariableDataStorage->GetWritableData<T>(dataOffset);
}

template <typename T>
EZ_FORCE_INLINE void ezVisualScriptInstance::SetData(DataOffset dataOffset, const T& value)
{
  EZ_ASSERT_DEBUG(dataOffset.m_uiIsConstant == 0, "Outputs can't set constant data");
  return m_pVariableDataStorage->SetData<T>(dataOffset, value);
}

EZ_FORCE_INLINE ezTypedPointer ezVisualScriptInstance::GetPointerData(DataOffset dataOffset)
{
  EZ_ASSERT_DEBUG(dataOffset.m_uiIsConstant == 0, "Pointers can't be constant data");
  return m_pVariableDataStorage->GetPointerData(dataOffset, m_uiExecutionCounter);
}

template <typename T>
EZ_FORCE_INLINE void ezVisualScriptInstance::SetPointerData(DataOffset dataOffset, T ptr, const ezRTTI* pType)
{
  EZ_ASSERT_DEBUG(dataOffset.m_uiIsConstant == 0, "Pointers can't be constant data");
  m_pVariableDataStorage->SetPointerData(dataOffset, ptr, pType, m_uiExecutionCounter);
}

EZ_FORCE_INLINE ezVariant ezVisualScriptInstance::GetDataAsVariant(DataOffset dataOffset, ezVariantType::Enum expectedType) const
{
  if (dataOffset.m_uiIsConstant)
  {
    return m_pConstantDataStorage->GetDataAsVariant(dataOffset, expectedType, m_uiExecutionCounter);
  }

  return m_pVariableDataStorage->GetDataAsVariant(dataOffset, expectedType, m_uiExecutionCounter);
}

EZ_FORCE_INLINE void ezVisualScriptInstance::SetDataFromVariant(DataOffset dataOffset, const ezVariant& value)
{
  EZ_ASSERT_DEBUG(dataOffset.m_uiIsConstant == 0, "Outputs can't set constant data");
  return m_pVariableDataStorage->SetDataFromVariant(dataOffset, value, m_uiExecutionCounter);
}
