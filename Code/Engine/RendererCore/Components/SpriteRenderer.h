#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Renderer.h>

struct ezPerSpriteData;
class ezRenderDataBatch;
using ezShaderResourceHandle = ezTypedResourceHandle<class ezShaderResource>;

/// Implements rendering of sprites.
///
/// Sprites are rendered as quads that always face the camera. All sprites in a batch are rendered with a single draw call.
class EZ_RENDERERCORE_DLL ezSpriteRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpriteRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezSpriteRenderer);

public:
  ezSpriteRenderer();
  ~ezSpriteRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezDynamicArray<const ezRTTI*>& out_types) const override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

protected:
  /// Creates a GPU buffer for per-sprite instance data.
  ezGALBufferHandle CreateSpriteDataBuffer(ezUInt32 uiBufferSize) const;

  /// Destroys a previously created sprite data buffer.
  void DeleteSpriteDataBuffer(ezGALBufferHandle hBuffer) const;

  /// Fills the sprite data array with per-instance information from the batch.
  void FillSpriteData(const ezRenderDataBatch& batch) const;

  ezShaderResourceHandle m_hShader;
  mutable ezDynamicArray<ezPerSpriteData, ezAlignedAllocatorWrapper> m_SpriteData;
};
