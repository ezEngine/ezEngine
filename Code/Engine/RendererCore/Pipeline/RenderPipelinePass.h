#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/TagSet.h>
#include <CoreUtils/NodeGraph/Node.h>
#include <RendererCore/Pipeline/Declarations.h>

class ezRenderPipeline;

class EZ_RENDERERCORE_DLL ezRenderPipelinePass : public ezNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelinePass);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezRenderPipelinePass);

public:
  ezRenderPipelinePass(const char* szName);
  ~ezRenderPipelinePass();

  void AddRenderer(ezUniquePtr<ezRenderer>&& pRenderer);
  void RemoveRenderer(ezUniquePtr<ezRenderer>&& pRenderer);

  virtual void Execute(const ezRenderViewContext& renderViewContext) = 0;

  virtual void GetRenderTargetDescriptions(ezDynamicArray<ezGALTextureCreationDescription*>& outputs,
    ezDynamicArray<ezGALTextureCreationDescription*>& helper);

  void RenderDataWithPassType(const ezRenderViewContext& renderViewContext, ezRenderPassType passType);

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
  ezDynamicArray<ezUniquePtr<ezRenderer>> m_Renderer;
};

