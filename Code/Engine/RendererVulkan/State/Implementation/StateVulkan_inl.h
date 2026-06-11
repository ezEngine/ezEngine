
EZ_ALWAYS_INLINE const vk::PipelineColorBlendStateCreateInfo* ezGALBlendStateVulkan::GetBlendState() const
{
  return &m_BlendState;
}

EZ_ALWAYS_INLINE const vk::PipelineDepthStencilStateCreateInfo* ezGALDepthStencilStateVulkan::GetDepthStencilState() const
{
  return &m_DepthStencilState;
}

EZ_ALWAYS_INLINE const vk::PipelineRasterizationStateCreateInfo* ezGALRasterizerStateVulkan::GetRasterizerState() const
{
  return &m_RasterizerState;
}

EZ_ALWAYS_INLINE const vk::DescriptorImageInfo& ezGALSamplerStateVulkan::GetImageInfo() const
{
  return m_ResourceImageInfo;
}
