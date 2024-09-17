
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererWebGPU/CommandEncoder/CommandEncoderImplWebGPU.h>
#include <RendererWebGPU/RendererWebGPUDLL.h>

#include <webgpu/webgpu_cpp.h>

/// \brief The WebGPU device implementation of the graphics abstraction layer.
class EZ_RENDERERWEBGPU_DLL ezGALDeviceWebGPU : public ezGALDevice
{
public:
  ezGALDeviceWebGPU(const ezGALDeviceCreationDescription& Description);
  virtual ~ezGALDeviceWebGPU();

  // TODO WebGPU: hack
  wgpu::Surface m_MainSurface;

protected:
  virtual ezStringView GetRendererPlatform() override;
  virtual ezResult InitPlatform() override;
  virtual ezResult ShutdownPlatform() override;

  // Command encoder functions

  virtual ezGALCommandEncoder* BeginCommandsPlatform(const char* szName) override;
  virtual void EndCommandsPlatform(ezGALCommandEncoder* pPass) override;

  virtual void FlushPlatform() override;


  // State creation functions

  virtual ezGALBlendState* CreateBlendStatePlatform(const ezGALBlendStateCreationDescription& Description) override;
  virtual void DestroyBlendStatePlatform(ezGALBlendState* pBlendState) override;

  virtual ezGALDepthStencilState* CreateDepthStencilStatePlatform(const ezGALDepthStencilStateCreationDescription& Description) override;
  virtual void DestroyDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState) override;

  virtual ezGALRasterizerState* CreateRasterizerStatePlatform(const ezGALRasterizerStateCreationDescription& Description) override;
  virtual void DestroyRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState) override;

  virtual ezGALSamplerState* CreateSamplerStatePlatform(const ezGALSamplerStateCreationDescription& Description) override;
  virtual void DestroySamplerStatePlatform(ezGALSamplerState* pSamplerState) override;


  // Resource creation functions

  virtual ezGALShader* CreateShaderPlatform(const ezGALShaderCreationDescription& Description) override;
  virtual void DestroyShaderPlatform(ezGALShader* pShader) override;

  virtual ezGALBuffer* CreateBufferPlatform(const ezGALBufferCreationDescription& Description, ezArrayPtr<const ezUInt8> pInitialData) override;
  virtual void DestroyBufferPlatform(ezGALBuffer* pBuffer) override;

  virtual ezGALTexture* CreateTexturePlatform(const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual void DestroyTexturePlatform(ezGALTexture* pTexture) override;

  virtual ezGALTexture* CreateSharedTexturePlatform(const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData, ezEnum<ezGALSharedTextureType> sharedType, ezGALPlatformSharedHandle handle) override;
  virtual void DestroySharedTexturePlatform(ezGALTexture* pTexture) override;

  virtual ezGALTextureResourceView* CreateResourceViewPlatform(ezGALTexture* pResource, const ezGALTextureResourceViewCreationDescription& Description) override;
  virtual void DestroyResourceViewPlatform(ezGALTextureResourceView* pResourceView) override;

  virtual ezGALBufferResourceView* CreateResourceViewPlatform(ezGALBuffer* pResource, const ezGALBufferResourceViewCreationDescription& Description) override;
  virtual void DestroyResourceViewPlatform(ezGALBufferResourceView* pResourceView) override;

  virtual ezGALRenderTargetView* CreateRenderTargetViewPlatform(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description) override;
  virtual void DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView) override;

  ezGALTextureUnorderedAccessView* CreateUnorderedAccessViewPlatform(ezGALTexture* pResource, const ezGALTextureUnorderedAccessViewCreationDescription& Description) override;
  virtual void DestroyUnorderedAccessViewPlatform(ezGALTextureUnorderedAccessView* pUnorderedAccessView) override;

  ezGALBufferUnorderedAccessView* CreateUnorderedAccessViewPlatform(ezGALBuffer* pResource, const ezGALBufferUnorderedAccessViewCreationDescription& Description) override;
  virtual void DestroyUnorderedAccessViewPlatform(ezGALBufferUnorderedAccessView* pUnorderedAccessView) override;

  // Other rendering creation functions

  virtual ezGALVertexDeclaration* CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description) override;
  virtual void DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration) override;

  // GPU -> CPU query functions

  virtual ezEnum<ezGALAsyncResult> GetTimestampResultPlatform(ezGALTimestampHandle hTimestamp, ezTime& out_result) override;
  virtual ezEnum<ezGALAsyncResult> GetOcclusionResultPlatform(ezGALOcclusionHandle hOcclusion, ezUInt64& out_uiResult) override;
  virtual ezEnum<ezGALAsyncResult> GetFenceResultPlatform(ezGALFenceHandle hFence, ezTime timeout) override;

  // Swap chain functions

  void PresentPlatform(const ezGALSwapChain* pSwapChain, bool bVSync);

  // Misc functions

  virtual void BeginFramePlatform(ezArrayPtr<ezGALSwapChain*> swapchains, const ezUInt64 uiAppFrame) override;
  virtual void EndFramePlatform(ezArrayPtr<ezGALSwapChain*> swapchains) override;
  virtual ezUInt64 GetCurrentFramePlatform() const override;
  virtual ezUInt64 GetSafeFramePlatform() const override;

  virtual void FillCapabilitiesPlatform() override;

  virtual void WaitIdlePlatform() override;

  virtual const ezGALSharedTexture* GetSharedTexture(ezGALTextureHandle hTexture) const override;

  /// \endcond

public:
  wgpu::Instance GetInstance() { return m_Instance; }
  wgpu::Adapter GetAdapter() { return m_Adapter; }
  wgpu::Device GetDevice() { return m_Device; }

  static void PreInitWebGPU();

private:
  static void WebGPUErrorCallback(WGPUErrorType type, char const* szMessage, void* pUserdata);

  friend class ezGALCommandEncoderImplWebGPU;

  ezUniquePtr<ezGALCommandEncoder> m_pCommandEncoder;
  ezUniquePtr<ezGALCommandEncoderImplWebGPU> m_pCommandEncoderImpl;

  wgpu::Instance m_Instance;
  wgpu::Adapter m_Adapter;
  wgpu::Device m_Device;
};

#include <RendererWebGPU/Device/Implementation/DeviceWebGPU_inl.h>
