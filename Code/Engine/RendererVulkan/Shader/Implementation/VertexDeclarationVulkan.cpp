#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/Shader/Shader.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

ezGALVertexDeclarationVulkan::ezGALVertexDeclarationVulkan(const ezGALVertexDeclarationCreationDescription& Description)
  : ezGALVertexDeclaration(Description)
{
}

ezGALVertexDeclarationVulkan::~ezGALVertexDeclarationVulkan() = default;

ezResult ezGALVertexDeclarationVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  const ezGALShaderVulkan* pShader = static_cast<const ezGALShaderVulkan*>(pDevice->GetShader(m_Description.m_hShader));

  if (pShader == nullptr || !pShader->GetDescription().HasByteCodeForStage(ezGALShaderStage::VertexShader))
  {
    return EZ_FAILURE;
  }

  ezHybridArray<ezShaderVertexInputAttribute, 8> vias(pShader->GetVertexInputAttributes());
  auto FindLocation = [&](ezGALVertexAttributeSemantic::Enum sematic, ezGALResourceFormat::Enum format) -> ezUInt32
  {
    for (ezUInt32 i = 0; i < vias.GetCount(); i++)
    {
      if (vias[i].m_eSemantic == sematic)
      {
        // EZ_ASSERT_DEBUG(vias[i].m_eFormat == format, "Found matching sematic {} but format differs: {} : {}", sematic, format, vias[i].m_eFormat);
        ezUInt32 uiLocation = vias[i].m_uiLocation;
        vias.RemoveAtAndSwap(i);
        return uiLocation;
      }
    }
    return ezMath::MaxValue<ezUInt32>();
  };

  // Copy attribute descriptions
  ezUInt32 usedBindings = 0;
  for (ezUInt32 i = 0; i < m_Description.m_VertexAttributes.GetCount(); i++)
  {
    const ezGALVertexAttribute& Current = m_Description.m_VertexAttributes[i];

    const ezUInt32 uiLocation = FindLocation(Current.m_eSemantic, Current.m_eFormat);
    if (uiLocation == ezMath::MaxValue<ezUInt32>())
    {
      // ezLog::Warning("Vertex buffer semantic {} not used by shader", Current.m_eSemantic);
      continue;
    }
    vk::VertexInputAttributeDescription& attrib = m_attributes.ExpandAndGetRef();
    attrib.binding = Current.m_uiVertexBufferSlot;
    attrib.location = uiLocation;
    attrib.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_format;
    attrib.offset = Current.m_uiOffset;

    if (attrib.format == vk::Format::eUndefined)
    {
      ezLog::Error("Vertex attribute format {0} of attribute at index {1} is undefined!", Current.m_eFormat, i);
      return EZ_FAILURE;
    }

    usedBindings |= EZ_BIT(Current.m_uiVertexBufferSlot);
  }

  const ezUInt32 uiBindings = m_Description.m_VertexBindings.GetCount();
  m_bindings.SetCount(uiBindings);
  for (ezUInt32 uiBinding = 0; uiBinding < uiBindings; ++uiBinding)
  {
    const ezGALVertexBinding& binding = m_Description.m_VertexBindings[uiBinding];
    vk::VertexInputBindingDescription& vkBinding = m_bindings[uiBinding];
    vkBinding.binding = uiBinding;
    vkBinding.stride = binding.m_uiStride;
    vkBinding.inputRate = ezConversionUtilsVulkan::GetVertexBindingRate(binding.m_Rate);
  }

  // Remove unused vertex bindings.
  for (ezInt32 i = (ezInt32)m_bindings.GetCount() - 1; i >= 0; --i)
  {
    if ((usedBindings & EZ_BIT(i)) == 0)
    {
      m_bindings.RemoveAtAndCopy(i);
    }
  }

  m_createInfo.pVertexAttributeDescriptions = m_attributes.GetData();
  m_createInfo.vertexAttributeDescriptionCount = m_attributes.GetCount();
  m_createInfo.pVertexBindingDescriptions = m_bindings.GetData();
  m_createInfo.vertexBindingDescriptionCount = m_bindings.GetCount();

  if (!vias.IsEmpty())
  {
    ezLog::Error("Vertex buffers do not cover all vertex attributes defined in the shader!");
    return EZ_FAILURE;
  }
  return EZ_SUCCESS;
}

ezResult ezGALVertexDeclarationVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}
