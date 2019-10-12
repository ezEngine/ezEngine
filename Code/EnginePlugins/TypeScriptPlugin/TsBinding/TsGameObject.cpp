#include <TypeScriptPluginPCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_GameObject_IsValid(duk_context* pDuk);
static int __CPP_GameObject_SetLocalPosition(duk_context* pDuk);
static int __CPP_GameObject_GetLocalPosition(duk_context* pDuk);
static int __CPP_GameObject_SetLocalRotation(duk_context* pDuk);
static int __CPP_GameObject_GetLocalRotation(duk_context* pDuk);
static int __CPP_GameObject_SetActive(duk_context* pDuk);
static int __CPP_GameObject_FindChildByName(duk_context* pDuk);
static int __CPP_GameObject_FindComponentByTypeName(duk_context* pDuk);
static int __CPP_GameObject_FindComponentByTypeNameHash(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_GameObject()
{
  m_Duk.RegisterFunction("__CPP_GameObject_IsValid", __CPP_GameObject_IsValid, 1);
  m_Duk.RegisterFunction("__CPP_GameObject_SetLocalPosition", __CPP_GameObject_SetLocalPosition, 2);
  m_Duk.RegisterFunction("__CPP_GameObject_GetLocalPosition", __CPP_GameObject_GetLocalPosition, 1);
  m_Duk.RegisterFunction("__CPP_GameObject_SetLocalRotation", __CPP_GameObject_SetLocalRotation, 2);
  m_Duk.RegisterFunction("__CPP_GameObject_GetLocalRotation", __CPP_GameObject_GetLocalRotation, 1);
  m_Duk.RegisterFunction("__CPP_GameObject_SetActive", __CPP_GameObject_SetActive, 2);
  m_Duk.RegisterFunction("__CPP_GameObject_FindChildByName", __CPP_GameObject_FindChildByName, 2);
  m_Duk.RegisterFunction("__CPP_GameObject_FindComponentByTypeName", __CPP_GameObject_FindComponentByTypeName, 2);
  m_Duk.RegisterFunction("__CPP_GameObject_FindComponentByTypeNameHash", __CPP_GameObject_FindComponentByTypeNameHash, 2);

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

  if (hObject.IsInvalidated())
  {
    duk_push_null(pDuk);
    return;
  }

  // TODO: make this more efficient by reusing previous ez.GameObject instances when possible

  // create ez.GameObject and store the ezGameObjectHandle as a property in it
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

void ezTypeScriptBinding::DukPutGameObject(duk_context* pDuk, const ezGameObject* pObject)
{
  if (pObject == nullptr)
  {
    duk_push_null(pDuk);
  }
  else
  {
    DukPutGameObject(pDuk, pObject->GetHandle());
  }
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

static int __CPP_GameObject_GetLocalPosition(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  ezTypeScriptBinding::PushVec3(pDuk, pGameObject->GetLocalPosition());

  return duk.ReturnCustom();
}

static int __CPP_GameObject_SetLocalRotation(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const ezQuat rot = ezTypeScriptBinding::GetQuat(pDuk, 1);

  pGameObject->SetLocalRotation(rot);

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetLocalRotation(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  ezTypeScriptBinding::PushQuat(pDuk, pGameObject->GetLocalRotation());

  return duk.ReturnCustom();
}

static int __CPP_GameObject_SetActive(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const bool bActive = duk.GetBoolParameter(1, true);

  if (bActive)
    pGameObject->Activate();
  else
    pGameObject->Deactivate();

  return duk.ReturnVoid();
}

static int __CPP_GameObject_FindChildByName(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const char* szName = duk.GetStringParameter(1);
  bool bRecursive = duk.GetBoolParameter(2, true);

  ezGameObject* pChild = pGameObject->FindChildByName(ezTempHashedString(szName), bRecursive);

  ezTypeScriptBinding::DukPutGameObject(duk, pChild);

  return duk.ReturnCustom();
}

static int __CPP_GameObject_FindComponentByTypeName(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const char* szTypeName = duk.GetStringParameter(1);

  const ezRTTI* pRtti = ezRTTI::FindTypeByName(szTypeName);
  if (pRtti == nullptr)
  {
    return duk.ReturnNull();
  }

  ezComponent* pComponent = nullptr;
  if (!pGameObject->TryGetComponentOfBaseType(pRtti, pComponent))
  {
    return duk.ReturnNull();
  }

  ezTypeScriptBinding::DukPutComponentObject(duk, pComponent);

  return duk.ReturnCustom();
}

static int __CPP_GameObject_FindComponentByTypeNameHash(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  ezUInt32 uiTypeNameHash = duk.GetUIntParameter(1);

  const ezRTTI* pRtti = ezRTTI::FindTypeByNameHash(uiTypeNameHash);
  if (pRtti == nullptr)
  {
    return duk.ReturnNull();
  }

  ezComponent* pComponent = nullptr;
  if (!pGameObject->TryGetComponentOfBaseType(pRtti, pComponent))
  {
    return duk.ReturnNull();
  }

  ezTypeScriptBinding::DukPutComponentObject(duk, pComponent);

  return duk.ReturnCustom();
}
