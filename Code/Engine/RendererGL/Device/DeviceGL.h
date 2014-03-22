#pragma once

#include <RendererGL/Basics.h>
#include <RendererFoundation/Device/Device.h>
#include <Foundation/Basics/Types/Bitflags.h>

typedef ezGALFormatLookupEntry<ezUInt32, EZ_RENDERERGL_INVALID_ID> ezGALFormatLookupEntryGL;
typedef ezGALFormatLookupTable<ezGALFormatLookupEntryGL> ezGALFormatLookupTableGL;


/// \brief The GL device implementation of the graphics abstraction layer.
class EZ_RENDERERGL_DLL ezGALDeviceGL : public ezGALDevice
{

  // TODO: This shouldn't be accessible, there should be a factory instantiating the correct renderer class via RTTI for example
public:

  ezGALDeviceGL(const ezGALDeviceCreationDescription& Description);

  virtual ~ezGALDeviceGL();

public:

  EZ_FORCE_INLINE const ezGALFormatLookupTableGL& GetFormatLookupTable() const;


  struct DebugMessageSeverity
  {
    enum Enum
    {
      LOW,
      MEDIUM,
      HIGH
    };
  };

  /// \brief Activates the OpenGL debug output using KHR_DEBUG.
  /// 
  /// GL-Devices created with the DebugDevice options will automatically call this function with its default arguments.
  ///
  /// \param minMessageSeverity
  ///   Determines the minimum severity of message must have to be logged.
  void SetupDebugOutput(DebugMessageSeverity::Enum minMessageSeverity = DebugMessageSeverity::MEDIUM);

  /// \brief Returns a yet unused buffer id and removes it from the internal pool.
  ///
  /// The callee is responsible for destroying the returned buffer!
  /// Will cause a call to EnsureGLBufferPoolSize with its default parameter if not enough buffers are available.
  /// \see EnsureGLBufferPoolSize
  glBufferId GetUnusedGLBufferId();

  /// \brief Will ensure that the internal buffer-pool meets the given requirements.
  ///
  /// Will call glGenBuffers if not enough glBufferIds are available. If the pool is greater equal uiPoolSize nothing will happen.
  /// \see GetUnusedGLBufferId
  void EnsureGLBufferPoolSize(ezUInt32 uiPoolSize = 128);


  /// \brief Returns a yet unused texture id and removes it from the internal pool.
  ///
  /// The callee is responsible for destroying the returned texture!
  /// Will cause a call to EnsureGLBufferPoolSize with its default parameter if not enough textures are available.
  /// \see EnsureGLTexturePoolSize
  glBufferId GetUnusedGLTextureId();

  /// \brief Will ensure that the internal texture-pool meets the given requirements.
  ///
  /// Will call glGenBuffers if not enough glBufferIds are available. If the pool is greater equal uiPoolSize nothing will happen.
  /// \see GetUnusedGLTextureId
  void EnsureGLTexturePoolSize(ezUInt32 uiPoolSize = 128);


protected:

  // Init & shutdown functions

  virtual ezResult InitPlatform() EZ_OVERRIDE;

  virtual ezResult ShutdownPlatform() EZ_OVERRIDE;


  // State creation functions

  virtual ezGALBlendState* CreateBlendStatePlatform(const ezGALBlendStateCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyBlendStatePlatform(ezGALBlendState* pBlendState) EZ_OVERRIDE;

  virtual ezGALDepthStencilState* CreateDepthStencilStatePlatform(const ezGALDepthStencilStateCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState) EZ_OVERRIDE;

  virtual ezGALRasterizerState* CreateRasterizerStatePlatform(const ezGALRasterizerStateCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState) EZ_OVERRIDE;

  virtual ezGALSamplerState* CreateSamplerStatePlatform(const ezGALSamplerStateCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroySamplerStatePlatform(ezGALSamplerState* pSamplerState) EZ_OVERRIDE;


  // Resource creation functions

  virtual ezGALShader* CreateShaderPlatform(const ezGALShaderCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyShaderPlatform(ezGALShader* pShader) EZ_OVERRIDE;

  virtual ezGALBuffer* CreateBufferPlatform(const ezGALBufferCreationDescription& Description, const void* pInitialData) EZ_OVERRIDE;

  virtual void DestroyBufferPlatform(ezGALBuffer* pBuffer) EZ_OVERRIDE;

  virtual ezGALTexture* CreateTexturePlatform(const ezGALTextureCreationDescription& Description, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData) EZ_OVERRIDE;

  virtual void DestroyTexturePlatform(ezGALTexture* pTexture) EZ_OVERRIDE;

  virtual ezGALResourceView* CreateResourceViewPlatform(const ezGALResourceViewCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyResourceViewPlatform(ezGALResourceView* pResourceView) EZ_OVERRIDE;

  virtual ezGALRenderTargetView* CreateRenderTargetViewPlatform(const ezGALRenderTargetViewCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView) EZ_OVERRIDE;


  // Other rendering creation functions

  virtual ezGALSwapChain* CreateSwapChainPlatform(const ezGALSwapChainCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroySwapChainPlatform(ezGALSwapChain* pSwapChain) EZ_OVERRIDE;

  virtual ezGALFence* CreateFencePlatform() EZ_OVERRIDE;

  virtual void DestroyFencePlatform(ezGALFence* pFence) EZ_OVERRIDE;

  virtual ezGALQuery* CreateQueryPlatform(const ezGALQueryCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyQueryPlatform(ezGALQuery* pQuery) EZ_OVERRIDE;

  virtual ezGALRenderTargetConfig* CreateRenderTargetConfigPlatform(const ezGALRenderTargetConfigCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyRenderTargetConfigPlatform(ezGALRenderTargetConfig* pRenderTargetConfig) EZ_OVERRIDE;

  virtual ezGALVertexDeclaration* CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration) EZ_OVERRIDE;
  
  
  // Get Query Data

  virtual void GetQueryDataPlatform(ezGALQuery* pQuery, ezUInt64* puiRendererdPixels) EZ_OVERRIDE;




  // Swap chain functions

  virtual void PresentPlatform(ezGALSwapChain* pSwapChain) EZ_OVERRIDE;

  // Misc functions

  virtual void BeginFramePlatform() EZ_OVERRIDE;

  virtual void EndFramePlatform() EZ_OVERRIDE;

  virtual void FlushPlatform() EZ_OVERRIDE;

  virtual void FinishPlatform() EZ_OVERRIDE;

  virtual void SetPrimarySwapChainPlatform(ezGALSwapChain* pSwapChain) EZ_OVERRIDE;

  /// \endcond

private:

  void FillFormatLookupTable();

  /// \brief Default method for resource/state-object creation.
  template<typename ResourceType, typename DescriptionType> ResourceType* DefaultCreate(const DescriptionType& Description);

  template<typename ResourceType, typename DescriptionType, typename DataPtr> ResourceType* DefaultCreate(const DescriptionType& Description, DataPtr pInitialData);

  /// \brief Default method for resource/state-object destruction.
  template<typename ResourceType, typename ResourceTypeBaseClass> void DefaultDestroy(ResourceTypeBaseClass* pResource);


  ezGALFormatLookupTableGL m_FormatLookupTable;

  /// \see EnsureGLBufferPoolSize, GetUnusedGLBufferId
  ezDynamicArray<glBufferId> m_BufferHandlePool;

  /// \see EnsureGLTexturePoolSize, GetUnusedGLTextureId
  ezDynamicArray<glTextureId> m_TextureHandlePool;
};

#include <RendererGL/Device/Implementation/DeviceGL_inl.h>
