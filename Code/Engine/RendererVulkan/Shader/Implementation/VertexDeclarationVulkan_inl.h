

ezArrayPtr<const vk::VertexInputAttributeDescription> ezGALVertexDeclarationVulkan::GetAttributes() const
{
  return m_attributes.GetArrayPtr();
}

ezArrayPtr<const vk::VertexInputBindingDescription> ezGALVertexDeclarationVulkan::GetBindings() const
{
  return m_bindings.GetArrayPtr();
}
