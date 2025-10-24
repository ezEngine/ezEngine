#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/Resources/BufferPool.h>

struct ezPerInstanceData;
class ezInstanceDataProvider;
class ezInstancedMeshComponent;

/// Stores per-instance data for instanced rendering.
///
/// Manages a GPU buffer pool containing ezPerInstanceData structures. Supports both transient
/// (per-frame) and persistent buffers. Used to render multiple instances of the same mesh with
/// different transformations, colors, etc. in a single draw call.
struct EZ_RENDERERCORE_DLL ezInstanceData
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezInstanceData);

public:
  /// Constructor.
  ///
  /// \param uiMaxInstanceCount Maximum number of instances this object can represent.
  /// \param bTransient If true, data will not be preserved across frames, requiring re-upload every frame.
  ezInstanceData(ezUInt32 uiMaxInstanceCount = 1024, bool bTransient = true);
  ~ezInstanceData();

  ezGALBufferPool m_InstanceDataBuffer;
  ezConstantBufferStorageHandle m_hConstantBuffer;

  /// Binds the instance data buffer to the render context for rendering.
  void BindResources(ezRenderContext* pRenderContext);

  /// Allocates space for instance data and returns a writable array.
  ///
  /// The offset into the buffer is returned in out_uiOffset. The returned array should be filled
  /// with per-instance data before calling UpdateInstanceData.
  ezArrayPtr<ezPerInstanceData> GetInstanceData(ezRenderContext* pRenderContext, ezUInt32 uiCount, ezUInt32& out_uiOffset);

  /// Uploads the filled instance data to the GPU.
  ///
  /// Must be called after filling the array returned by GetInstanceData.
  void UpdateInstanceData(ezRenderContext* pRenderContext, ezUInt32 uiCount);

private:
  friend ezInstanceDataProvider;
  friend ezInstancedMeshComponent;

  void CreateBuffer(ezUInt32 uiSize, bool bTransient);
  void Reset();

  ezUInt32 m_uiBufferSize = 0;
  ezUInt32 m_uiBufferOffset = 0;
  ezDynamicArray<ezPerInstanceData, ezAlignedAllocatorWrapper> m_PerInstanceData;
};

/// Frame data provider for instance data.
///
/// Manages per-frame instance data for instanced rendering. Provides access to ezInstanceData
/// through the frame data provider interface.
class EZ_RENDERERCORE_DLL ezInstanceDataProvider : public ezFrameDataProvider<ezInstanceData>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInstanceDataProvider, ezFrameDataProviderBase);

public:
  ezInstanceDataProvider();
  ~ezInstanceDataProvider();

private:
  virtual void* UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData) override;

  ezInstanceData m_Data;
};
