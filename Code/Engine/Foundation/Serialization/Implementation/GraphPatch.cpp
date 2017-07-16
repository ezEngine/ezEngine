#include <PCH.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/RttiConverter.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezGraphPatch);

ezGraphPatch::ezGraphPatch(const char* szType, ezUInt32 uiTypeVersion, PatchType type)
  : m_uiTypeVersion(uiTypeVersion)
  , m_PatchType(type)
{
  if (!ezStringUtils::IsNullOrEmpty(szType))
    m_sType.Assign(szType);
}

const ezHashedString ezGraphPatch::GetType() const
{
  return m_sType;
}

ezUInt32 ezGraphPatch::GetTypeVersion() const
{
  return m_uiTypeVersion;
}


ezGraphPatch::PatchType ezGraphPatch::GetPatchType() const
{
  return m_PatchType;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_GraphPatch);

