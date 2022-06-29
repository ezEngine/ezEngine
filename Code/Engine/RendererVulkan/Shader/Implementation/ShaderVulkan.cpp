#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <ShaderCompilerDXC/SpirvMetaData.h>

EZ_CHECK_AT_COMPILETIME(ezVulkanDescriptorSetLayoutBinding::ConstantBuffer == ezGALShaderVulkan::BindingMapping::ConstantBuffer);
EZ_CHECK_AT_COMPILETIME(ezVulkanDescriptorSetLayoutBinding::ResourceView == ezGALShaderVulkan::BindingMapping::ResourceView);
EZ_CHECK_AT_COMPILETIME(ezVulkanDescriptorSetLayoutBinding::UAV == ezGALShaderVulkan::BindingMapping::UAV);
EZ_CHECK_AT_COMPILETIME(ezVulkanDescriptorSetLayoutBinding::Sampler == ezGALShaderVulkan::BindingMapping::Sampler);

void ezGALShaderVulkan::DescriptorSetLayoutDesc::ComputeHash()
{
  ezHashStreamWriter32 writer;
  const ezUInt32 uiSize = m_bindings.GetCount();
  for (ezUInt32 i = 0; i < uiSize; i++)
  {
    const auto& binding = m_bindings[i];
    writer << binding.binding;
    writer << ezConversionUtilsVulkan::GetUnderlyingValue(binding.descriptorType);
    writer << binding.descriptorCount;
    writer << ezConversionUtilsVulkan::GetUnderlyingFlagsValue(binding.stageFlags);
    writer << binding.pImmutableSamplers;
  }
  m_uiHash = writer.GetHashValue();
}

ezGALShaderVulkan::ezGALShaderVulkan(const ezGALShaderCreationDescription& Description)
  : ezGALShader(Description)
{
}

ezGALShaderVulkan::~ezGALShaderVulkan() {}

void ezGALShaderVulkan::SetDebugName(const char* szName) const
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(ezGALDevice::GetDefaultDevice());
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; i++)
  {
    pVulkanDevice->SetDebugName(szName, m_Shaders[i]);
  }
}

ezResult ezGALShaderVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  // Extract meta data and shader code.
  ezArrayPtr<const ezUInt8> shaderCode[ezGALShaderStage::ENUM_COUNT];
  ezDynamicArray<ezVulkanDescriptorSetLayout> sets[ezGALShaderStage::ENUM_COUNT];
  ezHybridArray<ezVulkanVertexInputAttribute, 8> vertexInputAttributes;

  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; i++)
  {
    if (m_Description.HasByteCodeForStage((ezGALShaderStage::Enum)i))
    {
      ezArrayPtr<const ezUInt8> metaData(reinterpret_cast<const ezUInt8*>(m_Description.m_ByteCodes[i]->GetByteCode()), m_Description.m_ByteCodes[i]->GetSize());
      // Only the vertex shader stores vertexInputAttributes, so passing in the array into other shaders is just a no op.
      ezSpirvMetaData::Read(metaData, shaderCode[i], sets[i], vertexInputAttributes);
    }
  }

  // For now the meta data and what the shader exposes is the exact same data but this might change so different types are used.
  for (ezVulkanVertexInputAttribute& via : vertexInputAttributes)
  {
    m_VertexInputAttributes.PushBack({via.m_eSemantic, via.m_uiLocation, via.m_eFormat});
  }

  // Compute remapping.
  // Each shader stage is compiled individually and has its own binding indices.
  // In Vulkan we need to map all stages into one descriptor layout which requires us to remap some shader stages so no binding index conflicts appear.
  struct ShaderRemapping
  {
    const ezVulkanDescriptorSetLayoutBinding* pBinding = 0;
    ezUInt16 m_uiTarget = 0; ///< The new binding target that pBinding needs to be remapped to.
  };
  struct LayoutBinding
  {
    const ezVulkanDescriptorSetLayoutBinding* m_binding = nullptr; ///< The first binding under which this resource was encountered.
    vk::ShaderStageFlags m_stages = {};                            ///< Bitflags of all stages that share this binding. Matching is done by name.
  };
  ezHybridArray<ShaderRemapping, 6> remappings[ezGALShaderStage::ENUM_COUNT]; ///< Remappings for each shader stage.
  ezHybridArray<LayoutBinding, 6> sourceBindings;                             ///< Bindings across all stages. Can have gaps. Array index is the binding index.
  ezMap<ezStringView, ezUInt32> bindingMap;                                   ///< Maps binding name to index in sourceBindings.

  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; i++)
  {
    const vk::ShaderStageFlags vulkanStage = ezConversionUtilsVulkan::GetShaderStage((ezGALShaderStage::Enum)i);
    if (m_Description.HasByteCodeForStage((ezGALShaderStage::Enum)i))
    {
      EZ_ASSERT_DEV(sets[i].GetCount() <= 1, "Only a single descriptor set is currently supported.");

      for (ezUInt32 j = 0; j < sets[i].GetCount(); j++)
      {
        const ezVulkanDescriptorSetLayout& set = sets[i][j];
        EZ_ASSERT_DEV(set.m_uiSet == 0, "Only a single descriptor set is currently supported.");
        for (ezUInt32 k = 0; k < set.bindings.GetCount(); k++)
        {
          const ezVulkanDescriptorSetLayoutBinding& binding = set.bindings[k];
          // Does a binding already exist for the resource with the same name?
          if (ezUInt32* pBindingIdx = bindingMap.GetValue(binding.m_sName))
          {
            LayoutBinding& layoutBinding = sourceBindings[*pBindingIdx];
            layoutBinding.m_stages |= vulkanStage;
            const ezVulkanDescriptorSetLayoutBinding* pCurrentBinding = layoutBinding.m_binding;
            EZ_ASSERT_DEBUG(pCurrentBinding->m_Type == binding.m_Type, "The descriptor {} was found with different resource type {} and {}", binding.m_sName, pCurrentBinding->m_Type, binding.m_Type);
            EZ_ASSERT_DEBUG(pCurrentBinding->m_uiDescriptorType == binding.m_uiDescriptorType, "The descriptor {} was found with different type {} and {}", binding.m_sName, pCurrentBinding->m_uiDescriptorType, binding.m_uiDescriptorType);
            EZ_ASSERT_DEBUG(pCurrentBinding->m_uiDescriptorCount == binding.m_uiDescriptorCount, "The descriptor {} was found with different count {} and {}", binding.m_sName, pCurrentBinding->m_uiDescriptorCount, binding.m_uiDescriptorCount);
            // The binding index differs from the one already in the set, remapping is necessary.
            if (binding.m_uiBinding != *pBindingIdx)
            {
              remappings[i].PushBack({&binding, pCurrentBinding->m_uiBinding});
            }
          }
          else
          {
            ezUInt8 uiTargetBinding = binding.m_uiBinding;
            // Doesn't exist yet, find a good place for it.
            if (binding.m_uiBinding >= sourceBindings.GetCount())
              sourceBindings.SetCount(binding.m_uiBinding + 1);

            // If the original binding index doesn't exist yet, use it (No remapping necessary).
            if (sourceBindings[binding.m_uiBinding].m_binding == nullptr)
            {
              sourceBindings[binding.m_uiBinding] = {&binding, vulkanStage};
              bindingMap[binding.m_sName] = uiTargetBinding;
            }
            else
            {
              // Binding index already in use, remapping necessary.
              uiTargetBinding = (ezUInt8)sourceBindings.GetCount();
              sourceBindings.PushBack({&binding, vulkanStage});
              bindingMap[binding.m_sName] = uiTargetBinding;
              remappings[i].PushBack({&binding, uiTargetBinding});
            }

            // The shader reflection used by the high level renderer is per stage and assumes it can map resources to stages.
            // We build this remapping table to map our descriptor binding to the original per-stage resource binding model.
            BindingMapping& bindingMapping = m_BindingMapping.ExpandAndGetRef();
            bindingMapping.m_descriptorType = (vk::DescriptorType)binding.m_uiDescriptorType;
            bindingMapping.m_ezType = binding.m_ezType;
            bindingMapping.m_type = (BindingMapping::Type)binding.m_Type;
            bindingMapping.m_stage = (ezGALShaderStage::Enum)i;
            bindingMapping.m_uiSource = binding.m_uiVirtualBinding;
            bindingMapping.m_uiTarget = uiTargetBinding;
          }
        }
      }
    }
  }
  m_BindingMapping.Sort([](const BindingMapping& lhs, const BindingMapping& rhs) { return lhs.m_uiTarget < rhs.m_uiTarget; });
  for (ezUInt32 i = 0; i < m_BindingMapping.GetCount(); i++)
  {
    m_BindingMapping[i].m_targetStages = ezConversionUtilsVulkan::GetPipelineStage(sourceBindings[m_BindingMapping[i].m_uiTarget].m_stages);
  }

  // Build Vulkan descriptor set layout
  for (ezUInt32 i = 0; i < sourceBindings.GetCount(); i++)
  {
    const LayoutBinding& sourceBinding = sourceBindings[i];
    if (sourceBinding.m_binding != nullptr)
    {
      vk::DescriptorSetLayoutBinding& binding = m_descriptorSetLayoutDesc.m_bindings.ExpandAndGetRef();
      binding.binding = i;
      binding.descriptorType = (vk::DescriptorType)sourceBinding.m_binding->m_uiDescriptorType;
      binding.descriptorCount = sourceBinding.m_binding->m_uiDescriptorCount;
      binding.stageFlags = sourceBinding.m_stages;
    }
  }
  m_descriptorSetLayoutDesc.m_bindings.Sort([](const vk::DescriptorSetLayoutBinding& lhs, const vk::DescriptorSetLayoutBinding& rhs) { return lhs.binding < rhs.binding; });
  m_descriptorSetLayoutDesc.ComputeHash();

  // Remap and build shaders
  ezUInt32 uiMaxShaderSize = 0;
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; i++)
  {
    if (!remappings[i].IsEmpty())
    {
      uiMaxShaderSize = ezMath::Max(uiMaxShaderSize, shaderCode[i].GetCount());
    }
  }

  vk::ShaderModuleCreateInfo createInfo;
  ezDynamicArray<ezUInt8> tempBuffer;
  tempBuffer.Reserve(uiMaxShaderSize);
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; i++)
  {
    if (m_Description.HasByteCodeForStage((ezGALShaderStage::Enum)i))
    {
      if (remappings[i].IsEmpty())
      {
        createInfo.codeSize = shaderCode[i].GetCount();
        EZ_ASSERT_DEV(createInfo.codeSize % 4 == 0, "Spirv shader code should be a multiple of 4.");
        createInfo.pCode = reinterpret_cast<const ezUInt32*>(shaderCode[i].GetPtr());
        VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_Shaders[i]));
      }
      else
      {
        tempBuffer = shaderCode[i];
        ezUInt32* pData = reinterpret_cast<ezUInt32*>(tempBuffer.GetData());
        for (const auto& remap : remappings[i])
        {
          EZ_ASSERT_DEBUG(pData[remap.pBinding->m_uiWordOffset] == remap.pBinding->m_uiBinding, "Spirv descriptor word offset does not point to descriptor index.");
          pData[remap.pBinding->m_uiWordOffset] = remap.m_uiTarget;
        }
        createInfo.codeSize = tempBuffer.GetCount();
        EZ_ASSERT_DEV(createInfo.codeSize % 4 == 0, "Spirv shader code should be a multiple of 4.");
        createInfo.pCode = pData;
        VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_Shaders[i]));
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezGALShaderVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  m_descriptorSetLayoutDesc = {};
  m_BindingMapping.Clear();

  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; i++)
  {
    pVulkanDevice->DeleteLater(m_Shaders[i]);
  }
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Shader_Implementation_ShaderVulkan);
