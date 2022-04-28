
vk::ShaderModule ezGALShaderVulkan::GetVertexShader() const
{
  return m_pVertexShader;
}

vk::ShaderModule ezGALShaderVulkan::GetHullShader() const
{
  return m_pHullShader;
}

vk::ShaderModule ezGALShaderVulkan::GetDomainShader() const
{
  return m_pDomainShader;
}

vk::ShaderModule ezGALShaderVulkan::GetGeometryShader() const
{
  return m_pGeometryShader;
}

vk::ShaderModule ezGALShaderVulkan::GetPixelShader() const
{
  return m_pPixelShader;
}

vk::ShaderModule ezGALShaderVulkan::GetComputeShader() const
{
  return m_pComputeShader;
}
