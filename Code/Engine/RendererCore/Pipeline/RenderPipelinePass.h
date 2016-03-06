#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <CoreUtils/NodeGraph/Node.h>
#include <RendererCore/Pipeline/RenderData.h>

struct ezGALTextureCreationDescription;

/// \brief Passed to ezRenderPipelinePass::SetRenderTargets to inform about
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
  ezRenderPipelinePass(const char* szName);
  ~ezRenderPipelinePass();

  ezArrayPtr<ezRenderer* const> GetRenderers() const;
  void AddRenderer(ezRenderer* pRenderer);
  void RemoveRenderer(ezRenderer* pRenderer);

  /// \brief Sets the name of the pass.
  void SetName(const char* szName);

  /// \brief returns the name of the pass.
  const char* GetName() const;

  /// \brief For a given input pin configuration, provide the output configuration of this node.
  /// Outputs is already resized to the number of output pins.
  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription*const> inputs, 
    ezArrayPtr<ezGALTextureCreationDescription> outputs) = 0;

  /// \brief After GetRenderTargetDescriptions was called successfully for each pass, this function is called
  /// with the resulting actual textures. Un-connected pins have a nullptr value in the passed in arrays.
  /// This is the time to create render target setups and remember input textures etc.
  virtual void SetRenderTargets(const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) = 0;

  /// \brief Render into outputs.
  virtual void Execute(const ezRenderViewContext& renderViewContext) = 0;

  virtual void ReadBackProperties(ezView* pView) {}

  void RenderDataWithCategory(const ezRenderViewContext& renderViewContext, ezRenderData::Category category);

  EZ_FORCE_INLINE ezRenderPipeline* GetPipeline()
  {
    return m_pPipeline;
  }

private:
  friend class ezRenderPipeline;

  ezHashedString m_sName;
  ezProfilingId m_ProfilingID;

  ezRenderPipeline* m_pPipeline;

  ezHashTable<const ezRTTI*, ezUInt32> m_TypeToRendererIndex;
  ezDynamicArray<ezRenderer*> m_Renderer;
};

