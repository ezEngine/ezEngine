#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderPipelineNode.h>
#include <RendererCore/RenderGraph/Declarations.h>
#include <RendererCore/RenderGraph/RenderGraph.h>

struct ezGALTextureCreationDescription;
class ezStreamWriter;
struct ezViewData;
class ezCamera;

/// Passed into `ezRenderPipelinePass::AddRenderPasses`. Defines the type and handle of a pin connection.
struct EZ_RENDERERCORE_DLL ezRenderPipelinePinConnection
{
  enum class Connectivity : ezUInt8
  {
    None,
    Texture,
    Buffer,
  };

  EZ_ALWAYS_INLINE ezRenderPipelinePinConnection(Connectivity connectivity = Connectivity::None);
  EZ_ALWAYS_INLINE ezRenderPipelinePinConnection(Connectivity connectivity, ezRenderGraphTextureHandle hTextureHandle);
  EZ_ALWAYS_INLINE ezRenderPipelinePinConnection(Connectivity connectivity, ezRenderGraphBufferHandle hBufferHandle);
  EZ_ALWAYS_INLINE ezRenderPipelinePinConnection(const ezRenderPipelinePinConnection& other);

  EZ_ALWAYS_INLINE ezRenderPipelinePinConnection& operator=(const ezRenderPipelinePinConnection& other);

  const Connectivity m_Connectivity = Connectivity::None;
  union
  {
    ezRenderGraphTextureHandle m_TextureHandle;
    ezRenderGraphBufferHandle m_BufferHandle;
  };
};

/// Shading quality settings for forward rendering.
struct ezForwardRenderShadingQuality
{
  using StorageType = ezInt8;

  enum Enum
  {
    Normal,     ///< Full lighting and shading calculations.
    Simplified, ///< Reduced quality for performance.

    Default = Normal,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezForwardRenderShadingQuality);

EZ_DECLARE_FLAGS(ezUInt32, ezTextureValidationFlags, CheckMsaa, Optional);

class EZ_RENDERERCORE_DLL ezRenderPipelinePass : public ezRenderPipelineNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelinePass, ezRenderPipelineNode);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezRenderPipelinePass);

public:
  ezRenderPipelinePass(const char* szName, bool bIsStereoAware = false);
  ~ezRenderPipelinePass();

  /// Sets the name of the pass.
  void SetName(const char* szName);

  /// returns the name of the pass.
  const char* GetName() const;

  /// True if the render pipeline pass can handle stereo cameras correctly.
  EZ_ALWAYS_INLINE bool IsStereoAware() const { return m_bIsStereoAware; }

  /// \name New Render Graph Interface
  ///@{

  /// Called by the render pipeline for every pass that is not culled. The pass declares its render-graph passes and writes the resulting transient handles into the `outputs` array.
  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) { return EZ_SUCCESS; }

  /// Returns the current texture this node provides at the given *ProviderPin.
  /// This function is called every frame if this node holds a ezRenderPipelineNodeInputProviderPin or ezRenderPipelineNodeOutputProviderPin pin. The node can return a valid texture handle, or an invalid handle, in which case the missing texture will be created from the texture pool.
  /// \param pPin The member pin for which the texture is requested.
  /// \param desc The format of the texture that should be provided.
  /// \return The texture to use for this pin's connections. Or invalid, in which case it reverts to a regular input / output pin.
  virtual ezGALTextureHandle QueryTextureProvider(const ezRenderPipelineNodePin* pPin, const ezGALTextureCreationDescription& desc) { return {}; }

  ///@}


  /// Allows for the pass to write data back using ezView::SetRenderPassReadBackProperty. E.g. picking results etc.
  virtual void ReadBackProperties(ezView* pView);

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const;
  virtual ezResult Deserialize(ezStreamReader& inout_stream);

  /// Imports and declares the render-graph barrier dependencies that were recorded during extraction for `category` (via ezMsgExtractRenderData::AddDependency).
  /// Call this in AddRenderPasses for every category you want to call `RenderDataWithCategory` on in the execution callback.
  void DeclareRendererDependenciesForCategory(ezRenderData::Category category, ezRenderGraph& ref_graph, ezRenderGraphPassBuilder& ref_passBuilder);

  void RenderDataWithCategory(const ezRenderViewContext& renderViewContext, ezRenderData::Category category);

  EZ_ALWAYS_INLINE ezRenderPipeline* GetPipeline() { return m_pPipeline; }
  EZ_ALWAYS_INLINE const ezRenderPipeline* GetPipeline() const { return m_pPipeline; }

protected:
  void SetReadBackProperty(ezView* pView, ezStringView sPropertyName, const ezVariant& value);

  /// Checks that hTexture has the same size as hReference, typically the color target of the pass.
  ///
  /// flags can be used to optionally require the same MSAA mode, which is needed for textures bound as render targets together with hReference.
  /// An invalid hTexture is an error unless ezTextureValidationFlags::Optional is set. An invalid hReference always succeeds.
  static ezStatus ValidateMatchingTexture(const ezRenderGraph& graph, ezRenderGraphTextureHandle hReference, ezStringView sReferencePinName, ezRenderGraphTextureHandle hTexture, ezStringView sPinName, ezBitflags<ezTextureValidationFlags> flags = ezTextureValidationFlags::Default);

private:
  friend class ezRenderPipeline;
  friend class ezRenderPipelinePassGraph;

  const bool m_bIsStereoAware;
  ezHashedString m_sName;

  ezRenderPipeline* m_pPipeline = nullptr;
};

#include <RendererCore/Pipeline/Implementation/RenderPipelinePass_inl.h>
