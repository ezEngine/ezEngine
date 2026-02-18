#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

/// Migrates ProcGen nodes from using RenderPipeline pin types to ProcGen-specific pin types.
///
/// This patch processes all ProcGen node types and updates their pin property types:
/// - ezRenderPipelineNodeInputPin -> ezProcGenNodeInputPin
/// - ezRenderPipelineNodeOutputPin -> ezProcGenNodeOutputPin
class ezProcGenPinTypePatch_1_2 : public ezGraphPatch
{
public:
  ezProcGenPinTypePatch_1_2()
    : ezGraphPatch("", 2, PatchType::GraphPatch)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    // Iterate through all nodes in the graph
    for (auto it = pGraph->GetAllNodes().GetIterator(); it.IsValid(); ++it)
    {
      ezAbstractObjectNode* pCurrentNode = it.Value();
      const ezStringView sType = pCurrentNode->GetType();

      // Only process ProcGen node types
      if (sType.StartsWith("ezProcGen"))
      {
        // Iterate through all properties of this node
        auto& properties = pCurrentNode->GetProperties();
        for (ezUInt32 i = 0; i < properties.GetCount(); ++i)
        {
          auto& prop = properties[i];

          // Check if this property is a struct with a RenderPipeline pin type
          if (prop.m_Value.IsA<ezUuid>())
          {
            ezUuid propertyObjectGuid = prop.m_Value.Get<ezUuid>();
            ezAbstractObjectNode* pPropertyNode = pGraph->GetNode(propertyObjectGuid);

            if (pPropertyNode)
            {
              const ezStringView sPropertyType = pPropertyNode->GetType();

              // Rename RenderPipeline pin types to ProcGen pin types
              if (sPropertyType == "ezRenderPipelineNodeInputPin")
              {
                pPropertyNode->SetType("ezProcGenNodeInputPin");
              }
              else if (sPropertyType == "ezRenderPipelineNodeOutputPin")
              {
                pPropertyNode->SetType("ezProcGenNodeOutputPin");
              }
              else if (sPropertyType == "ezRenderPipelineNodePin")
              {
                pPropertyNode->SetType("ezProcGenNodePin");
              }
            }
          }
        }
      }
    }
  }
};

ezProcGenPinTypePatch_1_2 g_ezProcGenPinTypePatch_1_2;
