#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
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

int TS_GameObjectMove(duk_context* pContext)
{
  ezDuktapeFunction wrapper(pContext);
  EZ_VERIFY(wrapper.IsParameterObject(0), "");

  duk_get_prop_string(pContext, 0, "goPtr");
  ezGameObject* pGo = (ezGameObject*)duk_get_pointer_default(pContext, -1, nullptr);

  if (pGo)
  {
    ezVec3 pos(0, 0, 10);
    pos.x = ezMath::Sin(ezAngle::Degree(ezTime::Now().GetSeconds() * 90)) * 5;
    pGo->SetLocalPosition(pos);
  }

  return wrapper.ReturnVoid();
}

static ezWorld* s_pWorld = nullptr;
static ezTypeScriptComponent* s_pTsComponent = nullptr;

int TS_GetGameObject(duk_context* pContext)
{
  ezDuktapeFunction wrapper(pContext);

  wrapper.BeginFunctionCall("_CreateGameObject");
  wrapper.ExecuteFunctionCall();
  EZ_VERIFY(wrapper.IsReturnValueObject(), "");

  duk_dup_top(pContext);

  wrapper.EndFunctionCall();

  duk_push_pointer(pContext, s_pTsComponent->GetOwner());
  duk_put_prop_string(pContext, -2, "goPtr");

  return wrapper.ReturnCustom();
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
  m_Script.RegisterFunction("GetGameObject", TS_GetGameObject, 0);
  m_Script.RegisterFunction("GameObjectMove", TS_GameObjectMove, 1);
}

void ezTypeScriptComponentManager::Deinitialize()
{
  SUPER::Deinitialize();
}

void ezTypeScriptComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  s_pWorld = GetWorld();

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndSimulating())
    {
      s_pTsComponent = &(*it);
      it->Update(m_Script);
    }
  }
}
