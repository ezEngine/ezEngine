#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/Shader/Shader.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>

#include <d3d11.h>

ezGALVertexDeclarationVulkan::ezGALVertexDeclarationVulkan(const ezGALVertexDeclarationCreationDescription& Description)
  : ezGALVertexDeclaration(Description)
{
}

ezGALVertexDeclarationVulkan::~ezGALVertexDeclarationVulkan() = default;

ezResult ezGALVertexDeclarationVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  const ezGALShader* pShader = pDevice->GetShader(m_Description.m_hShader);

  if (pShader == nullptr || !pShader->GetDescription().HasByteCodeForStage(ezGALShaderStage::VertexShader))
  {
    return EZ_FAILURE;
  }

  // Copy attribute descriptions
  ezUInt32 usedBindings = 0;
  for (ezUInt32 i = 0; i < m_Description.m_VertexAttributes.GetCount(); i++)
  {
    const ezGALVertexAttribute& Current = m_Description.m_VertexAttributes[i];

    vk::VertexInputAttributeDescription& attrib = m_attributes.ExpandAndGetRef();
    attrib.binding = Current.m_uiVertexBufferSlot;
    attrib.location = i;
    attrib.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType;
    attrib.offset = Current.m_uiOffset;

    if (attrib.format == vk::Format::eUndefined)
    {
      ezLog::Error("Vertex attribute format {0} of attribute at index {1} is undefined!", Current.m_eFormat, i);
      return EZ_FAILURE;
    }

    usedBindings |= EZ_BIT(Current.m_uiVertexBufferSlot);
    if (Current.m_uiVertexBufferSlot >= m_bindings.GetCount())
    {
      m_bindings.SetCount(Current.m_uiVertexBufferSlot + 1);
    }
    vk::VertexInputBindingDescription& binding = m_bindings[Current.m_uiVertexBufferSlot];
    binding.binding = Current.m_uiVertexBufferSlot;
    binding.stride = 0;
    binding.inputRate = Current.m_bInstanceData ? vk::VertexInputRate::eInstance : vk::VertexInputRate::eVertex;
  }
  for (ezInt32 i = (ezInt32)m_bindings.GetCount() - 1; i >= 0; --i)
  {
    if ((usedBindings & EZ_BIT(i)) == 0)
    {
      m_bindings.RemoveAtAndCopy(i);
    }
  }

  return EZ_SUCCESS;
}

ezResult ezGALVertexDeclarationVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Shader_Implementation_VertexDeclarationVulkan);
