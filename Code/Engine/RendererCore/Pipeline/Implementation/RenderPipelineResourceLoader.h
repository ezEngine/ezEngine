#pragma once

#include <Foundation/Serialization/RttiConverter.h>
#include <RendererCore/RendererCoreDLL.h>

class ezRenderPipeline;
struct ezRenderPipelineResourceDescriptor;
class ezStreamWriter;

struct EZ_RENDERERCORE_DLL ezRenderPipelineResourceLoaderConnection
{
  ezUInt32 m_uiSource;
  ezUInt32 m_uiTarget;
  ezString m_sSourcePin;
  ezString m_sTargetPin;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezRenderPipelineResourceLoaderConnection);

struct EZ_RENDERERCORE_DLL ezRenderPipelineResourceLoader
{
  static ezInternal::NewInstance<ezRenderPipeline> CreateRenderPipeline(const ezRenderPipelineResourceDescriptor& desc);
  static void CreateRenderPipelineResourceDescriptor(const ezRenderPipeline* pPipeline, ezRenderPipelineResourceDescriptor& ref_desc);
  static ezResult ExportPipeline(ezArrayPtr<const ezRenderPipelinePass* const> passes, ezArrayPtr<const ezExtractor* const> extractors, ezArrayPtr<const ezRenderPipelineResourceLoaderConnection> connections, ezMemoryStreamWriter& ref_StreamWriter);
};
