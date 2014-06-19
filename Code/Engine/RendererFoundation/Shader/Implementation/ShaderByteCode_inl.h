

const void* ezGALShaderByteCode::GetByteCode() const
{
  if (m_Source.IsEmpty())
    return nullptr;

  return &m_Source[0];
}

ezUInt32 ezGALShaderByteCode::GetSize() const
{
  return m_Source.GetCount();
}

bool ezGALShaderByteCode::IsValid() const
{
  return !m_Source.IsEmpty();
}