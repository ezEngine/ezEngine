#include <Foundation/PCH.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

EZ_IMPLEMENT_SINGLETON(ezGraphVersioning);

EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, GraphVersioning)

BEGIN_SUBSYSTEM_DEPENDENCIES
"Reflection"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
  EZ_DEFAULT_NEW(ezGraphVersioning);
}

ON_CORE_SHUTDOWN
{
  ezGraphVersioning* pDummy = ezGraphVersioning::GetSingleton();
  EZ_DEFAULT_DELETE(pDummy);
}

EZ_END_SUBSYSTEM_DECLARATION


ezGraphVersioning::ezGraphVersioning()
  : m_SingletonRegistrar(this)
{
  ezPlugin::s_PluginEvents.AddEventHandler(ezMakeDelegate(&ezGraphVersioning::PluginEventHandler, this));

  UpdatePatches();
}

ezGraphVersioning::~ezGraphVersioning()
{
  ezPlugin::s_PluginEvents.RemoveEventHandler(ezMakeDelegate(&ezGraphVersioning::PluginEventHandler, this));
}

void ezGraphVersioning::PatchGraph(ezAbstractObjectGraph* pGraph)
{
  auto& nodes = pGraph->GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    ezAbstractObjectNode* pNode = it.Value();
    const ezRTTI* pNodeType = ezRTTI::FindTypeByName(pNode->GetType());
    if (pNodeType)
      PatchNode(pGraph, pNode, pNodeType, pNodeType->GetTypeVersion());
  }
}

void ezGraphVersioning::PatchNode(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode, const ezRTTI* pType, ezUInt32 uiTypeVersion)
{
  if (pNode->GetTypeVersion() < uiTypeVersion)
  {
    for (ezUInt32 uiCurrentVersion = pNode->GetTypeVersion() + 1; uiCurrentVersion <= uiTypeVersion; uiCurrentVersion++)
    {
      VersionKey key;
      key.m_pType = pType;
      key.m_uiTypeVersion = uiCurrentVersion;

      const ezGraphPatch* pPatch = nullptr;
      if (m_Patches.TryGetValue(key, pPatch))
      {
        pPatch->Patch(pGraph, pNode);
        pNode->SetTypeVersion(uiCurrentVersion);
      }
    }

    // Even if there are no patches we will always up to the requested version.
    pNode->SetTypeVersion(uiTypeVersion);
  }
}

void ezGraphVersioning::PluginEventHandler(const ezPlugin::PluginEvent& EventData)
{
  UpdatePatches();
}

void ezGraphVersioning::UpdatePatches()
{
  m_Patches.Clear();

  VersionKey key;
  ezGraphPatch* pInstance = ezGraphPatch::GetFirstInstance();

  while (pInstance)
  {
    key.m_pType = pInstance->GetType();
    key.m_uiTypeVersion = pInstance->GetTypeVersion();
    m_Patches.Insert(key, pInstance);

    pInstance = pInstance->GetNextInstance();
  }
}

EZ_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_GraphVersioning);
