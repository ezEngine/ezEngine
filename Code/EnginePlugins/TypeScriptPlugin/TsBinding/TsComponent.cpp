#include <TypeScriptPluginPCH.h>

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Component_GetOwner(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_Component()
{
  m_Duk.RegisterFunction("__CPP_Component_GetOwner", __CPP_Component_GetOwner, 1);

  return EZ_SUCCESS;
}

static int __CPP_Component_GetOwner(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  duk_require_object(duk, 0);
  duk_get_prop_string(duk, 0, "ezComponentPtr");
  ezComponent* pComponent = (ezComponent*)duk_get_pointer(duk, -1);
  duk_pop(duk);

  ezTypeScriptBinding::DukPutGameObject(duk, pComponent->GetOwner()->GetHandle());

  return duk.ReturnCustom();
}
