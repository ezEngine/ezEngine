
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>
#include <RendererWebGPU/RendererWebGPUDLL.h>

#include <webgpu/webgpu_cpp.h>

class ezGALVertexDeclarationWebGPU : public ezGALVertexDeclaration
{
protected:
  friend class ezGALDeviceWebGPU;
  friend class ezMemoryUtils;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ezGALVertexDeclarationWebGPU(const ezGALVertexDeclarationCreationDescription& Description);

  virtual ~ezGALVertexDeclarationWebGPU();

private:
  friend class ezGALCommandEncoderImplWebGPU;

  ezHybridArray<wgpu::VertexAttribute, 6> m_Attributes;
  wgpu::VertexBufferLayout m_Layout;
};

#include <RendererWebGPU/Shader/Implementation/VertexDeclarationWebGPU_inl.h>
