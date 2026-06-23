#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Resources/ReadbackHelper.h>

using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;

class ezGALDevice;
class ezRenderGraph;
class ezStreamReader;
class ezStreamWriter;

/// Identifies a specific texture access within a render graph pass to observe.
/// Optionally also sets a target swap-chain to blit a preview image on.
struct ezRenderGraphObserverRequest
{
  // Source texture
  ezUInt64 m_uiRenderGraphId = 0;
  ezString m_sPassName;
  ezUInt16 m_uiAccessIndex = 0; ///< Nth texture access (reads then writes) within the pass.

  // Preview target
  ezUInt32 m_uiSwapChainId = 0xFFFFFFFF; ///< If invalid, no preview will be rendered.

  // Sub-resource
  ezUInt16 m_uiArraySlice = 0;
  ezUInt8 m_uiMipLevel = 0;
  ezUInt8 m_uiChannelMask = EZ_BIT(0) | EZ_BIT(1) | EZ_BIT(2) | EZ_BIT(3); ///< red, green, blue, alpha
  ezInt8 m_iSampleIndex = -1;                                              ///< Negative = auto-resolve, 0+ = specific MSAA sample.

  // Value Clamp
  float m_fRangeMin = 0.0f;
  float m_fRangeMax = 1.0f;

  // Zoom
  float m_fZoom = 1.0f;
  ezVec2 m_vPanCenter = ezVec2(0.5f);

  // Pixel readback
  ezVec2I32 m_vPixelPosition = ezVec2I32(-1, -1); ///< Updated while pixel sampling is active. If negative, no pixel is selected.
  bool m_bHighlightPixel = false;                 ///< If true, preview rendering highlights the selected pixel row and column.
};

/// Contains the readback data produced for the active render graph observer request.
struct ezRenderGraphObserverResponse
{
  float m_fImageMin = 0.0f;
  float m_fImageMax = 1.0f;
  ezStaticArray<ezUInt8, 1024> m_Histogram; ///< Four 256-bin RGBA histograms, relative to m_fRangeMin / m_fRangeMax and normalized to the highest non-flat channel value.
  bool m_bHistogramValid = false;
  ezVariant m_PixelValue;
};

EZ_RENDERERCORE_DLL void operator<<(ezStreamWriter& inout_stream, const ezRenderGraphObserverRequest& value);
EZ_RENDERERCORE_DLL void operator>>(ezStreamReader& inout_stream, ezRenderGraphObserverRequest& ref_value);
EZ_RENDERERCORE_DLL void operator<<(ezStreamWriter& inout_stream, const ezRenderGraphObserverResponse& value);
EZ_RENDERERCORE_DLL void operator>>(ezStreamReader& inout_stream, ezRenderGraphObserverResponse& ref_value);

/// Observes a texture at a specific point in the render graph pipeline by copying it into a persistent texture after the target pass executes. Register instances with ezRenderGraphManager. The render graph will compute barriers and execute the copy without modifying its own data structures.
class EZ_RENDERERCORE_DLL ezRenderGraphPassObserver : public ezRefCounted
{
public:
  ~ezRenderGraphPassObserver();

  /// Thread-safe. Can be called from any thread. The request is applied
  /// on the render thread before barrier computation.
  void SetRequest(const ezRenderGraphObserverRequest& request);

  /// Returns the active request. Only valid to call from the render thread.
  const ezRenderGraphObserverRequest& GetRequest() const { return m_Request; }

  /// Returns the persistent copy of the observed texture as a resource handle.
  /// Can be used with material texture bindings. Invalid if no capture has occurred.
  const ezTexture2DResourceHandle& GetCopyTextureResource() const { return m_hCopyTextureResource; }

  /// Returns the underlying GAL handle of the copy texture. Used internally for
  /// the GPU copy operation.
  ezGALTextureHandle GetCopyTexture() const { return m_hCopyTexture; }

  /// Thread-safe. Returns the most recent inspection data that has finished readback.
  ezRenderGraphObserverResponse GetResponse() const;

  /// \name Internal — called by ezRenderGraph during ComputeBarriers / Execute
  ///@{

  /// Clears per-frame state. Called at the start of each frame's barrier computation.
  void Reset();

  /// Copies the pending request into the active request. Called on the
  /// render thread under the manager's mutex before barrier computation.
  void ApplyPendingRequest();

  /// (Re)creates the copy texture to match the source description if needed.
  void EnsureCopyTexture(const ezGALTextureCreationDescription& srcDesc);

  void RecordPreview(ezRenderGraph& ref_graph);

  ///@}

private:
  friend class ezRenderGraph;
  friend class ezRenderGraphManager;
  ezRenderGraphPassObserver(ezGALDevice* pDevice);

  void EnsureInspectionResources();
  void DestroyInspectionResources();
  void PollReadbacks();
  void ProcessPixelReadback(ezArrayPtr<const ezUInt8> memory);
  void ProcessHistogramReadback(ezArrayPtr<const ezUInt8> memory);
  void ProcessMinMaxReadback(ezArrayPtr<const ezUInt8> memory);

  mutable ezMutex m_Mutex;
  ezGALDevice* m_pDevice = nullptr;
  ezRenderGraphObserverRequest m_Request;        ///< Active request, only accessed on render thread.
  ezRenderGraphObserverRequest m_PendingRequest; ///< Written by any thread via SetRequest(), guarded by m_Mutex.
  ezRenderGraphObserverResponse m_Response;      ///< Guarded by m_Mutex.
  bool m_bPendingRequestDirty = false;           ///< True when m_PendingRequest has been set but not yet applied.
  ezRenderGraph* m_pGraph = nullptr;
  ezGALSwapChainHandle m_hSwapChain;
  ezGALTextureHandle m_hCopyTexture;
  ezTexture2DResourceHandle m_hCopyTextureResource;

  ezGALBufferHandle m_hPixelReadbackBuffer;
  ezGALBufferHandle m_hHistogramBuffer;
  ezGALBufferHandle m_hMinMaxBuffer;
  ezGALReadbackBufferHelper m_PixelReadback;
  ezGALReadbackBufferHelper m_HistogramReadback;
  ezGALReadbackBufferHelper m_MinMaxReadback;
  ezShaderResourceHandle m_hReadbackPixelShader;
  ezShaderResourceHandle m_hClearHistogramShader;
  ezShaderResourceHandle m_hBuildHistogramShader;
  ezShaderResourceHandle m_hClearMinMaxShader;
  ezShaderResourceHandle m_hBuildMinMaxShader;
  ezShaderResourceHandle m_hPreviewShader;
  bool m_bPixelReadbackInFlight = false;
  bool m_bHistogramReadbackInFlight = false;
  bool m_bMinMaxReadbackInFlight = false;

  bool m_bValid = false;
  ezUInt32 m_uiSortedPassIndex = 0xFFFFFFFF;
  ezGALTextureHandle m_hResolvedSourceTexture;

  ezHybridArray<ezGALTextureBarrier, 4> m_PreCopyBarriers;
  ezHybridArray<ezGALTextureBarrier, 4> m_PostCopyBarriers;
};
