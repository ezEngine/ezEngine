#include <TypeScriptPluginPCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Duktape/duktape.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_GameObject_IsValid(duk_context* pDuk);
static int __CPP_GameObject_SetX_Vec3(duk_context* pDuk);
static int __CPP_GameObject_GetX_Vec3(duk_context* pDuk);
static int __CPP_GameObject_SetX_Float(duk_context* pDuk);
static int __CPP_GameObject_GetX_Float(duk_context* pDuk);
static int __CPP_GameObject_SetX_Quat(duk_context* pDuk);
static int __CPP_GameObject_GetX_Quat(duk_context* pDuk);
static int __CPP_GameObject_SetX_Bool(duk_context* pDuk);
static int __CPP_GameObject_GetX_Bool(duk_context* pDuk);
static int __CPP_GameObject_FindChildByName(duk_context* pDuk);
static int __CPP_GameObject_FindChildByPath(duk_context* pDuk);
static int __CPP_GameObject_TryGetComponentOfBaseTypeName(duk_context* pDuk);
static int __CPP_GameObject_TryGetComponentOfBaseTypeNameHash(duk_context* pDuk);
static int __CPP_GameObject_SearchForChildByNameSequence(duk_context* pDuk);
static int __CPP_GameObject_SendMessage(duk_context* pDuk);
static int __CPP_GameObject_SetName(duk_context* pDuk);
static int __CPP_GameObject_GetName(duk_context* pDuk);
static int __CPP_GameObject_SetTeamID(duk_context* pDuk);
static int __CPP_GameObject_GetTeamID(duk_context* pDuk);
static int __CPP_GameObject_ChangeTags(duk_context* pDuk);
static int __CPP_GameObject_CheckTags(duk_context* pDuk);

enum GameObject_X
{
  LocalPosition,
  GlobalPosition,
  LocalScaling,
  GlobalScaling,
  LocalUniformScaling,
  LocalRotation,
  GlobalRotation,
  GlobalDirForwards,
  GlobalDirRight,
  GlobalDirUp,
  Velocity,
  Active,
};

ezResult ezTypeScriptBinding::Init_GameObject()
{
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_IsValid", __CPP_GameObject_IsValid, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetLocalPosition", __CPP_GameObject_SetX_Vec3, 2, GameObject_X::LocalPosition);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetLocalPosition", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::LocalPosition);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetGlobalPosition", __CPP_GameObject_SetX_Vec3, 2, GameObject_X::GlobalPosition);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalPosition", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalPosition);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetLocalScaling", __CPP_GameObject_SetX_Vec3, 2, GameObject_X::LocalScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetLocalScaling", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::LocalScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetGlobalScaling", __CPP_GameObject_SetX_Vec3, 2, GameObject_X::GlobalScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalScaling", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetLocalUniformScaling", __CPP_GameObject_SetX_Float, 2, GameObject_X::LocalUniformScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetLocalUniformScaling", __CPP_GameObject_GetX_Float, 1, GameObject_X::LocalUniformScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetLocalRotation", __CPP_GameObject_SetX_Quat, 2, GameObject_X::LocalRotation);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetLocalRotation", __CPP_GameObject_GetX_Quat, 1, GameObject_X::LocalRotation);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetGlobalRotation", __CPP_GameObject_SetX_Quat, 2, GameObject_X::GlobalRotation);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalRotation", __CPP_GameObject_GetX_Quat, 1, GameObject_X::GlobalRotation);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetActive", __CPP_GameObject_SetX_Bool, 2, GameObject_X::Active);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_IsActive", __CPP_GameObject_GetX_Bool, 1, GameObject_X::Active);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_FindChildByName", __CPP_GameObject_FindChildByName, 3);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_FindChildByPath", __CPP_GameObject_FindChildByPath, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_TryGetComponentOfBaseTypeName", __CPP_GameObject_TryGetComponentOfBaseTypeName, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_TryGetComponentOfBaseTypeNameHash", __CPP_GameObject_TryGetComponentOfBaseTypeNameHash, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SearchForChildByNameSequence", __CPP_GameObject_SearchForChildByNameSequence, 3);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SendMessage", __CPP_GameObject_SendMessage, 5, 0);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_PostMessage", __CPP_GameObject_SendMessage, 5, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalDirForwards", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalDirForwards);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalDirRight", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalDirRight);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalDirUp", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalDirUp);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetVelocity", __CPP_GameObject_SetX_Vec3, 2, GameObject_X::Velocity);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetVelocity", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::Velocity);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetName", __CPP_GameObject_SetName, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetName", __CPP_GameObject_GetName, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetTeamID", __CPP_GameObject_SetTeamID, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetTeamID", __CPP_GameObject_GetTeamID, 1);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_GameObject_SetTags", __CPP_GameObject_ChangeTags, 0);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_GameObject_AddTags", __CPP_GameObject_ChangeTags, 1);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_GameObject_RemoveTags", __CPP_GameObject_ChangeTags, 2);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_GameObject_HasAnyTags", __CPP_GameObject_CheckTags, 0);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_GameObject_HasAllTags", __CPP_GameObject_CheckTags, 1);

  return EZ_SUCCESS;
}

ezGameObjectHandle ezTypeScriptBinding::RetrieveGameObjectHandle(duk_context* pDuk, ezInt32 iObjIdx /*= 0 */)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return ezGameObjectHandle();

  if (duk_get_prop_index(pDuk, iObjIdx, ezTypeScriptBindingIndexProperty::GameObjectHandle))
  {
    ezGameObjectHandle hObject = *reinterpret_cast<ezGameObjectHandle*>(duk_get_buffer(pDuk, -1, nullptr));
    duk_pop(pDuk);
    return hObject;
  }

  return ezGameObjectHandle();
}

ezGameObject* ezTypeScriptBinding::ExpectGameObject(duk_context* pDuk, ezInt32 iObjIdx /*= 0*/)
{
  ezGameObjectHandle hObject = ezTypeScriptBinding::RetrieveGameObjectHandle(pDuk, 0 /*this*/);

  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(pDuk);

  ezGameObject* pGameObject = nullptr;
  EZ_VERIFY(pWorld->TryGetObject(hObject, pGameObject), "Invalid ezGameObject");

  return pGameObject;
}

bool ezTypeScriptBinding::RegisterGameObject(ezGameObjectHandle handle, ezUInt32& out_uiStashIdx)
{
  if (handle.IsInvalidated())
    return false;

  ezUInt32& uiStashIdx = m_GameObjectToStashIdx[handle];

  if (uiStashIdx != 0)
  {
    out_uiStashIdx = uiStashIdx;
    return true;
  }

  if (!m_FreeStashObjIdx.IsEmpty())
  {
    uiStashIdx = m_FreeStashObjIdx.PeekBack();
    m_FreeStashObjIdx.PopBack();
  }
  else
  {
    uiStashIdx = m_uiNextStashObjIdx;
    ++m_uiNextStashObjIdx;
  }

  ezDuktapeHelper duk(m_Duk);                                     // [ ]
  duk.PushGlobalObject();                                         // [ global ]
  EZ_VERIFY(duk.PushLocalObject("__GameObject").Succeeded(), ""); // [ global __GameObject ]
  duk_get_prop_string(duk, -1, "GameObject");                     // [ global __GameObject GameObject ]

  duk_new(duk, 0); // [ global __GameObject object ]

  // set the ezGameObjectHandle property
  {
    ezGameObjectHandle* pHandleBuffer = reinterpret_cast<ezGameObjectHandle*>(duk_push_fixed_buffer(duk, sizeof(ezGameObjectHandle))); // [ global __GameObject object buffer ]
    *pHandleBuffer = handle;
    duk_put_prop_index(duk, -2, ezTypeScriptBindingIndexProperty::GameObjectHandle); // [ global __GameObject object ]
  }

  StoreReferenceInStash(duk, uiStashIdx); // [ global __GameObject object ]
  duk.PopStack(3);                        // [ ]

  out_uiStashIdx = uiStashIdx;
  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
}

bool ezTypeScriptBinding::DukPutGameObject(const ezGameObjectHandle& hObject)
{
  ezDuktapeHelper duk(m_Duk);

  ezUInt32 uiStashIdx = 0;
  if (!RegisterGameObject(hObject, uiStashIdx))
  {
    duk.PushNull(); // [ null ]
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, false, +1);
  }

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, DukPushStashObject(duk, uiStashIdx), +1);
}

void ezTypeScriptBinding::DukPutGameObject(const ezGameObject* pObject)
{
  if (pObject == nullptr)
  {
    duk_push_null(m_Duk);
  }
  else
  {
    DukPutGameObject(pObject->GetHandle());
  }
}

static int __CPP_GameObject_IsValid(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObjectHandle hObject = ezTypeScriptBinding::RetrieveGameObjectHandle(pDuk, 0 /*this*/);

  if (hObject.IsInvalidated())
  {
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(false), +1);
  }

  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(pDuk);

  ezGameObject* pGameObject = nullptr;
  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pWorld->TryGetObject(hObject, pGameObject)), +1);
}

static int __CPP_GameObject_SetX_Vec3(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  if (!pGameObject->IsDynamic())
  {
    ezLog::SeriousWarning("TypeScript component modifies transform of static game-object '{}'. Use 'Force Dynamic' mode on owner game-object.", pGameObject->GetName());
    pGameObject->MakeDynamic();
  }

  const ezVec3 value = ezTypeScriptBinding::GetVec3(pDuk, 1);

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalPosition:
      pGameObject->SetLocalPosition(value);
      break;

    case GameObject_X::GlobalPosition:
      pGameObject->SetGlobalPosition(value);
      break;

    case GameObject_X::LocalScaling:
      pGameObject->SetLocalScaling(value);
      break;

    case GameObject_X::GlobalScaling:
      pGameObject->SetGlobalScaling(value);
      break;

    case GameObject_X::Velocity:
      pGameObject->SetVelocity(value);
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetX_Vec3(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  ezVec3 value;

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalPosition:
      value = pGameObject->GetLocalPosition();
      break;

    case GameObject_X::GlobalPosition:
      value = pGameObject->GetGlobalPosition();
      break;

    case GameObject_X::LocalScaling:
      value = pGameObject->GetLocalScaling();
      break;

    case GameObject_X::GlobalScaling:
      value = pGameObject->GetGlobalScaling();
      break;

    case GameObject_X::GlobalDirForwards:
      value = pGameObject->GetGlobalDirForwards();
      break;

    case GameObject_X::GlobalDirRight:
      value = pGameObject->GetGlobalDirRight();
      break;

    case GameObject_X::GlobalDirUp:
      value = pGameObject->GetGlobalDirUp();
      break;

    case GameObject_X::Velocity:
      value = pGameObject->GetVelocity();
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  ezTypeScriptBinding::PushVec3(pDuk, value);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_SetX_Float(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  if (!pGameObject->IsDynamic())
  {
    ezLog::SeriousWarning("TypeScript component modifies transform of static game-object '{}'. Use 'Force Dynamic' mode on owner game-object.", pGameObject->GetName());
    pGameObject->MakeDynamic();
  }

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalUniformScaling:
      pGameObject->SetLocalUniformScaling(duk.GetFloatValue(1));
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetX_Float(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  float value = 0.0f;

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalUniformScaling:
      value = pGameObject->GetLocalUniformScaling();
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnFloat(value), +1);
}

static int __CPP_GameObject_SetX_Quat(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  if (!pGameObject->IsDynamic())
  {
    ezLog::SeriousWarning("TypeScript component modifies transform of static game-object '{}'. Use 'Force Dynamic' mode on owner game-object.", pGameObject->GetName());
    pGameObject->MakeDynamic();
  }

  const ezQuat rot = ezTypeScriptBinding::GetQuat(pDuk, 1);

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalRotation:
      pGameObject->SetLocalRotation(rot);
      break;

    case GameObject_X::GlobalRotation:
      pGameObject->SetGlobalRotation(rot);
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetX_Quat(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  ezQuat value;

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalRotation:
      value = pGameObject->GetLocalRotation();
      break;

    case GameObject_X::GlobalRotation:
      pGameObject->GetGlobalRotation();
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  ezTypeScriptBinding::PushQuat(pDuk, value);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_SetX_Bool(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const bool value = duk.GetBoolValue(1, true);

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::Active:
      pGameObject->SetActive(value);
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetX_Bool(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  bool value = false;

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::Active:
      value = pGameObject->IsActive();
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(value), +1);
}

static int __CPP_GameObject_FindChildByName(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const char* szName = duk.GetStringValue(1);
  bool bRecursive = duk.GetBoolValue(2, true);

  ezGameObject* pChild = pGameObject->FindChildByName(ezTempHashedString(szName), bRecursive);

  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pChild);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_FindChildByPath(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const char* szPath = duk.GetStringValue(1);

  ezGameObject* pChild = pGameObject->FindChildByPath(szPath);

  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pChild);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_TryGetComponentOfBaseTypeName(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const char* szTypeName = duk.GetStringValue(1);

  const ezRTTI* pRtti = ezRTTI::FindTypeByName(szTypeName);
  if (pRtti == nullptr)
  {
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnNull(), +1);
  }

  ezComponent* pComponent = nullptr;
  if (!pGameObject->TryGetComponentOfBaseType(pRtti, pComponent))
  {
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnNull(), +1);
  }

  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(duk);
  pBinding->DukPutComponentObject(pComponent);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_TryGetComponentOfBaseTypeNameHash(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  ezUInt32 uiTypeNameHash = duk.GetUIntValue(1);

  const ezRTTI* pRtti = ezRTTI::FindTypeByNameHash(uiTypeNameHash);
  if (pRtti == nullptr)
  {
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnNull(), +1);
  }

  ezComponent* pComponent = nullptr;
  if (!pGameObject->TryGetComponentOfBaseType(pRtti, pComponent))
  {
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnNull(), +1);
  }

  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(duk);
  pBinding->DukPutComponentObject(pComponent);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_SearchForChildByNameSequence(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const ezUInt32 uiTypeNameHash = duk.GetUIntValue(2);

  const ezRTTI* pRtti = nullptr;

  if (uiTypeNameHash != 0)
  {
    pRtti = ezRTTI::FindTypeByNameHash(uiTypeNameHash);
  }

  ezGameObject* pObject = pGameObject->SearchForChildByNameSequence(duk.GetStringValue(1), pRtti);

  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pObject);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_SendMessage(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(duk);

  if (duk.GetFunctionMagicValue() == 0) // SendMessage
  {
    ezUniquePtr<ezMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, ezTime::Zero());

    if (duk.GetBoolValue(3))
      pGameObject->SendMessageRecursive(*pMsg);
    else
      pGameObject->SendMessage(*pMsg);

    if (duk.GetBoolValue(4)) // expect the message to have result values
    {
      // sync msg back to TS
      ezTypeScriptBinding::SyncEzObjectToTsObject(pDuk, pMsg->GetDynamicRTTI(), pMsg.Borrow(), 1);
    }
  }
  else // PostMessage
  {
    const ezTime delay = ezTime::Seconds(duk.GetNumberValue(4));

    ezUniquePtr<ezMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, delay);

    if (duk.GetBoolValue(3))
      pGameObject->PostMessageRecursive(*pMsg, ezObjectMsgQueueType::NextFrame, delay);
    else
      pGameObject->PostMessage(*pMsg, ezObjectMsgQueueType::NextFrame, delay);
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_SetName(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  pGameObject->SetName(duk.GetStringValue(1));

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetName(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  return duk.ReturnString(pGameObject->GetName());
}

static int __CPP_GameObject_SetTeamID(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  pGameObject->SetTeamID(duk.GetUIntValue(1));

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetTeamID(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnUInt(pGameObject->GetTeamID()), +1);
}

static int __CPP_GameObject_ChangeTags(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const ezUInt32 uiMagic = duk.GetFunctionMagicValue();

  if (uiMagic == 0) // SetTags
  {
    pGameObject->GetTags().Clear();
  }

  for (ezUInt32 i = 1; i < duk.GetNumVarArgFunctionParameters(); ++i)
  {
    const char* szParam = duk.GetStringValue(i, nullptr);

    if (!ezStringUtils::IsNullOrEmpty(szParam))
    {
      switch (uiMagic)
      {
        case 0: // SetTags
        case 1: // AddTags
          pGameObject->GetTags().SetByName(szParam);
          break;

        case 2: // RemoveTags
          pGameObject->GetTags().RemoveByName(szParam);
          break;

        default:
          EZ_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_GameObject_CheckTags(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const ezUInt32 uiMagic = duk.GetFunctionMagicValue();

  bool result = false;

  for (ezUInt32 i = 1; i < duk.GetNumVarArgFunctionParameters(); ++i)
  {
    const char* szParam = duk.GetStringValue(i, nullptr);

    if (!ezStringUtils::IsNullOrEmpty(szParam))
    {
      const bool bIsSet = pGameObject->GetTags().IsSetByName(szParam);

      if (uiMagic == 0) // HasAnyTag
      {
        if (bIsSet)
        {
          result = true;
          break;
        }
      }
      else if (uiMagic == 1) // HasAllTags
      {
        if (!bIsSet)
        {
          result = false;
          break;
        }
      }
      else
      {
        EZ_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(result), +1);
}
