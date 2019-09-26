#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

void ezTypeScriptBinding::StoreWorld(ezWorld* pWorld)
{
  m_pWorld = pWorld;

  // store ezWorld* in the global stash
  m_Duk.OpenGlobalStashObject();

  ezWorld** pWorldBuffer = reinterpret_cast<ezWorld**>(duk_push_fixed_buffer(m_Duk.GetContext(), sizeof(void*)));
  *pWorldBuffer = pWorld;

  duk_put_prop_index(m_Duk.GetContext(), -2, 0 /* index for ezWorld* */);

  m_Duk.CloseObject();
}
