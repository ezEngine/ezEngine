#include <Foundation/PCH.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezGraphPatch);

ezGraphPatch::ezGraphPatch(const ezRTTI* pType, ezUInt32 uiTypeVersion)
  : m_pType(pType)
  , m_uiTypeVersion(uiTypeVersion)
{
}

void ezGraphPatch::PatchBaseClass(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode, const ezRTTI* pType, ezUInt32 uiTypeVersion) const
{
  ezGraphVersioning::GetSingleton()->PatchNode(pGraph, pNode, pType, uiTypeVersion);
}

const ezRTTI* ezGraphPatch::GetType() const
{
  return m_pType;
}

ezUInt32 ezGraphPatch::GetTypeVersion() const
{
  return m_uiTypeVersion;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_GraphPatch);

