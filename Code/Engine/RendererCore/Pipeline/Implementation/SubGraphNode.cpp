#include <RendererCore/Pipeline/SubGraphNode.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSubGraphTextureInputNode, 1, ezRTTIDefaultAllocator<ezSubGraphTextureInputNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Value", m_Value),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Subgraph"),
    new ezTitleAttribute("Texture Input: {Name}"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Blue)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSubGraphTextureOutputNode, 1, ezRTTIDefaultAllocator<ezSubGraphTextureOutputNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Value", m_Value),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Subgraph"),
    new ezTitleAttribute("Texture Output: {Name}"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Blue)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSubGraphBufferInputNode, 1, ezRTTIDefaultAllocator<ezSubGraphBufferInputNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Value", m_Value),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Subgraph"),
    new ezTitleAttribute("Buffer Input: {Name}"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Teal)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSubGraphBufferOutputNode, 1, ezRTTIDefaultAllocator<ezSubGraphBufferOutputNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Value", m_Value),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Subgraph"),
    new ezTitleAttribute("Buffer Output: {Name}"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Teal)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSubGraphNode, 1, ezRTTIDefaultAllocator<ezSubGraphNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Pipeline", m_sPipeline)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_RenderPipeline", ezDependencyFlags::Transform)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Subgraph"),
    new ezTitleAttribute("Subgraph: {Pipeline}"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSubGraphTextureInputNode::ezSubGraphTextureInputNode()
  : ezRenderPipelinePass("TextureInput")
{
}

ezSubGraphTextureOutputNode::ezSubGraphTextureOutputNode()
  : ezRenderPipelinePass("TextureOutput")
{
}

ezSubGraphBufferInputNode::ezSubGraphBufferInputNode()
  : ezRenderPipelinePass("BufferInput")
{
}

ezSubGraphBufferOutputNode::ezSubGraphBufferOutputNode()
  : ezRenderPipelinePass("BufferOutput")
{
}

ezSubGraphNode::ezSubGraphNode()
  : ezRenderPipelineNode()
{
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineSubgraph);
