
const vk::DescriptorSetLayoutBinding& ezGALResourceViewVulkan::GetResourceBinding() const
{
  return m_resourceBinding;
}

const vk::WriteDescriptorSet& ezGALResourceViewVulkan::GetResourceBindingData() const
{
  return m_resourceBindingData;
}
