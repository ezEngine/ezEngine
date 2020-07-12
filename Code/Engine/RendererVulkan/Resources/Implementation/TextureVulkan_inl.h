

ID3D11Resource* ezGALTextureVulkan::GetDXTexture() const
{
	return m_pDXTexture;
}

ID3D11Resource* ezGALTextureVulkan::GetDXStagingTexture() const
{
  return m_pDXStagingTexture;
}