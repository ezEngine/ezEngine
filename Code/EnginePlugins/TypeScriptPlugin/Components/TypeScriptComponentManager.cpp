#include <TypeScriptPluginPCH.h>

#include <TypeScriptPlugin/Components/TypeScriptComponent.h>

ezTypeScriptComponentManager::ezTypeScriptComponentManager(ezWorld* pWorld)
  : SUPER(pWorld)
  , m_Script("TypeScriptComponent")
{
}

ezTypeScriptComponentManager::~ezTypeScriptComponentManager()
{
}

int TS_ezLog_Info(duk_context* pContext)
{
  ezDuktapeFunction wrapper(pContext);
  ezLog::Info(wrapper.GetStringParameter(0));

  return wrapper.ReturnVoid();
}

void ezTypeScriptComponentManager::Initialize()
{
  SUPER::Initialize();

  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezTypeScriptComponentManager::Update, this);
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);

  EZ_VERIFY(m_Script.ExecuteFile("TypeScript/typescriptServices.js").Succeeded(), "");

  m_Script.RegisterFunction("ezLog_Info", TS_ezLog_Info, 1);
}

void ezTypeScriptComponentManager::Deinitialize()
{
  SUPER::Deinitialize();
}

void ezTypeScriptComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndSimulating())
      it->Update(m_Script);
  }
}
