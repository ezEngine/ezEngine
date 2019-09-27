#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Component_GetOwner(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_Component()
{
  m_Duk.RegisterFunction("__CPP_Component_GetOwner", __CPP_Component_GetOwner, 1);

  return EZ_SUCCESS;
}

ezResult ezTypeScriptBinding::CreateTsComponent(const char* szTypeName, const ezComponentHandle& hCppComponent, const char* szDebugString)
{
  ezDuktapeStackValidator validator(m_Duk);

  const ezStringBuilder sFactoryName("__TS_Create_", szTypeName);

  EZ_SUCCEED_OR_RETURN(m_Duk.BeginFunctionCall(sFactoryName));

  m_Duk.PushParameter(szDebugString);
  EZ_SUCCEED_OR_RETURN(m_Duk.ExecuteFunctionCall());

  // store C++ side component handle in obj as property
  ezComponentHandle* pBuffer = reinterpret_cast<ezComponentHandle*>(duk_push_fixed_buffer(m_Duk, sizeof(ezComponentHandle)));
  *pBuffer = hCppComponent;
  duk_put_prop_index(m_Duk, -2, ezTypeScriptBindingIndexProperty::ComponentHandle);

  {
    const ezUInt32 uiComponentReference = hCppComponent.GetInternalID().m_Data;

    m_Duk.OpenGlobalStashObject();
    duk_push_uint(m_Duk, uiComponentReference);
    duk_dup(m_Duk, -3); // duplicate component obj
    EZ_VERIFY(duk_put_prop(m_Duk, -3), "Storing property failed");
    m_Duk.CloseObject();
  }

  m_Duk.EndFunctionCall();

  return EZ_SUCCESS;
}

ezResult ezTypeScriptBinding::DukPutComponentObject(const ezComponentHandle& hComponent)
{
  ezDuktapeStackValidator validator(m_Duk, +1);

  duk_push_global_stash(m_Duk);

  const ezUInt32 uiComponentReference = hComponent.GetInternalID().m_Data;
  duk_push_uint(m_Duk, uiComponentReference);
  if (!duk_get_prop(m_Duk, -2))
  {
    // remove 'undefined' result from stack
    duk_pop(m_Duk);
    validator.AdjustExpected(-1);
    return EZ_FAILURE;
  }

  // remove stash object, keep result on top
  duk_replace(m_Duk, -2);

  return EZ_SUCCESS;
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
