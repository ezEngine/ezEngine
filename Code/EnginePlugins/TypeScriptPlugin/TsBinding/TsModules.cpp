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

  if (m_Duk.ExecuteString("var __AllComponents = require(\"./ez/AllComponents\");").Failed())
  {
    ezLog::Error("Failed to import 'AllComponents.ts'");
    return EZ_FAILURE;
  }

  if (m_Duk.ExecuteString("var __AllMessages = require(\"./ez/AllMessages\");").Failed())
  {
    ezLog::Error("Failed to import 'AllMessages.ts'");
    return EZ_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Log = require(\"./ez/Log\");").Failed())
  {
    ezLog::Error("Failed to import 'Log.ts'");
    return EZ_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Vec3 = require(\"./ez/Vec3\");").Failed())
  {
    ezLog::Error("Failed to import 'Vec3.ts'");
    return EZ_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Quat = require(\"./ez/Quat\");").Failed())
  {
    ezLog::Error("Failed to import 'Quat.ts'");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezTypeScriptBinding::SetModuleSearchPath(const char* szPath)
{
  m_Duk.StoreStringInStash("ModuleSearchPath", szPath);
}

int ezTypeScriptBinding::DukSearchModule(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, +1);

  ezTypeScriptTranspiler* pTranspiler = static_cast<ezTypeScriptTranspiler*>(duk.RetrievePointerFromStash("Transpiler"));

  ezStringBuilder sRequestedFile = duk.GetStringValue(0);

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
    duk.PushUndefined();
    duk.Error(ezFmt("'required' module \"{}\" could not be found/transpiled.", sRequestedFile));
    return duk.ReturnCustom();
  }

  return duk.ReturnString(sTranspiledCode);
}
