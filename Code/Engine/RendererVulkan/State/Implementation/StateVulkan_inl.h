
EZ_ALWAYS_INLINE const vk::PipelineColorBlendStateCreateInfo* ezGALBlendStateVulkan::GetBlendState() const
{
  return &m_blendState;
}

EZ_ALWAYS_INLINE const vk::PipelineDepthStencilStateCreateInfo* ezGALDepthStencilStateVulkan::GetDepthStencilState() const
{
  return &m_depthStencilState;
}

EZ_ALWAYS_INLINE const vk::PipelineRasterizationStateCreateInfo* ezGALRasterizerStateVulkan::GetRasterizerState() const
{
  return &m_rasterizerState;
}

EZ_ALWAYS_INLINE const vk::DescriptorSetLayoutBinding* ezGALSamplerStateVulkan::GetSamplerState() const
{
  return &m_samplerState;
}
