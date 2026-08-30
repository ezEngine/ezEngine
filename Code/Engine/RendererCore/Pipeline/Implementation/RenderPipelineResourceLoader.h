#pragma once

#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Status.h>
#include <RendererCore/RendererCoreDLL.h>

class ezRenderPipeline;
struct ezRenderPipelineResourceDescriptor;
class ezStreamReader;
class ezStreamWriter;
class ezRenderPipelinePass;
class ezRenderPipelineNode;
class ezExtractor;

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
  /// Loads a transformed render pipeline by asset GUID or path. Used to resolve ezSubGraphNode references.
  using ImportPipelineCallback = ezDelegate<ezStatus(ezStringView, ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>>&, ezDynamicArray<ezUniquePtr<ezExtractor>>&, ezDynamicArray<ezRenderPipelineResourceLoaderConnection>&)>;

  /// Reads passes, extractors and connections from the binary format written by ExportPipeline.
  static ezStatus ImportPipeline(ezStreamReader& ref_streamReader, ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>>& out_passes, ezDynamicArray<ezUniquePtr<ezExtractor>>& out_extractors, ezDynamicArray<ezRenderPipelineResourceLoaderConnection>& out_connections);

  /// Replaces every ezSubGraphNode node with the contents of the pipeline it references.
  ///
  /// Connections to the sub-graph's pins are rerouted to the passes behind the corresponding boundary nodes, and connections to unconnected boundaries are dropped. Extractors that the root graph already has are not imported again. The imported objects are kept alive through ref_ownedPasses / ref_ownedExtractors, so those have to outlive ref_nodes and ref_extractors.
  static ezStatus InlineImportedSubGraphs(ezDynamicArray<ezRenderPipelineNode*>& ref_nodes, ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>>& ref_ownedPasses, ezDynamicArray<ezExtractor*>& ref_extractors, ezDynamicArray<ezUniquePtr<ezExtractor>>& ref_ownedExtractors, ezDynamicArray<ezRenderPipelineResourceLoaderConnection>& ref_connections, const ImportPipelineCallback& importPipeline);

  static ezInternal::NewInstance<ezRenderPipeline> CreateRenderPipeline(const ezRenderPipelineResourceDescriptor& desc);
  static ezResult ExportPipeline(ezArrayPtr<const ezRenderPipelinePass* const> passes, ezArrayPtr<const ezExtractor* const> extractors, ezArrayPtr<const ezRenderPipelineResourceLoaderConnection> connections, ezStreamWriter& ref_streamWriter);
};
