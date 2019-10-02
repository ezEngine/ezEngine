#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

ezHashTable<duk_context*, ezWorld*> ezTypeScriptBinding::s_DukToWorld;

void ezTypeScriptBinding::StoreWorld(ezWorld* pWorld)
{
  s_DukToWorld[m_Duk.GetContext()] = pWorld;
}

ezWorld* ezTypeScriptBinding::RetrieveWorld(duk_context* pDuk)
{
  ezWorld* pWorld = nullptr;
  s_DukToWorld.TryGetValue(pDuk, pWorld);
  return pWorld;
}
