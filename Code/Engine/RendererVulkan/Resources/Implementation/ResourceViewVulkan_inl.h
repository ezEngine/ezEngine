
vk::DescriptorSetLayoutBinding& ezGALResourceViewVulkan::GetResourceBinding() const
{
  return &m_resourceBinding;
}

vk::WriteDescriptorSet& ezGALResourceViewVulkan::GetResourceBindingData() const
{
  return &m_resourceBinding;
}
