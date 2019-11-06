#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

ezResult ezTypeScriptBinding::Init_RequireModules()
{
  EZ_LOG_BLOCK("Init_RequireModules");

  if (m_Duk.ExecuteString("var __GameObject = require(\"./TypeScript/ez/GameObject\");").Failed())
  {
    ezLog::Error("Failed to import 'GameObject.ts'");
    return EZ_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Component = require(\"./TypeScript/ez/Component\");").Failed())
  {
    ezLog::Error("Failed to import 'Component.ts'");
    return EZ_FAILURE;
  }

  if (m_Duk.ExecuteString("var __AllComponents = require(\"./TypeScript/ez/AllComponents\");").Failed())
  {
    ezLog::Error("Failed to import 'AllComponents.ts'");
    return EZ_FAILURE;
  }

  if (m_Duk.ExecuteString("var __AllMessages = require(\"./TypeScript/ez/AllMessages\");").Failed())
  {
    ezLog::Error("Failed to import 'AllMessages.ts'");
    return EZ_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Log = require(\"./TypeScript/ez/Log\");").Failed())
  {
    ezLog::Error("Failed to import 'Log.ts'");
    return EZ_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Vec2 = require(\"./TypeScript/ez/Vec2\");").Failed())
  {
    ezLog::Error("Failed to import 'Vec2.ts'");
    return EZ_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Vec3 = require(\"./TypeScript/ez/Vec3\");").Failed())
  {
    ezLog::Error("Failed to import 'Vec3.ts'");
    return EZ_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Quat = require(\"./TypeScript/ez/Quat\");").Failed())
  {
    ezLog::Error("Failed to import 'Quat.ts'");
    return EZ_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Color = require(\"./TypeScript/ez/Color\");").Failed())
  {
    ezLog::Error("Failed to import 'Color.ts'");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

int ezTypeScriptBinding::DukSearchModule(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, +1);

  ezStringBuilder sRequestedFile = duk.GetStringValue(0);
  if (!sRequestedFile.HasAnyExtension())
  {
    sRequestedFile.ChangeFileExtension("ts");
  }

  EZ_LOG_BLOCK("DukSearchModule", sRequestedFile);

  ezTypeScriptBinding* pBinding = static_cast<ezTypeScriptBinding*>(duk.RetrievePointerFromStash("ezTypeScriptBinding"));

  ezResourceLock<ezScriptCompendiumResource> pCompendium(pBinding->m_hScriptCompendium, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pCompendium.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    duk.PushUndefined();
    duk.Error(ezFmt("'required' module \"{}\" could not be loaded: JsLib resource is missing.", sRequestedFile));
    return duk.ReturnCustom();
  }

  auto it = pCompendium->GetDescriptor().m_PathToSource.Find(sRequestedFile);

  if (!it.IsValid())
  {
    duk.PushUndefined();
    duk.Error(ezFmt("'required' module \"{}\" could not be loaded: JsLib resource does not contain source for it.", sRequestedFile));
    return duk.ReturnCustom();
  }

  return duk.ReturnString(it.Value());
}
