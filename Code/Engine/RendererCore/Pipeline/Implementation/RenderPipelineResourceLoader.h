#pragma once

#include <RendererCore/Basics.h>
#include <Foundation/Serialization/RttiConverter.h>

class ezRenderPipeline;
struct ezRenderPipelineResourceDescriptor;

struct EZ_RENDERERCORE_DLL ezRenderPipelineResourceLoader
{
  static ezInternal::NewInstance<ezRenderPipeline> CreateRenderPipeline(const ezRenderPipelineResourceDescriptor& desc);
  static void CreateRenderPipelineResourceDescriptor(const ezRenderPipeline* pPipeline, ezRenderPipelineResourceDescriptor& desc);
};

class EZ_RENDERERCORE_DLL ezRenderPipelineRttiConverterContext : public ezRttiConverterContext
{
public:
  ezRenderPipelineRttiConverterContext() : m_pRenderPipeline(nullptr) {}

  virtual void Clear() override;

  virtual void* CreateObject(const ezUuid& guid, const ezRTTI* pRtti) override;
  virtual void DeleteObject(const ezUuid& guid) override;

  ezRenderPipeline* m_pRenderPipeline;
};



