#pragma once

#include <Foundation/Serialization/RttiConverter.h>
#include <RendererCore/RendererCoreDLL.h>

class ezRenderPipeline;
struct ezRenderPipelineResourceDescriptor;

struct EZ_RENDERERCORE_DLL ezRenderPipelineResourceLoader
{
  static ezInternal::NewInstance<ezRenderPipeline> CreateRenderPipeline(const ezRenderPipelineResourceDescriptor& desc);
  static void CreateRenderPipelineResourceDescriptor(const ezRenderPipeline* pPipeline, ezRenderPipelineResourceDescriptor& ref_desc);
};

class EZ_RENDERERCORE_DLL ezRenderPipelineRttiConverterContext : public ezRttiConverterContext
{
public:
  ezRenderPipelineRttiConverterContext()

  {
  }

  virtual void Clear() override;

  virtual ezInternal::NewInstance<void> CreateObject(const ezUuid& guid, const ezRTTI* pRtti) override;
  virtual void DeleteObject(const ezUuid& guid) override;

  ezRenderPipeline* m_pRenderPipeline = nullptr;
};
