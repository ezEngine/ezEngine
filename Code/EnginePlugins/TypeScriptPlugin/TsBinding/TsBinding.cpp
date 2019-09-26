#include <TypeScriptPluginPCH.h>

#include <Core/World/World.h>
#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

ezTypeScriptBinding::ezTypeScriptBinding()
  : m_Duk("Typescript Binding")
{
}

ezTypeScriptBinding::~ezTypeScriptBinding() = default;

void ezTypeScriptBinding::Initialize(ezTypeScriptTranspiler& transpiler, ezWorld& world)
{
  m_pTranspiler = &transpiler;

  m_Duk.EnableModuleSupport(&ezTypeScriptBinding::DukSearchModule);
  m_Duk.StorePointerInStash("Transpiler", m_pTranspiler);

  StoreWorld(&world);

  SetModuleSearchPath("TypeScript");
}

ezStatus ezTypeScriptBinding::SetupBinding()
{
  EZ_SUCCEED_OR_RETURN(Init_RequireModules());
  EZ_SUCCEED_OR_RETURN(Init_Log());
  EZ_SUCCEED_OR_RETURN(Init_GameObject());
  EZ_SUCCEED_OR_RETURN(Init_Component());

  ezStringBuilder js;
  EZ_SUCCEED_OR_RETURN(m_pTranspiler->TranspileFileAndStoreJS("TypeScript/Component.ts", js));

  EZ_SUCCEED_OR_RETURN(m_Duk.ExecuteString(js, "TypeScript/Component.ts"));

  return ezStatus(EZ_SUCCESS);
}
