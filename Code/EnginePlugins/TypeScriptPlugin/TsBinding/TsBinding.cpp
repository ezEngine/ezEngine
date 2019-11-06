#include <TypeScriptPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Duktape/duktape.h>
#include <Foundation/Profiling/Profiling.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static ezHashTable<duk_context*, ezTypeScriptBinding*> s_DukToBinding;

ezTypeScriptBinding::ezTypeScriptBinding()
  : m_Duk("Typescript Binding")
{
  s_DukToBinding[m_Duk] = this;
}

ezTypeScriptBinding::~ezTypeScriptBinding()
{
  s_DukToBinding.Remove(m_Duk);
}

ezTypeScriptBinding* ezTypeScriptBinding::RetrieveBinding(duk_context* pDuk)
{
  ezTypeScriptBinding* pBinding = nullptr;
  s_DukToBinding.TryGetValue(pDuk, pBinding);
  return pBinding;
}

ezResult ezTypeScriptBinding::Initialize(ezWorld& world)
{
  EZ_LOG_BLOCK("Initialize TypeScript Binding");
  EZ_PROFILE_SCOPE("Initialize TypeScript Binding");

  m_hScriptCompendium = ezResourceManager::LoadResource<ezScriptCompendiumResource>(":project/AssetCache/Common/Scripts.ezScriptCompendium");

  m_Duk.EnableModuleSupport(&ezTypeScriptBinding::DukSearchModule);
  m_Duk.StorePointerInStash("ezTypeScriptBinding", this);

  m_Duk.RegisterGlobalFunction("__CPP_Binding_RegisterMessageHandler", &ezTypeScriptBinding::__CPP_Binding_RegisterMessageHandler, 2);

  StoreWorld(&world);

  SetupRttiFunctionBindings();
  SetupRttiPropertyBindings();

  EZ_SUCCEED_OR_RETURN(Init_RequireModules());
  EZ_SUCCEED_OR_RETURN(Init_Log());
  EZ_SUCCEED_OR_RETURN(Init_GameObject());
  EZ_SUCCEED_OR_RETURN(Init_FunctionBinding());
  EZ_SUCCEED_OR_RETURN(Init_PropertyBinding());
  EZ_SUCCEED_OR_RETURN(Init_Component());
  EZ_SUCCEED_OR_RETURN(Init_World());

  m_bInitialized = true;
  return EZ_SUCCESS;
}

ezResult ezTypeScriptBinding::LoadComponent(const ezUuid& typeGuid, TsComponentTypeInfo& out_TypeInfo)
{
  if (!m_bInitialized || !typeGuid.IsValid())
  {
    return EZ_FAILURE;
  }

  // check if this component type has been loaded before
  {
    auto itLoaded = m_LoadedComponents.Find(typeGuid);

    if (itLoaded.IsValid())
    {
      out_TypeInfo = m_TsComponentTypes.Find(typeGuid);
      return itLoaded.Value() ? EZ_SUCCESS : EZ_FAILURE;
    }
  }

  EZ_PROFILE_SCOPE("Load Script Component");

  bool& bLoaded = m_LoadedComponents[typeGuid];
  bLoaded = false;

  ezResourceLock<ezScriptCompendiumResource> pCompendium(m_hScriptCompendium, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pCompendium.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    return EZ_FAILURE;
  }

  auto itType = pCompendium->GetDescriptor().m_AssetGuidToInfo.Find(typeGuid);
  if (!itType.IsValid())
  {
    return EZ_FAILURE;
  }

  const ezString& sComponentPath = itType.Value().m_sComponentFilePath;
  const ezString& sComponentName = itType.Value().m_sComponentTypeName;

  const ezStringBuilder sCompModule("__", sComponentName);

  m_Duk.PushGlobalObject();

  ezStringBuilder req;
  req.Format("var {} = require(\"./{}\");", sCompModule, itType.Value().m_sComponentFilePath);
  if (m_Duk.ExecuteString(req).Failed())
  {
    ezLog::Error("Could not load component");
    return EZ_FAILURE;
  }

  m_Duk.PopStack();

  m_TsComponentTypes[typeGuid].m_sComponentTypeName = sComponentName;
  RegisterMessageHandlersForComponentType(sComponentName, typeGuid);

  bLoaded = true;

  out_TypeInfo = m_TsComponentTypes.FindOrAdd(typeGuid, nullptr);

  return EZ_SUCCESS;
}

ezVec2 ezTypeScriptBinding::GetVec2(duk_context* pDuk, ezInt32 iObjIdx, const ezVec2& fallback /*= ezVec2::ZeroVector()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return fallback;

  ezVec2 res;

  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "x"), "");
  res.x = duk_get_number_default(pDuk, -1, fallback.x);
  duk_pop(pDuk);
  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "y"), "");
  res.y = duk_get_number_default(pDuk, -1, fallback.y);
  duk_pop(pDuk);

  return res;
}

ezVec2 ezTypeScriptBinding::GetVec2Property(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezVec2& fallback /*= ezVec2::ZeroVector()*/)
{
  ezDuktapeHelper duk(pDuk, 0);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    return fallback;
  }

  const ezVec2 res = GetVec2(pDuk, -1, fallback);
  duk.PopStack(); // [ ]
  return res;
}

ezVec3 ezTypeScriptBinding::GetVec3(duk_context* pDuk, ezInt32 iObjIdx, const ezVec3& fallback /*= ezVec3::ZeroVector()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return fallback;

  ezVec3 res;

  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "x"), "");
  res.x = duk_get_number_default(pDuk, -1, fallback.x);
  duk_pop(pDuk);
  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "y"), "");
  res.y = duk_get_number_default(pDuk, -1, fallback.y);
  duk_pop(pDuk);
  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "z"), "");
  res.z = duk_get_number_default(pDuk, -1, fallback.z);
  duk_pop(pDuk);

  return res;
}

ezVec3 ezTypeScriptBinding::GetVec3Property(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezVec3& fallback /*= ezVec3::ZeroVector()*/)
{
  ezDuktapeHelper duk(pDuk, 0);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    return fallback;
  }

  const ezVec3 res = GetVec3(pDuk, -1, fallback);
  duk.PopStack(); // [ ]
  return res;
}

ezQuat ezTypeScriptBinding::GetQuat(duk_context* pDuk, ezInt32 iObjIdx, ezQuat fallback /*= ezQuat::IdentityQuaternion()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return fallback;

  ezQuat res;

  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "x"), "");
  res.v.x = duk_get_number_default(pDuk, -1, fallback.v.x);
  duk_pop(pDuk);
  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "y"), "");
  res.v.y = duk_get_number_default(pDuk, -1, fallback.v.y);
  duk_pop(pDuk);
  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "z"), "");
  res.v.z = duk_get_number_default(pDuk, -1, fallback.v.z);
  duk_pop(pDuk);
  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "w"), "");
  res.w = duk_get_number_default(pDuk, -1, fallback.w);
  duk_pop(pDuk);

  return res;
}

ezQuat ezTypeScriptBinding::GetQuatProperty(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, ezQuat fallback /*= ezQuat::IdentityQuaternion()*/)
{
  ezDuktapeHelper duk(pDuk, 0);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    return fallback;
  }

  const ezQuat res = GetQuat(pDuk, -1, fallback);
  duk.PopStack(); // [ ]
  return res;
}

ezColor ezTypeScriptBinding::GetColor(duk_context* pDuk, ezInt32 iObjIdx, const ezColor& fallback /*= ezColor::White*/)
{
  ezDuktapeHelper duk(pDuk, 0);

  ezColor res;
  res.r = duk.GetFloatProperty("r", fallback.r, iObjIdx);
  res.g = duk.GetFloatProperty("g", fallback.g, iObjIdx);
  res.b = duk.GetFloatProperty("b", fallback.b, iObjIdx);
  res.a = duk.GetFloatProperty("a", fallback.a, iObjIdx);

  return res;
}

ezColor ezTypeScriptBinding::GetColorProperty(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezColor& fallback /*= ezColor::White*/)
{
  ezDuktapeHelper duk(pDuk, 0);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    return fallback;
  }

  const ezColor res = GetColor(pDuk, -1, fallback);
  duk.PopStack(); // [ ]
  return res;
}

void ezTypeScriptBinding::PushVec2(duk_context* pDuk, const ezVec2& value)
{
  ezDuktapeHelper duk(pDuk, +1);

  duk.PushGlobalObject();                                   // [ global ]
  EZ_VERIFY(duk.PushLocalObject("__Vec2").Succeeded(), ""); // [ global __Vec2 ]
  duk_get_prop_string(duk, -1, "Vec2");                     // [ global __Vec2 Vec2 ]
  duk_push_number(duk, value.x);                            // [ global __Vec2 Vec2 x ]
  duk_push_number(duk, value.y);                            // [ global __Vec2 Vec2 x y ]
  duk_new(duk, 2);                                          // [ global __Vec2 result ]
  duk_remove(duk, -2);                                      // [ global result ]
  duk_remove(duk, -2);                                      // [ result ]
}

void ezTypeScriptBinding::PushVec3(duk_context* pDuk, const ezVec3& value)
{
  ezDuktapeHelper duk(pDuk, +1);

  duk.PushGlobalObject();                                   // [ global ]
  EZ_VERIFY(duk.PushLocalObject("__Vec3").Succeeded(), ""); // [ global __Vec3 ]
  duk_get_prop_string(duk, -1, "Vec3");                     // [ global __Vec3 Vec3 ]
  duk_push_number(duk, value.x);                            // [ global __Vec3 Vec3 x ]
  duk_push_number(duk, value.y);                            // [ global __Vec3 Vec3 x y ]
  duk_push_number(duk, value.z);                            // [ global __Vec3 Vec3 x y z ]
  duk_new(duk, 3);                                          // [ global __Vec3 result ]
  duk_remove(duk, -2);                                      // [ global result ]
  duk_remove(duk, -2);                                      // [ result ]
}

void ezTypeScriptBinding::PushQuat(duk_context* pDuk, const ezQuat& value)
{
  ezDuktapeHelper duk(pDuk, +1);

  duk.PushGlobalObject();                                   // [ global ]
  EZ_VERIFY(duk.PushLocalObject("__Quat").Succeeded(), ""); // [ global __Quat ]
  duk_get_prop_string(duk, -1, "Quat");                     // [ global __Quat Quat ]
  duk_push_number(duk, value.v.x);                          // [ global __Quat Quat x ]
  duk_push_number(duk, value.v.y);                          // [ global __Quat Quat x y ]
  duk_push_number(duk, value.v.z);                          // [ global __Quat Quat x y z ]
  duk_push_number(duk, value.w);                            // [ global __Quat Quat x y z w ]
  duk_new(duk, 4);                                          // [ global __Quat result ]
  duk_remove(duk, -2);                                      // [ global result ]
  duk_remove(duk, -2);                                      // [ result ]
}

void ezTypeScriptBinding::PushColor(duk_context* pDuk, const ezColor& value)
{
  ezDuktapeHelper duk(pDuk, +1);

  duk.PushGlobalObject();                                    // [ global ]
  EZ_VERIFY(duk.PushLocalObject("__Color").Succeeded(), ""); // [ global __Color ]
  duk_get_prop_string(duk, -1, "Color");                     // [ global __Color Color ]
  duk_push_number(duk, value.r);                             // [ global __Color Color r ]
  duk_push_number(duk, value.g);                             // [ global __Color Color r g ]
  duk_push_number(duk, value.b);                             // [ global __Color Color r g b ]
  duk_push_number(duk, value.a);                             // [ global __Color Color r g b a ]
  duk_new(duk, 4);                                           // [ global __Color result ]
  duk_remove(duk, -2);                                       // [ global result ]
  duk_remove(duk, -2);                                       // [ result ]
}

void ezTypeScriptBinding::StoreReferenceInStash(ezUInt32 uiStashIdx)
{
  ezDuktapeHelper duk(m_Duk, 0); // [ object ]
  duk.PushGlobalStash();         // [ object stash ]
  duk.PushUInt(uiStashIdx);      // [ object stash uint ]
  duk_dup(duk, -3);              // [ object stash uint object ]
  duk_put_prop(duk, -3);         // [ object stash ]
  duk.PopStack();                // [ object ]
}

bool ezTypeScriptBinding::DukPushStashObject(ezUInt32 uiStashIdx)
{
  ezDuktapeHelper duk(m_Duk, +1);

  duk.PushGlobalStash();          // [ stash ]
  duk_push_uint(duk, uiStashIdx); // [ stash idx ]

  if (!duk_get_prop(duk, -2)) // [ stash obj/undef ]
  {
    duk_pop_2(duk);     // [ ]
    duk_push_null(duk); // [ null ]
    return false;
  }
  else // [ stash obj ]
  {
    duk_replace(duk, -2); // [ obj ]
    return true;
  }
}
