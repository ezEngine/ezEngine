#include <TypeScriptPluginPCH.h>

#include <Core/World/Component.h>
#include <Duktape/duktape.h>
#include <Foundation/Types/ScopeExit.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Component_GetOwner(duk_context* pDuk);
static int __CPP_Component_SetActive(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_Component()
{
  m_Duk.RegisterFunction("__CPP_Component_GetOwner", __CPP_Component_GetOwner, 1);
  m_Duk.RegisterFunction("__CPP_Component_SetActive", __CPP_Component_SetActive, 2);

  return EZ_SUCCESS;
}

ezResult ezTypeScriptBinding::CreateTsComponent(duk_context* pDuk, const char* szTypeName, const ezComponentHandle& hCppComponent, const char* szDebugString)
{
  ezDuktapeStackValidator validator(pDuk);
  ezDuktapeWrapper duk(pDuk);

  ezStringBuilder sTypeName = szTypeName;

  bool bCloseAllComps = false;
  if (sTypeName.TrimWordStart("ez"))
  {
    EZ_SUCCEED_OR_RETURN(duk.OpenObject("__AllComponents"));
    bCloseAllComps = true;
  }

  EZ_SCOPE_EXIT(if (bCloseAllComps) duk.CloseObject());

  const ezStringBuilder sFactoryName("__TS_Create_", sTypeName);

  EZ_SUCCEED_OR_RETURN(duk.BeginFunctionCall(sFactoryName));

  duk.PushParameter(szDebugString);
  EZ_SUCCEED_OR_RETURN(duk.ExecuteFunctionCall());

  // store C++ side component handle in obj as property
  ezComponentHandle* pBuffer = reinterpret_cast<ezComponentHandle*>(duk_push_fixed_buffer(duk, sizeof(ezComponentHandle)));
  *pBuffer = hCppComponent;
  duk_put_prop_index(duk, -2, ezTypeScriptBindingIndexProperty::ComponentHandle);

  {
    const ezUInt32 uiComponentReference = hCppComponent.GetInternalID().m_Data;

    duk.OpenGlobalStashObject();
    duk_push_uint(duk, uiComponentReference);
    duk_dup(duk, -3); // duplicate component obj
    EZ_VERIFY(duk_put_prop(duk, -3), "Storing property failed");
    duk.CloseObject();
  }

  duk.EndFunctionCall();

  return EZ_SUCCESS;
}

void ezTypeScriptBinding::DukPutComponentObject(duk_context* pDuk, const ezComponentHandle& hComponent)
{
  ezDuktapeStackValidator validator(pDuk, +1);

  duk_push_global_stash(pDuk);

  const ezUInt32 uiComponentReference = hComponent.GetInternalID().m_Data;
  duk_push_uint(pDuk, uiComponentReference);
  if (!duk_get_prop(pDuk, -2))
  {
    // remove 'undefined' result from stack, replace it with null
    duk_pop(pDuk);
    duk_push_null(pDuk);
  }
  else
  {
    // remove stash object, keep result on top
    duk_replace(pDuk, -2);
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

  ezDuktapeStackValidator validator(m_Duk);

  m_Duk.OpenGlobalStashObject();
  duk_push_uint(m_Duk, uiComponentReference);
  EZ_VERIFY(duk_del_prop(m_Duk, -2), "Could not delete property");
  m_Duk.CloseObject();
}

ezComponentHandle ezTypeScriptBinding::RetrieveComponentHandle(duk_context* pDuk, ezInt32 iObjIdx /*= 0 */)
{
  ezDuktapeStackValidator validator(pDuk);

  duk_get_prop_index(pDuk, iObjIdx, ezTypeScriptBindingIndexProperty::ComponentHandle);
  ezComponentHandle hComponent = *reinterpret_cast<ezComponentHandle*>(duk_get_buffer(pDuk, -1, nullptr));
  duk_pop(pDuk);

  return hComponent;
}

static int __CPP_Component_GetOwner(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  ezTypeScriptBinding::DukPutGameObject(duk, pComponent->GetOwner()->GetHandle());

  return duk.ReturnCustom();
}

static int __CPP_Component_SetActive(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  pComponent->SetActive(duk.GetBoolParameter(1, true));

  return duk.ReturnVoid();
}
