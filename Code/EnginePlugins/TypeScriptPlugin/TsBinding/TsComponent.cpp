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

  duk_get_prop_index(duk, 0, ezTypeScriptBindingIndexProperty::ComponentHandle);
  ezComponentHandle hOwnHandle = *reinterpret_cast<ezComponentHandle*>(duk_get_buffer(duk, -1, nullptr));
  duk_pop(duk);

  ezComponent* pComponent = nullptr;
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);
  EZ_VERIFY(pWorld->TryGetComponent(hOwnHandle, pComponent), "");

  ezTypeScriptBinding::DukPutGameObject(duk, pComponent->GetOwner()->GetHandle());

  return duk.ReturnCustom();
}
