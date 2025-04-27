
vk::ShaderModule ezGALShaderVulkan::GetShader(ezGALShaderStage::Enum stage) const
{
  return m_Shaders[stage];
}

vk::PipelineLayout ezGALShaderVulkan::GetPipelineLayout() const
{
  return m_PipelineLayout;
}

ezUInt32 ezGALShaderVulkan::GetSetCount() const
{
  return m_SetBindings.GetCount();
}

vk::DescriptorSetLayout ezGALShaderVulkan::GetDescriptorSetLayout(ezUInt32 uiSet) const
{
  EZ_ASSERT_DEBUG(uiSet < m_descriptorSetLayout.GetCount(), "Set index out of range.");
  return m_descriptorSetLayout[uiSet];
}

ezArrayPtr<const ezShaderResourceBinding> ezGALShaderVulkan::GetBindings(ezUInt32 uiSet) const
{
  EZ_ASSERT_DEBUG(uiSet < m_SetBindings.GetCount(), "Set index out of range.");
  return m_SetBindings[uiSet].GetArrayPtr();
}

vk::PushConstantRange ezGALShaderVulkan::GetPushConstantRange() const
{
  return m_pushConstants;
}
