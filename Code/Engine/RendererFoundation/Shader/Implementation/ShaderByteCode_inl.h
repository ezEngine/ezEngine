

const void* ezGALShaderByteCode::GetByteCode() const
{
  return m_pSource.GetPtr();
}

ezUInt32 ezGALShaderByteCode::GetSize() const
{
  return m_pSource.GetCount();
}

bool ezGALShaderByteCode::IsValid() const
{
  return GetByteCode() != nullptr && GetSize() != 0;
}