
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Shader/BindGroup.h>

struct ID3D11DeviceChild;
struct ID3D11DeviceContext;
struct ID3DUserDefinedAnnotation;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11Buffer;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;
struct ID3D11SamplerState;
struct ID3D11Query;

class ezGALDeviceDX11;
struct ezGALBindGroupCreationDescription;

class EZ_RENDERERDX11_DLL ezGALCommandEncoderImplDX11 final : public ezGALCommandEncoderCommonPlatformInterface
{
public:
  ezGALCommandEncoderImplDX11(ezGALDeviceDX11& ref_deviceDX11);
  ~ezGALCommandEncoderImplDX11();

  void EndFrame();

  // ezGALCommandEncoderCommonPlatformInterface
  // State setting functions
  virtual void SetBindGroupPlatform(ezUInt32 uiBindGroup, const ezGALBindGroupCreationDescription& bindGroup) override;
  virtual void SetPushConstantsPlatform(ezArrayPtr<const ezUInt8> data) override;

  // GPU -> CPU query functions

  virtual ezGALTimestampHandle InsertTimestampPlatform() override;
  virtual ezGALOcclusionHandle BeginOcclusionQueryPlatform(ezEnum<ezGALQueryType> type) override;
  virtual void EndOcclusionQueryPlatform(ezGALOcclusionHandle hOcclusion) override;
  virtual ezGALFenceHandle InsertFencePlatform() override;


  // Resource update functions
  virtual void CopyBufferPlatform(const ezGALBuffer* pDestination, const ezGALBuffer* pSource) override;
  virtual void CopyBufferRegionPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const ezGALBuffer* pSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount) override;

  virtual void UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> sourceData, ezGALUpdateMode::Enum updateMode) override;

  virtual void CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource) override;
  virtual void CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& destinationSubResource, const ezVec3U32& vDestinationPoint, const ezGALTexture* pSource, const ezGALTextureSubresource& sourceSubResource, const ezBoundingBoxu32& box) override;

  virtual void UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& destinationSubResource,
    const ezBoundingBoxu32& destinationBox, const ezGALSystemMemoryDescription& sourceData) override;

  virtual void ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& destinationSubResource,
    const ezGALTexture* pSource, const ezGALTextureSubresource& sourceSubResource) override;

  virtual void ReadbackTexturePlatform(const ezGALReadbackTexture* pDestination, const ezGALTexture* pSource) override;
  virtual void ReadbackBufferPlatform(const ezGALReadbackBuffer* pDestination, const ezGALBuffer* pSource) override;

  virtual void GenerateMipMapsPlatform(const ezGALTexture* pTexture, ezGALTextureRange range) override;

  // Misc

  virtual void FlushPlatform() override;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker) override;
  virtual void PopMarkerPlatform() override;
  virtual void InsertEventMarkerPlatform(const char* szMarker) override;

  // ezGALCommandEncoderComputePlatformInterface
  // Dispatch
  virtual void BeginComputePlatform() override;
  virtual void EndComputePlatform() override;

  virtual ezResult DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ) override;
  virtual ezResult DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;

  // ezGALCommandEncoderRenderPlatformInterface
  virtual void BeginRenderingPlatform(const ezGALRenderingSetup& renderingSetup) override;
  virtual void EndRenderingPlatform() override;

  // Draw functions

  virtual void ClearPlatform(const ezColor& clearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear) override;

  virtual ezResult DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex) override;
  virtual ezResult DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex) override;
  virtual ezResult DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex) override;
  virtual ezResult DrawIndexedInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;
  virtual ezResult DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex) override;
  virtual ezResult DrawInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;

  // State functions

  virtual void SetIndexBufferPlatform(const ezGALBuffer* pIndexBuffer) override;
  virtual void SetVertexBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pVertexBuffer, ezUInt32 uiOffset) override;

  virtual void SetGraphicsPipelinePlatform(const ezGALGraphicsPipeline* pGraphicsPipeline) override;
  virtual void SetComputePipelinePlatform(const ezGALComputePipeline* pComputePipeline) override;

  virtual void SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth) override;
  virtual void SetScissorRectPlatform(const ezRectU32& rect) override;
  virtual void SetStencilReferencePlatform(ezUInt8 uiStencilRefValue) override;

private:
  friend class ezGALDeviceDX11;
  void SetShader(const ezGALShader* pShader);
  void SetVertexDeclaration(const ezGALVertexDeclaration* pVertexDeclaration);
  void SetPrimitiveTopology(ezGALPrimitiveTopology::Enum topology);
  void SetBlendState(const ezGALBlendState* pBlendState, const ezColor& blendFactor = ezColor::White, ezUInt32 uiSampleMask = 0xFFFFFFFFu);
  void SetDepthStencilState(const ezGALDepthStencilState* pDepthStencilState);
  void SetRasterizerState(const ezGALRasterizerState* pRasterizerState);

  bool UnsetResourceViews(const ezGALResourceBase* pResource);
  bool UnsetUnorderedAccessViews(const ezGALResourceBase* pResource);

  void SetResourceView(const ezShaderResourceBinding& binding, const ezGALResourceBase* pResource, ID3D11ShaderResourceView* pResourceViewDX11);
  void SetUnorderedAccessView(const ezShaderResourceBinding& binding, ID3D11UnorderedAccessView* pUnorderedAccessViewDX11, const ezGALResourceBase* pResource);
  void SetConstantBuffer(const ezShaderResourceBinding& binding, const ezGALBuffer* pBuffer);
  void SetSamplerState(const ezShaderResourceBinding& binding, const ezGALSamplerState* pSamplerState);

  ezResult FlushDeferredStateChanges();

  ezGALDeviceDX11& m_GALDeviceDX11;

  ID3D11DeviceContext* m_pDXContext = nullptr;
  ID3DUserDefinedAnnotation* m_pDXAnnotation = nullptr;

  // Bound objects for deferred state flushes
  ezEnum<ezGALPrimitiveTopology> m_Topology;
  ezUInt8 m_uiTessellationPatchControlPoints = 0;

  ID3D11Buffer* m_pBoundConstantBuffers[EZ_GAL_MAX_CONSTANT_BUFFER_COUNT] = {};
  ezGAL::ModifiedRange m_BoundConstantBuffersRange[ezGALShaderStage::ENUM_COUNT];

  ezHybridArray<ID3D11ShaderResourceView*, 16> m_pBoundShaderResourceViews[ezGALShaderStage::ENUM_COUNT] = {};
  ezHybridArray<const ezGALResourceBase*, 16> m_ResourcesForResourceViews[ezGALShaderStage::ENUM_COUNT];
  ezGAL::ModifiedRange m_BoundShaderResourceViewsRange[ezGALShaderStage::ENUM_COUNT];

  ezHybridArray<ID3D11UnorderedAccessView*, 16> m_BoundUnorderedAccessViews;
  ezHybridArray<const ezGALResourceBase*, 16> m_ResourcesForUnorderedAccessViews;
  ezGAL::ModifiedRange m_BoundUnorderedAccessViewsRange;

  ID3D11SamplerState* m_pBoundSamplerStates[ezGALShaderStage::ENUM_COUNT][EZ_GAL_MAX_SAMPLER_COUNT] = {};
  ezGAL::ModifiedRange m_BoundSamplerStatesRange[ezGALShaderStage::ENUM_COUNT];

  ID3D11DeviceChild* m_pBoundShaders[ezGALShaderStage::ENUM_COUNT] = {};
  ezUInt8 m_uiStencilRefValue = 0xFFu;

  ezGALRenderingSetup m_RenderTargetSetup;
  ID3D11RenderTargetView* m_pBoundRenderTargets[EZ_GAL_MAX_RENDERTARGET_COUNT] = {};
  ezUInt32 m_uiBoundRenderTargetCount = 0;
  ID3D11DepthStencilView* m_pBoundDepthStencilTarget = nullptr;

  ID3D11Buffer* m_pBoundVertexBuffers[EZ_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  ezGAL::ModifiedRange m_BoundVertexBuffersRange;

  ezUInt32 m_VertexBufferStrides[EZ_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  ezUInt32 m_VertexBufferOffsets[EZ_GAL_MAX_VERTEX_BUFFER_COUNT] = {};

  ezHashSet<const ezGALBuffer*> m_AlreadyUpdatedTransientBuffers;
};
