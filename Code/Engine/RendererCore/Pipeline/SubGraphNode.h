#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// Marks a texture input of a render pipeline that is meant to be used as a sub-graph.
///
/// The node's name becomes the name of the input pin on the ezSubGraphNode node that references the pipeline. Boundary nodes are removed when a sub-graph is inlined into its parent, so they never end up in a runtime pipeline.
class EZ_RENDERERCORE_DLL ezSubGraphTextureInputNode : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSubGraphTextureInputNode, ezRenderPipelinePass);

public:
  ezSubGraphTextureInputNode();

  ezRenderPipelineNodeOutputPin m_Value;
};

/// Marks a texture output of a render pipeline that is meant to be used as a sub-graph. See ezSubGraphTextureInputNode.
class EZ_RENDERERCORE_DLL ezSubGraphTextureOutputNode : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSubGraphTextureOutputNode, ezRenderPipelinePass);

public:
  ezSubGraphTextureOutputNode();

  ezRenderPipelineNodeInputPin m_Value;
};

/// Marks a buffer input of a render pipeline that is meant to be used as a sub-graph. See ezSubGraphTextureInputNode.
class EZ_RENDERERCORE_DLL ezSubGraphBufferInputNode : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSubGraphBufferInputNode, ezRenderPipelinePass);

public:
  ezSubGraphBufferInputNode();

  ezRenderPipelineNodeBufferOutputPin m_Value;
};

/// Marks a buffer output of a render pipeline that is meant to be used as a sub-graph. See ezSubGraphTextureInputNode.
class EZ_RENDERERCORE_DLL ezSubGraphBufferOutputNode : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSubGraphBufferOutputNode, ezRenderPipelinePass);

public:
  ezSubGraphBufferOutputNode();

  ezRenderPipelineNodeBufferInputPin m_Value;
};

/// Placeholder for another render pipeline asset that is inlined into this one.
///
/// Its pins are derived from the boundary nodes of the referenced pipeline. This node only exists while authoring, the asset transform replaces it with the contents of the referenced pipeline, which is why it is not an ezRenderPipelinePass.
class EZ_RENDERERCORE_DLL ezSubGraphNode : public ezRenderPipelineNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSubGraphNode, ezRenderPipelineNode);

public:
  ezSubGraphNode();

  ezString m_sPipeline; ///< The render pipeline asset to inline.
};
