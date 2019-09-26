#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

ezStatus ezTypeScriptBinding::Init_RequireModules()
{
  if (m_Duk.ExecuteString("var __GameObject = require(\"./ez/GameObject\");").Failed())
    return ezStatus("Failed to import 'GameObject.ts'");

  if (m_Duk.ExecuteString("var __Component = require(\"./ez/Component\");").Failed())
    return ezStatus("Failed to import 'Component.ts'");

  return ezStatus(EZ_SUCCESS);
}

void ezTypeScriptBinding::SetModuleSearchPath(const char* szPath)
{
  m_Duk.OpenGlobalStashObject();

  duk_push_string(m_Duk, szPath);
  EZ_VERIFY(duk_put_prop_string(m_Duk, -2, "ModuleSearchPath"), "");

  m_Duk.CloseObject();
}

int ezTypeScriptBinding::DukSearchModule(duk_context* pContext)
{
  ezDuktapeFunction duk(pContext);

  ezTypeScriptTranspiler* pTranspiler = static_cast<ezTypeScriptTranspiler*>(duk.RetrievePointerFromStash("Transpiler"));

  /* 
  *   index 0: id
  *   index 1: require
  *   index 2: exports
  *   index 3: module
  */

  ezStringBuilder sRequestedFile = duk.GetStringParameter(0);

  if (!sRequestedFile.HasAnyExtension())
  {
    sRequestedFile.ChangeFileExtension("ts");
  }

  // retrieve the ModuleSearchPath
  {
    duk.OpenGlobalStashObject();

    EZ_ASSERT_DEV(duk_get_prop_string(duk, -1, "ModuleSearchPath"), "");
    const char* szSearchPath = duk_get_string_default(duk, -1, "");

    if (!ezStringUtils::IsNullOrEmpty(szSearchPath))
    {
      sRequestedFile.Prepend(szSearchPath, "/");
    }

    duk_pop(duk);

    duk.CloseObject();
  }


  ezStringBuilder result;
  pTranspiler->TranspileFileAndStoreJS(sRequestedFile, result);

  return duk.ReturnString(result);
}
