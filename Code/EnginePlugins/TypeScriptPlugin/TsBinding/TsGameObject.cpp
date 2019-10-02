#include <TypeScriptPluginPCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_GameObject_IsValid(duk_context* pDuk);
static int __CPP_GameObject_SetLocalPosition(duk_context* pDuk);
static int __CPP_GameObject_SetLocalRotation(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_GameObject()
{
  m_Duk.RegisterFunction("__CPP_GameObject_IsValid", __CPP_GameObject_IsValid, 1);
  m_Duk.RegisterFunction("__CPP_GameObject_SetLocalPosition", __CPP_GameObject_SetLocalPosition, 2);
  m_Duk.RegisterFunction("__CPP_GameObject_SetLocalRotation", __CPP_GameObject_SetLocalRotation, 2);

  return EZ_SUCCESS;
}

ezGameObjectHandle ezTypeScriptBinding::RetrieveGameObjectHandle(duk_context* pDuk, ezInt32 iObjIdx /*= 0 */)
{
  duk_get_prop_string(pDuk, iObjIdx, "ezGameObjectHandle");
  ezGameObjectHandle hObject = *reinterpret_cast<ezGameObjectHandle*>(duk_get_buffer(pDuk, -1, nullptr));
  duk_pop(pDuk);

  return hObject;
}

ezGameObject* ezTypeScriptBinding::ExpectGameObject(duk_context* pDuk, ezInt32 iObjIdx /*= 0*/)
{
  ezGameObjectHandle hObject = ezTypeScriptBinding::RetrieveGameObjectHandle(pDuk, 0 /*this*/);

  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(pDuk);

  ezGameObject* pGameObject = nullptr;
  EZ_VERIFY(pWorld->TryGetObject(hObject, pGameObject), "Invalid ezGameObject");

  return pGameObject;
}

void ezTypeScriptBinding::DukPutGameObject(duk_context* pDuk, const ezGameObjectHandle& hObject)
{
  ezDuktapeWrapper duk(pDuk);
  ezDuktapeStackValidator validator(pDuk, +1);

  // create ez.GameObject and store ezGameObjectHandle as a property in it
  duk.OpenGlobalObject();
  EZ_VERIFY(duk.OpenObject("__GameObject").Succeeded(), "");
  EZ_VERIFY(duk.BeginFunctionCall("__TS_CreateGameObject").Succeeded(), "");
  EZ_VERIFY(duk.ExecuteFunctionCall().Succeeded(), "");
  // top of the stack (+3) contains our ez.GameObject now

  // set the ezGameObjectHandle property
  {
    ezGameObjectHandle* pHandleBuffer = reinterpret_cast<ezGameObjectHandle*>(duk_push_fixed_buffer(duk, sizeof(ezGameObjectHandle)));
    *pHandleBuffer = hObject;
    duk_put_prop_string(duk, -2, "ezGameObjectHandle");
  }

  // move the top of the stack to the only position that we want to keep (return)
  duk_replace(duk, -3);
  // remove the remaining element that is too much
  duk_pop(duk);
}

static int __CPP_GameObject_IsValid(duk_context* pDuk)
{
  ezGameObjectHandle hObject = ezTypeScriptBinding::RetrieveGameObjectHandle(pDuk, 0 /*this*/);

  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(pDuk);

  ezGameObject* pGameObject = nullptr;
  return pWorld->TryGetObject(hObject, pGameObject);
}

static int __CPP_GameObject_SetLocalPosition(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const ezVec3 pos = ezTypeScriptBinding::GetVec3(pDuk, 1);

  pGameObject->SetLocalPosition(pos);

  return duk.ReturnVoid();
}

static int __CPP_GameObject_SetLocalRotation(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const ezQuat rot = ezTypeScriptBinding::GetQuat(pDuk, 1);

  pGameObject->SetLocalRotation(rot);

  return duk.ReturnVoid();
}
