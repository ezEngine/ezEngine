

ID3D11Resource* ezGALTextureDX11::GetDXTexture() const
{
	return m_pDXTexture;
}

ID3D11Resource* ezGALTextureDX11::GetDXStagingTexture() const
{
  return m_pDXStagingTexture;
}