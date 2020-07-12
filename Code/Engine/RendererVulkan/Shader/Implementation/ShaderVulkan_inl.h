
vk::ShaderModule ezGALShaderVulkan::GetDXVertexShader() const
{
  return m_pVertexShader;
}

vk::ShaderModule ezGALShaderVulkan::GetDXHullShader() const
{
  return m_pHullShader;
}

vk::ShaderModule ezGALShaderVulkan::GetDXDomainShader() const
{
  return m_pDomainShader;
}

vk::ShaderModule ezGALShaderVulkan::GetDXGeometryShader() const
{
  return m_pGeometryShader;
}

vk::ShaderModule ezGALShaderVulkan::GetDXPixelShader() const
{
  return m_pPixelShader;
}

vk::ShaderModule ezGALShaderVulkan::GetDXComputeShader() const
{
  return m_pComputeShader;
}
