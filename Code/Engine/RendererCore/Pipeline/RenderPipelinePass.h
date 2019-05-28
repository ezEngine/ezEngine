#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Utilities/Node.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>

struct ezGALTextureCreationDescription;

/// \brief Passed to ezRenderPipelinePass::InitRenderPipelinePass to inform about
/// existing connections on each input / output pin index.
struct ezRenderPipelinePassConnection
{
  ezRenderPipelinePassConnection()
  {
    m_pOutput = nullptr;
  }

  ezGALTextureCreationDescription m_Desc;
  ezGALTextureHandle m_TextureHandle;
  const ezNodePin* m_pOutput; ///< The output pin that this connection spawns from.
  ezHybridArray<const ezNodePin*, 4> m_Inputs; ///< The various input pins this connection is connected to.
};

class EZ_RENDERERCORE_DLL ezRenderPipelinePass : public ezNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelinePass, ezNode);
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
  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription*const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) = 0;

  /// \brief After GetRenderTargetDescriptions was called successfully for each pass, this function is called
  /// with the inputs and outputs for review. Disconnected pins have a nullptr value in the passed in arrays.
  /// This is the time to create additional resources that are not covered by the pins automatically, e.g. a picking texture or eye adaptation buffer.
  virtual void InitRenderPipelinePass(const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs);

  /// \brief Render into outputs. Both inputs and outputs are passed in with actual texture handles.
  /// Disconnected pins have a nullptr value in the passed in arrays. You can now create views and render target setups on the fly and
  /// fill the output targets with data.
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) = 0;

  virtual void ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs);

  /// \brief Allows for the pass to write data back using ezView::SetRenderPassReadBackProperty. E.g. picking results etc.
  virtual void ReadBackProperties(ezView* pView);

  void RenderDataWithCategory(const ezRenderViewContext& renderViewContext, ezRenderData::Category category, ezRenderDataBatch::Filter filter = ezRenderDataBatch::Filter());

  EZ_ALWAYS_INLINE ezRenderPipeline* GetPipeline()
  {
    return m_pPipeline;
  }

private:
  friend class ezRenderPipeline;

  bool m_bActive;

  const bool m_bIsStereoAware;
  ezHashedString m_sName;

  ezRenderPipeline* m_pPipeline;
};

