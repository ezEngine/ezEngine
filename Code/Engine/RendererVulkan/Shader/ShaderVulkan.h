
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererVulkan/RendererVulkanDLL.h>

struct ID3D11VertexShader;
struct ID3D11HullShader;
struct ID3D11DomainShader;
struct ID3D11GeometryShader;
struct ID3D11PixelShader;
struct ID3D11ComputeShader;

class EZ_RENDERERVULKAN_DLL ezGALShaderVulkan : public ezGALShader
{
public:
  void SetDebugName(const char* szName) const override;

  EZ_ALWAYS_INLINE vk::ShaderModule GetDXVertexShader() const;

  EZ_ALWAYS_INLINE vk::ShaderModule GetDXHullShader() const;

  EZ_ALWAYS_INLINE vk::ShaderModule GetDXDomainShader() const;

  EZ_ALWAYS_INLINE vk::ShaderModule GetDXGeometryShader() const;

  EZ_ALWAYS_INLINE vk::ShaderModule GetDXPixelShader() const;

  EZ_ALWAYS_INLINE vk::ShaderModule GetDXComputeShader() const;


protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALShaderVulkan(const ezGALShaderCreationDescription& description);

  virtual ~ezGALShaderVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  vk::ShaderModule m_pVertexShader;
  vk::ShaderModule m_pHullShader;
  vk::ShaderModule m_pDomainShader;
  vk::ShaderModule m_pGeometryShader;
  vk::ShaderModule m_pPixelShader;
  vk::ShaderModule m_pComputeShader;
};

#include <RendererVulkan/Shader/Implementation/ShaderVulkan_inl.h>
