

ID3D11InputLayout* ezGALVertexDeclarationDX11::GetDXInputLayout() const
{
  return m_pDXInputLayout;
}

ezArrayPtr<const ezUInt32> ezGALVertexDeclarationDX11::GetVertexBufferStrides() const
{
  return m_VertexBufferStrides;
}
