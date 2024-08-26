#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/Messages/EventMessage.h>
#include <Core/World/Component.h>
#include <Duktape/duktape.h>
#include <Foundation/Types/ScopeExit.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Component_IsValid(duk_context* pDuk);
static int __CPP_Component_GetUniqueID(duk_context* pDuk);
static int __CPP_Component_GetOwner(duk_context* pDuk);
static int __CPP_Component_SetActiveFlag(duk_context* pDuk);
static int __CPP_Component_IsActive(duk_context* pDuk);
static int __CPP_Component_SendMessage(duk_context* pDuk);
static int __CPP_TsComponent_BroadcastEvent(duk_context* pDuk);
static int __CPP_TsComponent_SetTickInterval(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_Component()
{
  m_Duk.RegisterGlobalFunction("__CPP_Component_IsValid", __CPP_Component_IsValid, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Component_GetUniqueID", __CPP_Component_GetUniqueID, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Component_GetOwner", __CPP_Component_GetOwner, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Component_SetActiveFlag", __CPP_Component_SetActiveFlag, 2);
  m_Duk.RegisterGlobalFunction("__CPP_Component_GetActiveFlag", __CPP_Component_IsActive, 1, -1);
  m_Duk.RegisterGlobalFunction("__CPP_Component_IsActive", __CPP_Component_IsActive, 1, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Component_IsActiveAndInitialized", __CPP_Component_IsActive, 1, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Component_IsActiveAndSimulating", __CPP_Component_IsActive, 1, 2);
  m_Duk.RegisterGlobalFunction("__CPP_Component_SendMessage", __CPP_Component_SendMessage, 4, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Component_PostMessage", __CPP_Component_SendMessage, 4, 1);
  m_Duk.RegisterGlobalFunction("__CPP_TsComponent_BroadcastEvent", __CPP_TsComponent_BroadcastEvent, 4);
  m_Duk.RegisterGlobalFunction("__CPP_TsComponent_SetTickInterval", __CPP_TsComponent_SetTickInterval, 2);

  return EZ_SUCCESS;
}

ezResult ezTypeScriptBinding::RegisterComponent(ezStringView sTypeName0, ezComponentHandle hHandle, ezUInt32& out_uiStashIdx, bool bIsNativeComponent)
{
  if (hHandle.IsInvalidated())
    return EZ_FAILURE;

  ezUInt32& uiStashIdx = m_ComponentToStashIdx[hHandle];

  if (uiStashIdx != 0)
  {
    out_uiStashIdx = uiStashIdx;
    return EZ_SUCCESS;
  }

  uiStashIdx = AcquireStashObjIndex();

  ezDuktapeHelper duk(m_Duk);
  EZ_DUK_VERIFY_STACK(duk, 0);

  duk.PushGlobalObject(); // [ global ]
  EZ_DUK_VERIFY_STACK(duk, +1);

  ezStringBuilder sTypeName = sTypeName0;

  if (bIsNativeComponent)
  {
    sTypeName.TrimWordStart("ez");

    EZ_SUCCEED_OR_RETURN(duk.PushLocalObject("__AllComponents")); // [ global __AllComponents ]
    EZ_DUK_VERIFY_STACK(duk, +2);
  }
  else
  {
    const ezStringBuilder sCompModule("__", sTypeName);

    EZ_SUCCEED_OR_RETURN(duk.PushLocalObject(sCompModule)); // [ global __CompModule ]
    EZ_DUK_VERIFY_STACK(duk, +2);
  }

  if (!duk_get_prop_string(duk, -1, sTypeName)) // [ global __CompModule sTypeName ]
  {
    duk.PopStack(3);                            // [ ]
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, EZ_FAILURE, 0);
  }

  duk_new(duk, 0); // [ global __CompModule object ]
  EZ_DUK_VERIFY_STACK(duk, +3);

  // store C++ side component handle in obj as property
  {
    ezComponentHandle* pBuffer =
      reinterpret_cast<ezComponentHandle*>(duk_push_fixed_buffer(duk, sizeof(ezComponentHandle))); // [ global __CompModule object buffer ]
    *pBuffer = hHandle;
    duk_put_prop_index(duk, -2, ezTypeScriptBindingIndexProperty::ComponentHandle);                // [ global __CompModule object ]
  }

  EZ_DUK_VERIFY_STACK(duk, +3);

  StoreReferenceInStash(duk, uiStashIdx); // [ global __CompModule object ]
  EZ_DUK_VERIFY_STACK(duk, +3);

  duk.PopStack(3);                        // [ ]
  EZ_DUK_VERIFY_STACK(duk, 0);

  out_uiStashIdx = uiStashIdx;

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, EZ_SUCCESS, 0);
}

void ezTypeScriptBinding::DukPutComponentObject(ezComponent* pComponent)
{
  if (pComponent == nullptr)
  {
    m_Duk.PushNull(); // [ null ]
  }
  else
  {
    ezUInt32 uiStashIdx = 0;
    if (RegisterComponent(pComponent->GetDynamicRTTI()->GetTypeName(), pComponent->GetHandle(), uiStashIdx, true).Failed())
    {
      m_Duk.PushNull(); // [ null ]
      return;
    }

    DukPushStashObject(m_Duk, uiStashIdx);
  }
}

ezComponentHandle ezTypeScriptBinding::RetrieveComponentHandle(duk_context* pDuk, ezInt32 iObjIdx /*= 0 */)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return ezComponentHandle();

  ezDuktapeHelper duk(pDuk);

  if (duk_get_prop_index(pDuk, iObjIdx, ezTypeScriptBindingIndexProperty::ComponentHandle))
  {
    ezComponentHandle hComponent = *reinterpret_cast<ezComponentHandle*>(duk_get_buffer(pDuk, -1, nullptr));
    duk_pop(pDuk);
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, hComponent, 0);
  }

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, ezComponentHandle(), 0);
}

static int __CPP_Component_IsValid(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  if (!duk_get_prop_index(pDuk, 0, ezTypeScriptBindingIndexProperty::ComponentHandle))
  {
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(false), 1);
  }

  ezComponentHandle hComponent = *reinterpret_cast<ezComponentHandle*>(duk_get_buffer(pDuk, -1, nullptr));
  duk_pop(pDuk);

  ezComponent* pComponent = nullptr;
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(pDuk);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pWorld->TryGetComponent(hComponent, pComponent)), 1);
}

static int __CPP_Component_GetUniqueID(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnUInt(pComponent->GetUniqueID()), +1);
}

static int __CPP_Component_GetOwner(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pComponent->GetOwner()->GetHandle());

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_Component_SetActiveFlag(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  pComponent->SetActiveFlag(duk.GetBoolValue(1, true));

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_Component_IsActive(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  switch (duk.GetFunctionMagicValue())
  {
    case -1:
      EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pComponent->GetActiveFlag()), +1);

    case 0:
      EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pComponent->IsActive()), +1);

    case 1:
      EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pComponent->IsActiveAndInitialized()), +1);

    case 2:
      EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pComponent->IsActiveAndSimulating()), +1);
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(false), +1);
}

static int __CPP_Component_SendMessage(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(duk, 0 /*this*/);

  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(duk);

  if (duk.GetFunctionMagicValue() == 0) // SendMessage
  {
    ezUniquePtr<ezMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, ezTime::MakeZero());
    pComponent->SendMessage(*pMsg);

    if (duk.GetBoolValue(3)) // expect the message to have result values
    {
      // sync msg back to TS
      ezTypeScriptBinding::SyncEzObjectToTsObject(pDuk, pMsg->GetDynamicRTTI(), pMsg.Borrow(), 1);
    }
  }
  else // PostMessage
  {
    const ezTime delay = ezTime::MakeFromSeconds(duk.GetNumberValue(3));

    ezUniquePtr<ezMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, delay);
    pComponent->PostMessage(*pMsg, delay);
  }

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_TsComponent_BroadcastEvent(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezTypeScriptComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezTypeScriptComponent>(duk, 0 /*this*/);

  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(duk);

  ezUniquePtr<ezMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, ezTime::MakeZero());
  pComponent->BroadcastEventMsg(ezStaticCast<ezEventMessage&>(*pMsg));

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_TsComponent_SetTickInterval(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezTypeScriptComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezTypeScriptComponent>(duk, 0 /*this*/);

  const ezTime interval = ezTime::MakeFromSeconds(duk.GetFloatValue(1, 0.0f));
  pComponent->SetUpdateInterval(interval);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}
