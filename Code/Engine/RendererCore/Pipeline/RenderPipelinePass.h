#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderPipelineNode.h>

struct ezGALTextureCreationDescription;
class ezStreamWriter;

/// \brief Passed to ezRenderPipelinePass::InitRenderPipelinePass to inform about
/// existing connections on each input / output pin index.
struct ezRenderPipelinePassConnection
{
  ezRenderPipelinePassConnection() { m_pOutput = nullptr; }

  ezGALTextureCreationDescription m_Desc;
  ezGALTextureHandle m_TextureHandle;
  const ezRenderPipelineNodePin* m_pOutput;                  ///< The output pin that this connection spawns from.
  ezHybridArray<const ezRenderPipelineNodePin*, 4> m_Inputs; ///< The various input pins this connection is connected to.
};

class EZ_RENDERERCORE_DLL ezRenderPipelinePass : public ezRenderPipelineNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelinePass, ezRenderPipelineNode);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezRenderPipelinePass);

public:
  ezRenderPipelinePass(const char* szName, bool bIsStereoAware = false);
  ~ezRenderPipelinePass();

  /// \brief Sets the name of the pass.
  void SetName(const char* szName);

  /// \brief returns the name of the pass.
  const char* GetName() const;

  /// \brief True if the render pipeline pass can handle stereo cameras correctly.
  bool IsStereoAware() const { return m_bIsStereoAware; }

  /// \brief For a given input pin configuration, provide the output configuration of this node.
  /// Outputs is already resized to the number of output pins.
  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) = 0;

  /// Returns the current texture this node provides at the given *ProviderPin.
  /// This function is called every frame if this node holds a ezRenderPipelineNodeInputProviderPin or ezRenderPipelineNodeOutputProviderPin pin. The node can return a valid texture handle, or an invalid handle, in which case the missing texture will be created from the texture pool.
  /// \param pPin The member pin for which the texture is requested.
  /// \param desc The format of the texture that should be provided.
  /// \return The texture to use for this pin's connections. Or invalid, in which case it reverts to a regular input / output pin.
  virtual ezGALTextureHandle QueryTextureProvider(const ezRenderPipelineNodePin* pPin, const ezGALTextureCreationDescription& desc) { return {}; }

  /// \brief After GetRenderTargetDescriptions was called successfully for each pass, this function is called
  /// with the inputs and outputs for review. Disconnected pins have a nullptr value in the passed in arrays.
  /// This is the time to create additional resources that are not covered by the pins automatically, e.g. a picking texture or eye
  /// adaptation buffer.
  virtual void InitRenderPipelinePass(const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs);

  /// \brief Render into outputs. Both inputs and outputs are passed in with actual texture handles.
  /// Disconnected pins have a nullptr value in the passed in arrays. You can now create views and render target setups on the fly and
  /// fill the output targets with data.
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) = 0;

  virtual void ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs);

  /// \brief Allows for the pass to write data back using ezView::SetRenderPassReadBackProperty. E.g. picking results etc.
  virtual void ReadBackProperties(ezView* pView);

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const;
  virtual ezResult Deserialize(ezStreamReader& inout_stream);

  void RenderDataWithCategory(const ezRenderViewContext& renderViewContext, ezRenderData::Category category);

  EZ_ALWAYS_INLINE ezRenderPipeline* GetPipeline() { return m_pPipeline; }
  EZ_ALWAYS_INLINE const ezRenderPipeline* GetPipeline() const { return m_pPipeline; }

private:
  friend class ezRenderPipeline;

  bool m_bActive = true;

  const bool m_bIsStereoAware;
  ezHashedString m_sName;

  ezRenderPipeline* m_pPipeline = nullptr;
};
