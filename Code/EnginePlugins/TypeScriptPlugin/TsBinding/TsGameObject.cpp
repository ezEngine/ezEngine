#include <TypeScriptPluginPCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Duktape/duktape.h>
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
static int __CPP_GameObject_FindComponentByTypeName(duk_context* pDuk);
static int __CPP_GameObject_FindComponentByTypeNameHash(duk_context* pDuk);
static int __CPP_GameObject_SendMessage(duk_context* pDuk);
static int __CPP_GameObject_SetName(duk_context* pDuk);
static int __CPP_GameObject_GetName(duk_context* pDuk);

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
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_FindChildByName", __CPP_GameObject_FindChildByName, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_FindComponentByTypeName", __CPP_GameObject_FindComponentByTypeName, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_FindComponentByTypeNameHash", __CPP_GameObject_FindComponentByTypeNameHash, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SendMessage", __CPP_GameObject_SendMessage, 3);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalDirForwards", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalDirForwards);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalDirRight", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalDirRight);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalDirUp", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalDirUp);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetVelocity", __CPP_GameObject_SetX_Vec3, 2, GameObject_X::Velocity);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetVelocity", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::Velocity);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetName", __CPP_GameObject_SetName, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetName", __CPP_GameObject_GetName, 1);

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
  ezDuktapeHelper duk(pDuk, +1);

  if (hObject.IsInvalidated())
  {
    duk.PushNull(); // [ null ]
    return;
  }

  // TODO: make this more efficient by reusing previous ez.GameObject instances when possible

  // create ez.GameObject and store the ezGameObjectHandle as a property in it
  duk.PushGlobalObject();                                         // [ global ]
  EZ_VERIFY(duk.PushLocalObject("__GameObject").Succeeded(), ""); // [ global __GameObject ]
  duk_get_prop_string(duk, -1, "GameObject");                     // [ global __GameObject GameObject ]
  duk_new(duk, 0);                                                // [ global __GameObject result ]

  // set the ezGameObjectHandle property
  {
    ezGameObjectHandle* pHandleBuffer = reinterpret_cast<ezGameObjectHandle*>(duk_push_fixed_buffer(duk, sizeof(ezGameObjectHandle))); // [ global __GameObject result buffer ]
    *pHandleBuffer = hObject;
    duk_put_prop_string(duk, -2, "ezGameObjectHandle"); // [ global __GameObject result ]
  }

  // move the top of the stack to the only position that we want to keep (return)
  duk_replace(duk, -3); // [ result __GameObject ]
  duk.PopStack();       // [ result ]
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

static int __CPP_GameObject_SetX_Vec3(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, 0);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

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
  ezDuktapeFunction duk(pDuk, +1);

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

  return duk.ReturnCustom();
}

static int __CPP_GameObject_SetX_Float(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, 0);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

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
  ezDuktapeFunction duk(pDuk, +1);

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

  return duk.ReturnFloat(value);
}

static int __CPP_GameObject_SetX_Quat(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, 0);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

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
  ezDuktapeFunction duk(pDuk, +1);

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

  return duk.ReturnCustom();
}

static int __CPP_GameObject_SetX_Bool(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, 0);

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
  ezDuktapeFunction duk(pDuk, +1);

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

  return duk.ReturnBool(value);
}

static int __CPP_GameObject_FindChildByName(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, +1);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const char* szName = duk.GetStringValue(1);
  bool bRecursive = duk.GetBoolValue(2, true);

  ezGameObject* pChild = pGameObject->FindChildByName(ezTempHashedString(szName), bRecursive);

  ezTypeScriptBinding::DukPutGameObject(duk, pChild);

  return duk.ReturnCustom();
}

static int __CPP_GameObject_FindComponentByTypeName(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, +1);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const char* szTypeName = duk.GetStringValue(1);

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
  ezDuktapeFunction duk(pDuk, +1);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  ezUInt32 uiTypeNameHash = duk.GetUIntValue(1);

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

ezMessage* CreateMessage(ezUInt32 uiTypeHash)
{
  static ezHashTable<ezUInt32, const ezRTTI*, ezHashHelper<ezUInt32>, ezStaticAllocatorWrapper> MessageTypes;

  const ezRTTI* pRtti = nullptr;
  if (!MessageTypes.TryGetValue(uiTypeHash, pRtti))
  {
    for (pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
    {
      if (pRtti->GetTypeNameHash() == uiTypeHash)
      {
        MessageTypes[uiTypeHash] = pRtti;
        break;
      }
    }
  }

  if (pRtti == nullptr || !pRtti->GetAllocator()->CanAllocate())
    return nullptr;

  auto pMsg = pRtti->GetAllocator()->Allocate<ezMessage>();

  return pMsg;
}

static int __CPP_GameObject_SendMessage(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, 0);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  ezUInt32 uiTypeNameHash = duk.GetUIntValue(1);

  ezMessage* pMsg = CreateMessage(uiTypeNameHash);

  const ezRTTI* pRtti = pMsg->GetDynamicRTTI();


  ezHybridArray<ezAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (ezAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member)
      continue;

    ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pProp);

    const ezVariantType::Enum type = pMember->GetSpecificType()->GetVariantType();
    switch (type)
    {
      case ezVariantType::Invalid:
        break;

      case ezVariantType::Bool:
      {
        bool value = duk.GetBoolProperty(pMember->GetPropertyName(), false, 2);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg, value);
        break;
      }

      case ezVariantType::String:
      case ezVariantType::StringView:
      {
        const char* value = duk.GetStringProperty(pMember->GetPropertyName(), "", 2);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg, value);
        break;
      }

      case ezVariantType::Int8:
      case ezVariantType::Int16:
      case ezVariantType::Int32:
      case ezVariantType::Int64:
      {
        ezInt32 value = duk.GetIntProperty(pMember->GetPropertyName(), 0, 2);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg, value);
        break;
      }

      case ezVariantType::UInt8:
      case ezVariantType::UInt16:
      case ezVariantType::UInt32:
      case ezVariantType::UInt64:
      {
        ezUInt32 value = duk.GetUIntProperty(pMember->GetPropertyName(), 0, 2);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg, value);
        break;
      }

      case ezVariantType::Float:
      {
        const float value = duk.GetFloatProperty(pMember->GetPropertyName(), 0, 2);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg, value);
        break;
      }

      case ezVariantType::Double:
      {
        const double value = duk.GetNumberProperty(pMember->GetPropertyName(), 0, 2);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg, value);
        break;
      }


      case ezVariantType::Vector3:
      {
        ezVec3 value = ezTypeScriptBinding::GetVec3Property(duk, pMember->GetPropertyName(), 2);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg, value);
        break;
      }

      case ezVariantType::Quaternion:
      {
        ezQuat value = ezTypeScriptBinding::GetQuatProperty(duk, pMember->GetPropertyName(), 2);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg, value);
        break;
      }


      case ezVariantType::Color:
      case ezVariantType::ColorGamma:
      {
        ezColor value = ezTypeScriptBinding::GetColorProperty(duk, pMember->GetPropertyName(), 2);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg, value);
        break;
      }

      case ezVariantType::Vector2:
      case ezVariantType::Matrix3:
      case ezVariantType::Matrix4:
      case ezVariantType::Time:
      case ezVariantType::Uuid:
      case ezVariantType::Angle:
      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }

  if (duk.GetBoolValue(2))
  {
    pGameObject->SendMessageRecursive(*pMsg);
  }
  else
  {
    pGameObject->SendMessage(*pMsg);
  }

  return duk.ReturnVoid();
}


static int __CPP_GameObject_SetName(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, 0);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  pGameObject->SetName(duk.GetStringValue(1));

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetName(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, 0);

  ezGameObject* pGameObject = ezTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  return duk.ReturnString(pGameObject->GetName());
}
