#pragma once

#include <RendererGL/Basics.h>
#include <RendererFoundation/Device/Device.h>
#include <Foundation/Types/Bitflags.h>

typedef ezGALFormatLookupEntry<ezUInt32, EZ_RENDERERGL_INVALID_ID> ezGALFormatLookupEntryGL;
typedef ezGALFormatLookupTable<ezGALFormatLookupEntryGL> ezGALFormatLookupTableGL;


/// \brief The GL device implementation of the graphics abstraction layer.
class EZ_RENDERERGL_DLL ezGALDeviceGL : public ezGALDevice
{

  /// \todo This shouldn't be accessible, there should be a factory instantiating the correct renderer class via RTTI for example
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

  virtual ezResult InitPlatform() override;

  virtual ezResult ShutdownPlatform() override;


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

  virtual ezGALBuffer* CreateBufferPlatform(const ezGALBufferCreationDescription& Description, const void* pInitialData) override;

  virtual void DestroyBufferPlatform(ezGALBuffer* pBuffer) override;

  virtual ezGALTexture* CreateTexturePlatform(const ezGALTextureCreationDescription& Description, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData) override;

  virtual void DestroyTexturePlatform(ezGALTexture* pTexture) override;

  virtual ezGALResourceView* CreateResourceViewPlatform(const ezGALResourceViewCreationDescription& Description) override;

  virtual void DestroyResourceViewPlatform(ezGALResourceView* pResourceView) override;

  virtual ezGALRenderTargetView* CreateRenderTargetViewPlatform(const ezGALRenderTargetViewCreationDescription& Description) override;

  virtual void DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView) override;


  // Other rendering creation functions

  virtual ezGALSwapChain* CreateSwapChainPlatform(const ezGALSwapChainCreationDescription& Description) override;

  virtual void DestroySwapChainPlatform(ezGALSwapChain* pSwapChain) override;

  virtual ezGALFence* CreateFencePlatform() override;

  virtual void DestroyFencePlatform(ezGALFence* pFence) override;

  virtual ezGALQuery* CreateQueryPlatform(const ezGALQueryCreationDescription& Description) override;

  virtual void DestroyQueryPlatform(ezGALQuery* pQuery) override;

  virtual ezGALVertexDeclaration* CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description) override;

  virtual void DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration) override;
  
  
  // Get Query Data

  virtual void GetQueryDataPlatform(ezGALQuery* pQuery, ezUInt64* puiRendererdPixels) override;




  // Swap chain functions

  virtual void PresentPlatform(ezGALSwapChain* pSwapChain) override;

  // Misc functions

  virtual void BeginFramePlatform() override;

  virtual void EndFramePlatform() override;

  virtual void FlushPlatform() override;

  virtual void FinishPlatform() override;

  virtual void SetPrimarySwapChainPlatform(ezGALSwapChain* pSwapChain) override;

  virtual void FillCapabilitiesPlatform() override;

  /// \endcond

private:

  friend class ezGALSwapChainGL;

  /// Intern init method which should be called by the first ezGALSwapChainGL since a RenderContext is needed to init glew and the like.
  ezResult EnsureInternOpenGLInit();

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


  /// \see EnsureInternOpenGLInit
  bool m_bGLInitialized;
};

#include <RendererGL/Device/Implementation/DeviceGL_inl.h>
