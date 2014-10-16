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

  virtual void Execute() = 0;

  void Run(const ezCamera& camera);

  void RenderDataWithPassType(ezRenderPassType passType);

  EZ_FORCE_INLINE ezRenderPipeline* GetPipeline()
  {
    return m_pPipeline;
  }

  EZ_FORCE_INLINE const ezCamera& GetCurrentCamera() const
  {
    return *m_pCurrentCamera;
  }

private:
  friend class ezRenderPipeline;

  ezHashedString m_sName;
  ezProfilingId m_ProfilingID;

  ezRenderPipeline* m_pPipeline;
  const ezCamera* m_pCurrentCamera;

  ezHashTable<const ezRTTI*, ezRenderer*> m_Renderer;
};

