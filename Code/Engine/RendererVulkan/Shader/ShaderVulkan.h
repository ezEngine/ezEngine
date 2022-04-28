
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class EZ_RENDERERVULKAN_DLL ezGALShaderVulkan : public ezGALShader
{
public:
  void SetDebugName(const char* szName) const override;

  EZ_ALWAYS_INLINE vk::ShaderModule GetVertexShader() const;
  EZ_ALWAYS_INLINE vk::ShaderModule GetHullShader() const;
  EZ_ALWAYS_INLINE vk::ShaderModule GetDomainShader() const;
  EZ_ALWAYS_INLINE vk::ShaderModule GetGeometryShader() const;
  EZ_ALWAYS_INLINE vk::ShaderModule GetPixelShader() const;
  EZ_ALWAYS_INLINE vk::ShaderModule GetComputeShader() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALShaderVulkan(const ezGALShaderCreationDescription& description);
  virtual ~ezGALShaderVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

private:
  vk::ShaderModule m_pVertexShader;
  vk::ShaderModule m_pHullShader;
  vk::ShaderModule m_pDomainShader;
  vk::ShaderModule m_pGeometryShader;
  vk::ShaderModule m_pPixelShader;
  vk::ShaderModule m_pComputeShader;
};

#include <RendererVulkan/Shader/Implementation/ShaderVulkan_inl.h>
