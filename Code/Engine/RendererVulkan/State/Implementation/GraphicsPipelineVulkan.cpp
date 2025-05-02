#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/GraphicsPipelineVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

ezGALGraphicsPipelineVulkan::ezGALGraphicsPipelineVulkan(const ezGALGraphicsPipelineCreationDescription& description)
  : ezGALGraphicsPipeline(description)
{
}

ezGALGraphicsPipelineVulkan::~ezGALGraphicsPipelineVulkan()
{
}

ezResult ezGALGraphicsPipelineVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pDeviceVulkan = static_cast<ezGALDeviceVulkan*>(pDevice);

  const ezGALShaderVulkan* pShader = static_cast<const ezGALShaderVulkan*>(pDevice->GetShader(m_Description.m_hShader));
  if (pShader == nullptr)
  {
    ezLog::Error("Failed to create Vulkan graphics pipeline: Invalid shader handle.");
    return EZ_FAILURE;
  }

  const ezGALBlendStateVulkan* pBlendState = static_cast<const ezGALBlendStateVulkan*>(pDevice->GetBlendState(m_Description.m_hBlendState));
  const ezGALDepthStencilStateVulkan* pDepthStencilState = static_cast<const ezGALDepthStencilStateVulkan*>(pDevice->GetDepthStencilState(m_Description.m_hDepthStencilState));
  const ezGALRasterizerStateVulkan* pRasterizerState = static_cast<const ezGALRasterizerStateVulkan*>(pDevice->GetRasterizerState(m_Description.m_hRasterizerState));
  const ezGALVertexDeclarationVulkan* pVertexDeclaration = static_cast<const ezGALVertexDeclarationVulkan*>(pDevice->GetVertexDeclaration(m_Description.m_hVertexDeclaration));

  if (pBlendState == nullptr || pDepthStencilState == nullptr || pRasterizerState == nullptr)
  {
    ezLog::Error("Failed to create Vulkan graphics pipeline: Invalid state handle(s).");
    return EZ_FAILURE;
  }

  vk::PipelineVertexInputStateCreateInfo dummy;
  const vk::PipelineVertexInputStateCreateInfo* pVertexCreateInfo = nullptr;
  ezHybridArray<vk::VertexInputBindingDescription, EZ_GAL_MAX_VERTEX_BUFFER_COUNT> bindings;
  pVertexCreateInfo = pVertexDeclaration ? &pVertexDeclaration->GetCreateInfo() : &dummy;

  vk::PipelineInputAssemblyStateCreateInfo input_assembly;
  input_assembly.topology = ezConversionUtilsVulkan::GetPrimitiveTopology(m_Description.m_Topology);
  const bool bTessellation = pShader->GetShader(ezGALShaderStage::HullShader) != nullptr;
  if (bTessellation)
  {
    // Tessellation shaders always need to use patch list as the topology.
    input_assembly.topology = vk::PrimitiveTopology::ePatchList;
  }

  // Specify rasterization state.
  const vk::PipelineRasterizationStateCreateInfo* raster = pRasterizerState->GetRasterizerState();

  // Our attachment will write to all color channels
  vk::PipelineColorBlendStateCreateInfo blend = *pBlendState->GetBlendState();
  blend.attachmentCount = m_Description.m_RenderPass.m_uiRTCount;

  // We will have one viewport and scissor box.
  vk::PipelineViewportStateCreateInfo viewport;
  viewport.viewportCount = 1;
  viewport.scissorCount = 1;

  // Depth Testing
  const vk::PipelineDepthStencilStateCreateInfo* depth_stencil = pDepthStencilState->GetDepthStencilState();

  // Multisampling.
  vk::PipelineMultisampleStateCreateInfo multisample;
  multisample.rasterizationSamples = ezConversionUtilsVulkan::GetSamples(m_Description.m_RenderPass.m_Msaa);
  if (multisample.rasterizationSamples != vk::SampleCountFlagBits::e1 && pBlendState->GetDescription().m_bAlphaToCoverage)
  {
    multisample.alphaToCoverageEnable = true;
  }

  // Specify that these states will be dynamic, i.e. not part of pipeline state object.
  ezHybridArray<vk::DynamicState, 3> dynamics;
  dynamics.PushBack(vk::DynamicState::eViewport);
  dynamics.PushBack(vk::DynamicState::eScissor);
  if (pDepthStencilState->GetDescription().m_bStencilTest)
    dynamics.PushBack(vk::DynamicState::eStencilReference);

  vk::PipelineDynamicStateCreateInfo dynamic;
  dynamic.pDynamicStates = dynamics.GetData();
  dynamic.dynamicStateCount = dynamics.GetCount();

  // Load our SPIR-V shaders.
  ezHybridArray<vk::PipelineShaderStageCreateInfo, 6> shader_stages;
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; i++)
  {
    if (vk::ShaderModule shader = pShader->GetShader((ezGALShaderStage::Enum)i))
    {
      vk::PipelineShaderStageCreateInfo& stage = shader_stages.ExpandAndGetRef();
      stage.stage = ezConversionUtilsVulkan::GetShaderStage((ezGALShaderStage::Enum)i);
      stage.module = shader;
      stage.pName = "main";
    }
  }

  vk::PipelineTessellationStateCreateInfo tessellationInfo;
  if (bTessellation)
  {
    tessellationInfo.patchControlPoints = pShader->GetDescription().m_ByteCodes[ezGALShaderStage::HullShader]->m_uiTessellationPatchControlPoints;
  }

  vk::GraphicsPipelineCreateInfo pipe;
  pipe.renderPass = ezResourceCacheVulkan::RequestRenderPass(m_Description.m_RenderPass);
  pipe.layout = pShader->GetPipelineLayout();
  pipe.stageCount = shader_stages.GetCount();
  pipe.pStages = shader_stages.GetData();
  pipe.pVertexInputState = pVertexCreateInfo;
  pipe.pInputAssemblyState = &input_assembly;
  pipe.pRasterizationState = raster;
  pipe.pColorBlendState = &blend;
  pipe.pMultisampleState = &multisample;
  pipe.pViewportState = &viewport;
  pipe.pDepthStencilState = depth_stencil;
  pipe.pDynamicState = &dynamic;
  if (bTessellation)
    pipe.pTessellationState = &tessellationInfo;

  VK_ASSERT_DEV(pDeviceVulkan->GetVulkanDevice().createGraphicsPipelines(ezResourceCacheVulkan::GetPipelineCache(), 1, &pipe, nullptr, &m_pipeline));

  return EZ_SUCCESS;
}

ezResult ezGALGraphicsPipelineVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pDeviceVulkan = static_cast<ezGALDeviceVulkan*>(pDevice);

  if (m_pipeline)
  {
    pDeviceVulkan->DeleteLater(m_pipeline);
    m_pipeline = nullptr;
  }
  return EZ_SUCCESS;
}

void ezGALGraphicsPipelineVulkan::SetDebugName(const char*)
{
}
