#include <TypeScriptPluginPCH.h>

#include <Core/World/Component.h>
#include <Duktape/duktape.h>
#include <Foundation/Types/ScopeExit.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Component_GetOwner(duk_context* pDuk);
static int __CPP_Component_SetActive(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_Component()
{
  m_Duk.RegisterGlobalFunction("__CPP_Component_GetOwner", __CPP_Component_GetOwner, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Component_SetActive", __CPP_Component_SetActive, 2);

  return EZ_SUCCESS;
}

ezResult ezTypeScriptBinding::CreateTsComponent(duk_context* pDuk, const char* szTypeName, const ezComponentHandle& hCppComponent, const char* szDebugString)
{
  ezDuktapeHelper duk(pDuk, 0);

  ezStringBuilder sTypeName = szTypeName;

  duk.PushGlobalObject(); // [ global ]

  bool bCloseAllComps = false;
  if (sTypeName.TrimWordStart("ez"))
  {
    EZ_SUCCEED_OR_RETURN(duk.PushLocalObject("__AllComponents")); // [ global __AllComponents ]
    bCloseAllComps = true;
  }

  duk_get_prop_string(duk, -1, sTypeName); // [ global __AllComponents sTypeName ]
  duk_new(duk, 0);                         // [ global __AllComponents instance ]

  // store C++ side component handle in obj as property
  {
    ezComponentHandle* pBuffer = reinterpret_cast<ezComponentHandle*>(duk_push_fixed_buffer(duk, sizeof(ezComponentHandle))); // [ global __AllComponents instance buffer ]
    *pBuffer = hCppComponent;
    duk_put_prop_index(duk, -2, ezTypeScriptBindingIndexProperty::ComponentHandle); // [ global __AllComponents instance ]
  }

  // store reference to component in the global stash
  {
    const ezUInt32 uiComponentReference = hCppComponent.GetInternalID().m_Data;

    duk.PushGlobalStash();                                       // [ global __AllComponents instance stash]
    duk.PushUInt(uiComponentReference);                          // [ global __AllComponents instance stash uint ]
    duk_dup(duk, -3);                                            // [ global __AllComponents instance stash uint instance ]
    EZ_VERIFY(duk_put_prop(duk, -3), "Storing property failed"); // [ global __AllComponents instance stash ]
    duk.PopStack();                                              // [ global __AllComponents instance ]
  }


  if (bCloseAllComps)
  {
    duk.PopStack(3); // [ global __AllComponents instance ] -> [ ]
  }
  else
  {
    duk.PopStack(2); // [ global instance ] -> [ ]
  }

  return EZ_SUCCESS;
}

void ezTypeScriptBinding::DukPutComponentObject(duk_context* pDuk, const ezComponentHandle& hComponent)
{
  ezDuktapeHelper duk(pDuk, +1);

  duk_push_global_stash(pDuk); // [ stash ]

  const ezUInt32 uiComponentReference = hComponent.GetInternalID().m_Data;
  duk_push_uint(pDuk, uiComponentReference); // [ stash key ]
  if (!duk_get_prop(pDuk, -2))               // [ stash obj/undef ]
  {
    duk_pop_2(pDuk);     // [ ]
    duk_push_null(pDuk); // [ null ]
  }
  else // [ stash obj ]
  {
    duk_replace(pDuk, -2); // [ obj ]
  }
}

void ezTypeScriptBinding::DukPutComponentObject(duk_context* pDuk, ezComponent* pComponent)
{
  if (pComponent == nullptr)
  {
    duk_push_null(pDuk);
  }
  else
  {
    CreateTsComponent(pDuk, pComponent->GetDynamicRTTI()->GetTypeName(), pComponent->GetHandle(), "");
    DukPutComponentObject(pDuk, pComponent->GetHandle());
  }
}


void ezTypeScriptBinding::DeleteTsComponent(const ezComponentHandle& hCppComponent)
{
  const ezUInt32 uiComponentReference = hCppComponent.GetInternalID().m_Data;

  ezDuktapeHelper validator(m_Duk, 0);

  m_Duk.PushGlobalStash();
  duk_push_uint(m_Duk, uiComponentReference);
  EZ_VERIFY(duk_del_prop(m_Duk, -2), "Could not delete property");
  m_Duk.PopStack();
}

ezComponentHandle ezTypeScriptBinding::RetrieveComponentHandle(duk_context* pDuk, ezInt32 iObjIdx /*= 0 */)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return ezComponentHandle();

  ezDuktapeHelper duk(pDuk, 0);

  if (duk_get_prop_index(pDuk, iObjIdx, ezTypeScriptBindingIndexProperty::ComponentHandle))
  {
    ezComponentHandle hComponent = *reinterpret_cast<ezComponentHandle*>(duk_get_buffer(pDuk, -1, nullptr));
    duk_pop(pDuk);
    return hComponent;
  }

  return ezComponentHandle();
}

static int __CPP_Component_GetOwner(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, +1);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  ezTypeScriptBinding::DukPutGameObject(duk, pComponent->GetOwner()->GetHandle());

  return duk.ReturnCustom();
}

static int __CPP_Component_SetActive(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, 0);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  pComponent->SetActive(duk.GetBoolValue(1, true));

  return duk.ReturnVoid();
}
