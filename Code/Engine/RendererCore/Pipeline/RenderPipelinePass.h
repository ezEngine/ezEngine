#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Pipeline/Declarations.h>

class ezRenderPipeline;

class EZ_RENDERERCORE_DLL ezRenderPipelinePass
{
public:
  ezRenderPipelinePass(const char* szName);

  void AddRenderer(ezRenderer* pRenderer);
  void RemoveRenderer(ezRenderer* pRenderer);

  virtual void Execute(const ezRenderContext& renderContext) = 0;

  void Run(const ezRenderContext& renderContext);

  void RenderDataWithPassType(const ezRenderContext& renderContext, ezRenderPassType passType);

  EZ_FORCE_INLINE ezRenderPipeline* GetPipeline()
  {
    return m_pPipeline;
  }

private:
  friend class ezRenderPipeline;

  ezHashedString m_sName;
  ezProfilingId m_ProfilingID;

  ezRenderPipeline* m_pPipeline;

  ezHashTable<const ezRTTI*, ezRenderer*> m_Renderer;
};

