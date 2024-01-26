
EZ_ALWAYS_INLINE const ezExpressionByteCode::StorageType* ezExpressionByteCode::GetByteCodeStart() const
{
  return m_pByteCode;
}

EZ_ALWAYS_INLINE const ezExpressionByteCode::StorageType* ezExpressionByteCode::GetByteCodeEnd() const
{
  return m_pByteCode + m_uiByteCodeCount;
}

EZ_ALWAYS_INLINE ezArrayPtr<const ezExpressionByteCode::StorageType> ezExpressionByteCode::GetByteCode() const
{
  return ezMakeArrayPtr(m_pByteCode, m_uiByteCodeCount);
}

EZ_ALWAYS_INLINE ezUInt32 ezExpressionByteCode::GetNumInstructions() const
{
  return m_uiNumInstructions;
}

EZ_ALWAYS_INLINE ezUInt32 ezExpressionByteCode::GetNumTempRegisters() const
{
  return m_uiNumTempRegisters;
}

EZ_ALWAYS_INLINE ezArrayPtr<const ezExpression::StreamDesc> ezExpressionByteCode::GetInputs() const
{
  return ezMakeArrayPtr(m_pInputs, m_uiNumInputs);
}

EZ_ALWAYS_INLINE ezArrayPtr<const ezExpression::StreamDesc> ezExpressionByteCode::GetOutputs() const
{
  return ezMakeArrayPtr(m_pOutputs, m_uiNumOutputs);
}

EZ_ALWAYS_INLINE ezArrayPtr<const ezExpression::FunctionDesc> ezExpressionByteCode::GetFunctions() const
{
  return ezMakeArrayPtr(m_pFunctions, m_uiNumFunctions);
}

// static
EZ_ALWAYS_INLINE ezExpressionByteCode::OpCode::Enum ezExpressionByteCode::GetOpCode(const StorageType*& ref_pByteCode)
{
  ezUInt32 uiOpCode = *ref_pByteCode;
  ++ref_pByteCode;
  return static_cast<OpCode::Enum>((uiOpCode >= 0 && uiOpCode < OpCode::Count) ? uiOpCode : 0);
}

// static
EZ_ALWAYS_INLINE ezUInt32 ezExpressionByteCode::GetRegisterIndex(const StorageType*& ref_pByteCode)
{
  ezUInt32 uiIndex = *ref_pByteCode;
  ++ref_pByteCode;
  return uiIndex;
}

// static
EZ_ALWAYS_INLINE ezExpression::Register ezExpressionByteCode::GetConstant(const StorageType*& ref_pByteCode)
{
  ezExpression::Register r;
  r.i = ezSimdVec4i(*ref_pByteCode);
  ++ref_pByteCode;
  return r;
}

// static
EZ_ALWAYS_INLINE ezUInt32 ezExpressionByteCode::GetFunctionIndex(const StorageType*& ref_pByteCode)
{
  ezUInt32 uiIndex = *ref_pByteCode;
  ++ref_pByteCode;
  return uiIndex;
}

// static
EZ_ALWAYS_INLINE ezUInt32 ezExpressionByteCode::GetFunctionArgCount(const StorageType*& ref_pByteCode)
{
  ezUInt32 uiArgCount = *ref_pByteCode;
  ++ref_pByteCode;
  return uiArgCount;
}
