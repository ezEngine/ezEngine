#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_World_DeleteObjectDelayed(duk_context* pDuk);
static int __CPP_World_CreateObject(duk_context* pDuk);
static int __CPP_World_CreateComponent(duk_context* pDuk);
static int __CPP_World_DeleteComponent(duk_context* pDuk);
static int __CPP_World_TryGetObjectWithGlobalKey(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_World()
{
  m_Duk.RegisterGlobalFunction("__CPP_World_DeleteObjectDelayed", __CPP_World_DeleteObjectDelayed, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_CreateObject", __CPP_World_CreateObject, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_CreateComponent", __CPP_World_CreateComponent, 2);
  m_Duk.RegisterGlobalFunction("__CPP_World_DeleteComponent", __CPP_World_DeleteComponent, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_TryGetObjectWithGlobalKey", __CPP_World_TryGetObjectWithGlobalKey, 1);

  return EZ_SUCCESS;
}

ezHashTable<duk_context*, ezWorld*> ezTypeScriptBinding::s_DukToWorld;

void ezTypeScriptBinding::StoreWorld(ezWorld* pWorld)
{
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
  ezDuktapeFunction duk(pDuk, 0);

  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);
  ezGameObjectHandle hObject = ezTypeScriptBinding::RetrieveGameObjectHandle(duk, 0 /*this*/);

  pWorld->DeleteObjectDelayed(hObject);

  return duk.ReturnVoid();
}

static int __CPP_World_CreateObject(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, +1);

  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  ezGameObjectDesc desc;

  desc.m_bActive = duk.GetBoolProperty("Active", desc.m_bActive, 0);
  desc.m_bDynamic = duk.GetBoolProperty("Dynamic", desc.m_bDynamic, 0);
  desc.m_LocalPosition = ezTypeScriptBinding::GetVec3Property(duk, "LocalPosition", 0);
  desc.m_LocalScaling = ezTypeScriptBinding::GetVec3Property(duk, "LocalScaling", 0);
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

  return duk.ReturnCustom();
}

static int __CPP_World_CreateComponent(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, +1);

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

  return duk.ReturnCustom();
}

static int __CPP_World_DeleteComponent(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk, 0);

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
  ezDuktapeFunction duk(pDuk, +1);

  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  ezGameObject* pObject = nullptr;
  pWorld->TryGetObjectWithGlobalKey(ezTempHashedString(duk.GetStringValue(0)), pObject);

  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pObject);

  return duk.ReturnCustom();
}
