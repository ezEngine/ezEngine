
vk::ShaderModule ezGALShaderVulkan::GetShader(ezGALShaderStage::Enum stage) const
{
  return m_Shaders[stage];
}

const ezGALShaderVulkan::DescriptorSetLayoutDesc& ezGALShaderVulkan::GetDescriptorSetLayout() const
{
  return m_descriptorSetLayoutDesc;
}

const ezArrayPtr<const ezGALShaderVulkan::BindingMapping> ezGALShaderVulkan::GetBindingMapping() const
{
  return m_BindingMapping;
}

const ezArrayPtr<const ezGALShaderVulkan::VertexInputAttribute> ezGALShaderVulkan::GetVertexInputAttributes() const
{
  return m_VertexInputAttributes;
}
