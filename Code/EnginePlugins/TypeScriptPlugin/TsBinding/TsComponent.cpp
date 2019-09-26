#include <TypeScriptPluginPCH.h>

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Component_GetOwner(duk_context* pContext);

ezResult ezTypeScriptBinding::Init_Component()
{
  m_Duk.RegisterFunction("__CPP_Component_GetOwner", __CPP_Component_GetOwner, 1);

  return EZ_SUCCESS;
}

static int __CPP_Component_GetOwner(duk_context* pContext)
{
  ezDuktapeFunction duk(pContext);

  duk_require_object(duk, 0);
  duk_get_prop_string(duk, 0, "ezComponentPtr");
  ezComponent* pComponent = (ezComponent*)duk_get_pointer(duk, -1);
  duk_pop(duk);

  //ezLog::Info("ezTsComponent::GetOwner -> {}", pComponent->GetOwner()->GetName());

  // create ezTsGameObject and store ezGameObject Ptr and Handle in it
  duk.OpenGlobalObject();
  EZ_VERIFY(duk.OpenObject("__GameObject").Succeeded(), "");
  EZ_VERIFY(duk.BeginFunctionCall("__TS_CreateGameObject").Succeeded(), "");
  EZ_VERIFY(duk.ExecuteFunctionCall().Succeeded(), "");
  duk_dup_top(duk);
  duk.EndFunctionCall();

  {
    duk_push_pointer(duk, pComponent->GetOwner()); // TODO: use handle
    duk_put_prop_string(duk, -2, "ezGameObjectPtr");
  }

  {
    ezGameObjectHandle* pHandleBuffer = reinterpret_cast<ezGameObjectHandle*>(duk_push_fixed_buffer(duk, sizeof(ezGameObjectHandle)));
    *pHandleBuffer = pComponent->GetOwner()->GetHandle();
    duk_put_prop_string(duk, -2, "ezGameObjectHandle");
  }

  return duk.ReturnCustom();
}
