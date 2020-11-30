
#pragma once

#include <RendererFoundation/State/State.h>

#include <vulkan/vulkan.hpp>

class EZ_RENDERERVULKAN_DLL ezGALBlendStateVulkan : public ezGALBlendState
{
public:

  EZ_ALWAYS_INLINE vk::PipelineColorBlendStateCreateInfo* GetDXBlendState() const;

protected:

  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALBlendStateVulkan(const ezGALBlendStateCreationDescription& Description);

  ~ezGALBlendStateVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  vk::PipelineColorBlendStateCreateInfo m_blendState = {};
  vk::PipelineColorBlendAttachmentState m_blendAttachmentState[8] = {};
};

class EZ_RENDERERVULKAN_DLL ezGALDepthStencilStateVulkan : public ezGALDepthStencilState
{
public:

  EZ_ALWAYS_INLINE vk::PipelineDepthStencilStateCreateInfo* GetDepthStencilState() const;

protected:

  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALDepthStencilStateVulkan(const ezGALDepthStencilStateCreationDescription& Description);

  ~ezGALDepthStencilStateVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  vk::PipelineDepthStencilStateCreateInfo m_depthStencilState = {};
};

class EZ_RENDERERVULKAN_DLL ezGALRasterizerStateVulkan : public ezGALRasterizerState
{
public:

  EZ_ALWAYS_INLINE vk::PipelineRasterizationStateCreateInfo* GetRasterizerState() const;

protected:

  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALRasterizerStateVulkan(const ezGALRasterizerStateCreationDescription& Description);

  ~ezGALRasterizerStateVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  vk::PipelineRasterizationStateCreateInfo m_rasterizerState = {};
};

class EZ_RENDERERVULKAN_DLL ezGALSamplerStateVulkan : public ezGALSamplerState
{
public:

  EZ_ALWAYS_INLINE vk::DescriptorSetLayoutBinding* GetSamplerState() const;

protected:

  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALSamplerStateVulkan(const ezGALSamplerStateCreationDescription& Description);

  ~ezGALSamplerStateVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  vk::Sampler m_sampler = {};
  vk::DescriptorSetLayoutBinding m_samplerState = {};
};


#include <RendererVulkan/State/Implementation/StateVulkan_inl.h>
