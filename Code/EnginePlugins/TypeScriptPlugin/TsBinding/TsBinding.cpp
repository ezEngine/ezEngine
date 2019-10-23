#include <TypeScriptPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Duktape/duktape.h>
#include <Foundation/Profiling/Profiling.h>
#include <Resources/JavaScriptResource.h>
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

ezResult ezTypeScriptBinding::Initialize(ezTypeScriptTranspiler& transpiler, ezWorld& world)
{
  EZ_LOG_BLOCK("Initialize TypeScript Binding");
  EZ_PROFILE_SCOPE("Initialize TypeScript Binding");

  m_pTranspiler = &transpiler;

  m_Duk.EnableModuleSupport(&ezTypeScriptBinding::DukSearchModule);
  m_Duk.StorePointerInStash("Transpiler", m_pTranspiler);

  m_Duk.RegisterGlobalFunction("__CPP_Binding_RegisterMessageHandler", &ezTypeScriptBinding::__CPP_Binding_RegisterMessageHandler, 2);

  StoreWorld(&world);

  SetModuleSearchPath("TypeScript");

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

ezResult ezTypeScriptBinding::LoadComponent(const char* szComponent, TsComponentTypeInfo& out_TypeInfo)
{
  if (!m_bInitialized)
  {
    return EZ_FAILURE;
  }

  if (m_LoadedComponents.Contains(szComponent))
  {
    out_TypeInfo = m_TsComponentTypes.Find(szComponent);
    return m_LoadedComponents[szComponent] ? EZ_SUCCESS : EZ_FAILURE;
  }

  EZ_PROFILE_SCOPE("Load TypeScript Component");

  m_LoadedComponents[szComponent] = false;

  const ezStringBuilder sComponentFile("TypeScript/", szComponent, ".ts");

  ezStringBuilder transpiledCode;
  EZ_SUCCEED_OR_RETURN(m_pTranspiler->TranspileFileAndStoreJS(sComponentFile, transpiledCode));

  EZ_SUCCEED_OR_RETURN(m_Duk.ExecuteString(transpiledCode, sComponentFile));
  RegisterMessageHandlersForComponentType(szComponent);

  m_LoadedComponents[szComponent] = true;

  m_TsComponentTypes[szComponent]; // important
  out_TypeInfo = m_TsComponentTypes.Find(szComponent);

  return EZ_SUCCESS;
}

ezResult ezTypeScriptBinding::LoadComponent(const ezJavaScriptResourceHandle& hResource, TsComponentTypeInfo& out_TypeInfo)
{
  if (!m_bInitialized || !hResource.IsValid())
  {
    return EZ_FAILURE;
  }

  ezResourceLock<ezJavaScriptResource> pJsResource(hResource, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pJsResource.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    return EZ_FAILURE;
  }

  const ezString& sComponent = pJsResource->GetDescriptor().m_sComponentName;

  auto itLoaded = m_LoadedComponents.Find(sComponent);

  if (itLoaded.IsValid())
  {
    out_TypeInfo = m_TsComponentTypes.Find(sComponent);
    return itLoaded.Value() ? EZ_SUCCESS : EZ_FAILURE;
  }

  EZ_PROFILE_SCOPE("Load JavaScript Component");

  bool& bLoaded = m_LoadedComponents[sComponent];
  bLoaded = false;

  const char* szSource = reinterpret_cast<const char*>(pJsResource->GetDescriptor().m_JsSource.GetData());

  EZ_SUCCEED_OR_RETURN(m_Duk.ExecuteString(szSource, sComponent));
  RegisterMessageHandlersForComponentType(sComponent);

  bLoaded = true;

  out_TypeInfo = m_TsComponentTypes.FindOrAdd(sComponent, nullptr);

  return EZ_SUCCESS;
}

const ezTypeScriptBinding::TsComponentInfo* ezTypeScriptBinding::GetComponentTypeInfo(const char* szComponentType) const
{
  auto it = m_TsComponentTypes.Find(szComponentType);
  if (!it.IsValid())
    return nullptr;

  return &it.Value();
}

ezVec3 ezTypeScriptBinding::GetVec3(duk_context* pDuk, ezInt32 iObjIdx)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return ezVec3::ZeroVector();

  ezVec3 res;

  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "x"), "");
  res.x = duk_get_number_default(pDuk, -1, 0.0f);
  duk_pop(pDuk);
  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "y"), "");
  res.y = duk_get_number_default(pDuk, -1, 0.0f);
  duk_pop(pDuk);
  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "z"), "");
  res.z = duk_get_number_default(pDuk, -1, 0.0f);
  duk_pop(pDuk);

  return res;
}

ezVec3 ezTypeScriptBinding::GetVec3Property(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx)
{
  ezDuktapeHelper duk(pDuk, 0);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    duk.Error(ezFmt("Vec3 property '{}' does not exist.", szPropertyName));
    return ezVec3::ZeroVector();
  }

  const ezVec3 res = GetVec3(pDuk, -1);
  duk.PopStack(); // [ ]
  return res;
}

ezQuat ezTypeScriptBinding::GetQuat(duk_context* pDuk, ezInt32 iObjIdx)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return ezQuat::IdentityQuaternion();

  ezQuat res;

  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "w"), "");
  res.w = duk_get_number_default(pDuk, -1, 0.0f);
  duk_pop(pDuk);

  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "v"), "");

  EZ_VERIFY(duk_get_prop_string(pDuk, -1, "x"), "");
  res.v.x = duk_get_number_default(pDuk, -1, 0.0f);
  EZ_VERIFY(duk_get_prop_string(pDuk, -2, "y"), "");
  res.v.y = duk_get_number_default(pDuk, -1, 0.0f);
  EZ_VERIFY(duk_get_prop_string(pDuk, -3, "z"), "");
  res.v.z = duk_get_number_default(pDuk, -1, 0.0f);
  duk_pop_n(pDuk, 4);

  return res;
}

ezQuat ezTypeScriptBinding::GetQuatProperty(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx)
{
  ezDuktapeHelper duk(pDuk, 0);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    duk.Error(ezFmt("Quat property '{}' does not exist.", szPropertyName));
    return ezQuat::IdentityQuaternion();
  }

  const ezQuat res = GetQuat(pDuk, -1);
  duk.PopStack(); // [ ]
  return res;
}

ezColor ezTypeScriptBinding::GetColor(duk_context* pDuk, ezInt32 iObjIdx)
{
  ezDuktapeHelper duk(pDuk, 0);

  ezColor res;
  res.r = duk.GetFloatProperty("r", 0.0f, iObjIdx);
  res.g = duk.GetFloatProperty("g", 0.0f, iObjIdx);
  res.b = duk.GetFloatProperty("b", 0.0f, iObjIdx);
  res.a = duk.GetFloatProperty("a", 1.0f, iObjIdx);

  return res;
}

ezColor ezTypeScriptBinding::GetColorProperty(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx)
{
  ezDuktapeHelper duk(pDuk, 0);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    duk.Error(ezFmt("Color property '{}' does not exist.", szPropertyName));
    return ezColor::RebeccaPurple;
  }

  const ezColor res = GetColor(pDuk, -1);
  duk.PopStack(); // [ ]
  return res;
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
  duk_get_prop_string(duk, -1, "Quat");                     // [ global __Quat Vec3 ]
  duk_push_number(duk, value.v.x);                          // [ global __Quat Vec3 x ]
  duk_push_number(duk, value.v.y);                          // [ global __Quat Vec3 x y ]
  duk_push_number(duk, value.v.z);                          // [ global __Quat Vec3 x y z ]
  duk_push_number(duk, value.w);                            // [ global __Quat Vec3 x y z w ]
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
