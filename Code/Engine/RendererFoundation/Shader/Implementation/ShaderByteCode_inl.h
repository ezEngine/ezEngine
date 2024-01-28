

const void* ezGALShaderByteCode::GetByteCode() const
{
  if (m_ByteCode.IsEmpty())
    return nullptr;

  return &m_ByteCode[0];
}

ezUInt32 ezGALShaderByteCode::GetSize() const
{
  return m_ByteCode.GetCount();
}

bool ezGALShaderByteCode::IsValid() const
{
  return !m_ByteCode.IsEmpty();
}
