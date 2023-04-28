#include <EditorPluginScene/EditorPluginScenePCH.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezFakeRopeComponentPatch_2_3 : public ezGraphPatch
{
public:
  ezFakeRopeComponentPatch_2_3()
    : ezGraphPatch("ezFakeRopeComponent", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Anchor", "Anchor2");
    pNode->RenameProperty("AttachToOrigin", "AttachToAnchor1");
    pNode->RenameProperty("AttachToAnchor", "AttachToAnchor2");
  }
};

ezFakeRopeComponentPatch_2_3 g_ezFakeRopeComponentPatch_2_3;
