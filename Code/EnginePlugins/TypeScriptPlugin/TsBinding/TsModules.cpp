#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

ezResult ezTypeScriptBinding::Init_RequireModules()
{
  EZ_LOG_BLOCK("Init_RequireModules");

  if (m_Duk.ExecuteString("var __GameObject = require(\"./ez/GameObject\");").Failed())
  {
    ezLog::Error("Failed to import 'GameObject.ts'");
    return EZ_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Component = require(\"./ez/Component\");").Failed())
  {
    ezLog::Error("Failed to import 'Component.ts'");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezTypeScriptBinding::SetModuleSearchPath(const char* szPath)
{
  m_Duk.StoreStringInStash("ModuleSearchPath", szPath);
}

int ezTypeScriptBinding::DukSearchModule(duk_context* pContext)
{
  ezDuktapeFunction duk(pContext);

  ezTypeScriptTranspiler* pTranspiler = static_cast<ezTypeScriptTranspiler*>(duk.RetrievePointerFromStash("Transpiler"));

  ezStringBuilder sRequestedFile = duk.GetStringParameter(0);

  if (!sRequestedFile.HasAnyExtension())
  {
    sRequestedFile.ChangeFileExtension("ts");
  }

  if (const char* szSearchPath = duk.RetrieveStringFromStash("ModuleSearchPath"))
  {
    sRequestedFile.Prepend(szSearchPath, "/");
  }

  EZ_LOG_BLOCK("DukSearchModule", sRequestedFile);

  ezStringBuilder sTranspiledCode;
  if (pTranspiler->TranspileFileAndStoreJS(sRequestedFile, sTranspiledCode).Failed())
  {
    ezLog::Error("'required' module \"{}\" could not be found/transpiled.", sRequestedFile);
    return 0;
  }

  return duk.ReturnString(sTranspiledCode);
}
