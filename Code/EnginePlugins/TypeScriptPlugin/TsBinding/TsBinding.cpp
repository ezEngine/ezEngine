#include <TypeScriptPluginPCH.h>

#include <Core/World/World.h>
#include <Duktape/duktape.h>
#include <Foundation/Profiling/Profiling.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

ezTypeScriptBinding::ezTypeScriptBinding()
  : m_Duk("Typescript Binding")
{
}

ezTypeScriptBinding::~ezTypeScriptBinding() = default;

ezResult ezTypeScriptBinding::Initialize(ezTypeScriptTranspiler& transpiler, ezWorld& world)
{
  EZ_LOG_BLOCK("Initialize TypeScript Binding");
  EZ_PROFILE_SCOPE("Initialize TypeScript Binding");

  m_pTranspiler = &transpiler;

  m_Duk.EnableModuleSupport(&ezTypeScriptBinding::DukSearchModule);
  m_Duk.StorePointerInStash("Transpiler", m_pTranspiler);

  StoreWorld(&world);

  SetModuleSearchPath("TypeScript");

  EZ_SUCCEED_OR_RETURN(Init_RequireModules());
  EZ_SUCCEED_OR_RETURN(Init_Log());
  EZ_SUCCEED_OR_RETURN(Init_GameObject());
  EZ_SUCCEED_OR_RETURN(Init_Component());

  m_bInitialized = true;
  return EZ_SUCCESS;
}

ezResult ezTypeScriptBinding::LoadComponent(const char* szComponent)
{
  if (!m_bInitialized)
  {
    return EZ_FAILURE;
  }

  if (m_LoadedComponents.Contains(szComponent))
  {
    return m_LoadedComponents[szComponent] ? EZ_SUCCESS : EZ_FAILURE;
  }

  EZ_PROFILE_SCOPE("Load TypeScript Component");

  m_LoadedComponents[szComponent] = false;

  ezStringBuilder transpiledCode;
  EZ_SUCCEED_OR_RETURN(m_pTranspiler->TranspileFileAndStoreJS(szComponent, transpiledCode));

  EZ_SUCCEED_OR_RETURN(m_Duk.ExecuteString(transpiledCode, szComponent));

  m_LoadedComponents[szComponent] = true;
  return EZ_SUCCESS;
}
