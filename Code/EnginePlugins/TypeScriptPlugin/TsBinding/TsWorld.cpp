#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_World_DeleteObjectDelayed(duk_context* pDuk);
static int __CPP_World_CreateObject(duk_context* pDuk);
static int __CPP_World_CreateComponent(duk_context* pDuk);
static int __CPP_World_DeleteComponent(duk_context* pDuk);
static int __CPP_World_TryGetObjectWithGlobalKey(duk_context* pDuk);
static int __CPP_World_FindObjectsInSphere(duk_context* pDuk);
static int __CPP_World_FindObjectsInBox(duk_context* pDuk);

ezHashTable<duk_context*, ezWorld*> ezTypeScriptBinding::s_DukToWorld;

ezResult ezTypeScriptBinding::Init_World()
{
  m_Duk.RegisterGlobalFunction("__CPP_World_DeleteObjectDelayed", __CPP_World_DeleteObjectDelayed, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_CreateObject", __CPP_World_CreateObject, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_CreateComponent", __CPP_World_CreateComponent, 2);
  m_Duk.RegisterGlobalFunction("__CPP_World_DeleteComponent", __CPP_World_DeleteComponent, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_TryGetObjectWithGlobalKey", __CPP_World_TryGetObjectWithGlobalKey, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_FindObjectsInSphere", __CPP_World_FindObjectsInSphere, 3);
  m_Duk.RegisterGlobalFunction("__CPP_World_FindObjectsInBox", __CPP_World_FindObjectsInBox, 3);

  return EZ_SUCCESS;
}

void ezTypeScriptBinding::StoreWorld(ezWorld* pWorld)
{
  m_pWorld = pWorld;
  s_DukToWorld[m_Duk.GetContext()] = pWorld;
}

ezWorld* ezTypeScriptBinding::RetrieveWorld(duk_context* pDuk)
{
  ezWorld* pWorld = nullptr;
  s_DukToWorld.TryGetValue(pDuk, pWorld);
  return pWorld;
}

static int __CPP_World_DeleteObjectDelayed(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);
  ezGameObjectHandle hObject = ezTypeScriptBinding::RetrieveGameObjectHandle(duk, 0 /*this*/);

  pWorld->DeleteObjectDelayed(hObject);

  return duk.ReturnVoid();
}

static int __CPP_World_CreateObject(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  ezGameObjectDesc desc;

  desc.m_bActive = duk.GetBoolProperty("Active", desc.m_bActive, 0);
  desc.m_bDynamic = duk.GetBoolProperty("Dynamic", desc.m_bDynamic, 0);
  desc.m_LocalPosition = ezTypeScriptBinding::GetVec3Property(duk, "LocalPosition", 0, ezVec3(0.0f));
  desc.m_LocalScaling = ezTypeScriptBinding::GetVec3Property(duk, "LocalScaling", 0, ezVec3(1.0f));
  desc.m_LocalRotation = ezTypeScriptBinding::GetQuatProperty(duk, "LocalRotation", 0);
  desc.m_LocalUniformScaling = duk.GetFloatProperty("LocalUniformScaling", 1.0f, 0);
  desc.m_uiTeamID = duk.GetUIntProperty("TeamID", 0, 0);

  if (duk.PushLocalObject("Parent", 0).Succeeded())
  {
    desc.m_hParent = ezTypeScriptBinding::RetrieveGameObjectHandle(duk, -1);
    duk.PopStack();
  }

  const char* szName = duk.GetStringProperty("Name", nullptr, 0);

  if (!ezStringUtils::IsNullOrEmpty(szName))
  {
    desc.m_sName.Assign(szName);
  }

  ezGameObjectHandle hObject = pWorld->CreateObject(desc);

  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(hObject);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_World_CreateComponent(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);
  ezGameObject* pOwner = ezTypeScriptBinding::ExpectGameObject(duk, 0);

  const ezUInt32 uiTypeNameHash = duk.GetUIntValue(1);

  const ezRTTI* pRtti = ezRTTI::FindTypeByNameHash(uiTypeNameHash);
  if (pRtti == nullptr)
  {
    duk.Error(ezFmt("Invalid component type name hash: {}", uiTypeNameHash));
    return duk.ReturnNull();
  }

  auto* pMan = pWorld->GetOrCreateManagerForComponentType(pRtti);

  ezComponent* pComponent = nullptr;
  pMan->CreateComponent(pOwner, pComponent);

  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(duk);
  pBinding->DukPutComponentObject(pComponent);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_World_DeleteComponent(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);
  ezComponentHandle hComponent = ezTypeScriptBinding::RetrieveComponentHandle(duk, 0 /*this*/);

  ezComponent* pComponent = nullptr;
  if (pWorld->TryGetComponent(hComponent, pComponent))
  {
    pComponent->GetOwningManager()->DeleteComponent(pComponent);
  }

  return duk.ReturnVoid();
}

static int __CPP_World_TryGetObjectWithGlobalKey(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  ezGameObject* pObject = nullptr;
  pWorld->TryGetObjectWithGlobalKey(ezTempHashedString(duk.GetStringValue(0)), pObject);

  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pObject);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

struct FindObjectsCallback
{
  duk_context* m_pDuk = nullptr;
  ezTypeScriptBinding* m_pBinding = nullptr;

  ezVisitorExecution::Enum Callback(ezGameObject* pObject)
  {
    ezDuktapeHelper duk(m_pDuk);

    if (!duk_get_global_string(m_pDuk, "callback")) // [ func ]
      return ezVisitorExecution::Stop;

    EZ_DUK_VERIFY_STACK(duk, +1);

    m_pBinding->DukPutGameObject(pObject); // [ func go ]

    EZ_DUK_VERIFY_STACK(duk, +2);

    duk_call(m_pDuk, 1); // [ res ]

    EZ_DUK_VERIFY_STACK(duk, +1);

    if (duk_get_boolean_default(m_pDuk, -1, false) == false)
    {
      duk_pop(m_pDuk); // [ ]
      return ezVisitorExecution::Stop;
    }

    duk_pop(m_pDuk); // [ ]

    EZ_DUK_VERIFY_STACK(duk, 0);
    return ezVisitorExecution::Continue;
  }
};

static int __CPP_World_FindObjectsInSphere(duk_context* pDuk)
{
  duk_require_function(pDuk, 2);

  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  const ezVec3 vSphereCenter = ezTypeScriptBinding::GetVec3(pDuk, 0);
  const float fRadius = duk.GetFloatValue(1);

  duk_dup(pDuk, -1);
  duk_put_global_string(pDuk, "callback");

  FindObjectsCallback cb;
  cb.m_pDuk = pDuk;
  cb.m_pBinding = ezTypeScriptBinding::RetrieveBinding(pDuk);

  pWorld->GetSpatialSystem()->FindObjectsInSphere(ezBoundingSphere(vSphereCenter, fRadius), ezDefaultSpatialDataCategories::RenderDynamic.GetBitmask(), ezMakeDelegate(&FindObjectsCallback::Callback, &cb));


  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_World_FindObjectsInBox(duk_context* pDuk)
{
  duk_require_function(pDuk, 2);

  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  const ezVec3 vBoxMin = ezTypeScriptBinding::GetVec3(pDuk, 0);
  const ezVec3 vBoxMax = ezTypeScriptBinding::GetVec3(pDuk, 1);

  duk_dup(pDuk, -1);
  duk_put_global_string(pDuk, "callback");

  FindObjectsCallback cb;
  cb.m_pDuk = pDuk;
  cb.m_pBinding = ezTypeScriptBinding::RetrieveBinding(pDuk);

  pWorld->GetSpatialSystem()->FindObjectsInBox(ezBoundingBox(vBoxMin, vBoxMax), ezDefaultSpatialDataCategories::RenderDynamic.GetBitmask(), ezMakeDelegate(&FindObjectsCallback::Callback, &cb));


  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

