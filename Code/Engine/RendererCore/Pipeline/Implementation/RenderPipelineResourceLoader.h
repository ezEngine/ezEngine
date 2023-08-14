#pragma once

#include <Foundation/Serialization/RttiConverter.h>
#include <RendererCore/RendererCoreDLL.h>

class ezRenderPipeline;
struct ezRenderPipelineResourceDescriptor;

struct EZ_RENDERERCORE_DLL ezRenderPipelineResourceLoaderConnection
{
  ezUInt32 m_uiSource;
  ezUInt32 m_uiTarget;
  ezString m_sSourcePin;
  ezString m_sTargetPin;

  ezResult Serialize(ezStreamWriter& inout_stream);
  ezResult Deserialize(ezStreamReader& inout_stream);
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezRenderPipelineResourceLoaderConnection);

struct EZ_RENDERERCORE_DLL ezRenderPipelineResourceLoader
{
  static ezInternal::NewInstance<ezRenderPipeline> CreateRenderPipeline(const ezRenderPipelineResourceDescriptor& desc);
  static void CreateRenderPipelineResourceDescriptor(const ezRenderPipeline* pPipeline, ezRenderPipelineResourceDescriptor& ref_desc);
};