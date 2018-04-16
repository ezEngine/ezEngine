
EZ_ALWAYS_INLINE const ezExpressionByteCode::StorageType* ezExpressionByteCode::GetByteCode() const
{
  return m_ByteCode.GetData();
}

EZ_ALWAYS_INLINE const ezExpressionByteCode::StorageType* ezExpressionByteCode::GetByteCodeEnd() const
{
  return m_ByteCode.GetData() + m_ByteCode.GetCount();
}

EZ_ALWAYS_INLINE ezUInt32 ezExpressionByteCode::GetNumInputRegisters() const
{
  return m_uiNumInputRegisters;
}

EZ_ALWAYS_INLINE ezUInt32 ezExpressionByteCode::GetNumTempRegisters() const
{
  return m_uiNumTempRegisters;
}

// static
EZ_ALWAYS_INLINE ezExpressionByteCode::OpCode::Enum ezExpressionByteCode::GetOpCode(const StorageType*& pByteCode)
{
  ezUInt32 uiOpCode = *pByteCode;
  ++pByteCode;
  return static_cast<OpCode::Enum>(uiOpCode);
}

// static
EZ_ALWAYS_INLINE ezUInt32 ezExpressionByteCode::GetRegisterIndex(const StorageType*& pByteCode, ezUInt32 uiNumRegisters)
{
  ezUInt32 uiIndex = *pByteCode * uiNumRegisters;
  ++pByteCode;
  return uiIndex;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezExpressionByteCode::GetConstant(const StorageType*& pByteCode)
{
  float c = *reinterpret_cast<const float*>(pByteCode);
  ++pByteCode;
  return ezSimdVec4f(c);
}
