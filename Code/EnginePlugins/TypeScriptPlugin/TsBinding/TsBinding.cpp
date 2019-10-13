#include <TypeScriptPluginPCH.h>

#include <Core/World/World.h>
#include <Duktape/duktape.h>
#include <Foundation/Profiling/Profiling.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

ezTypeScriptBinding::ezTypeScriptBinding()
  : m_Duk("Typescript Binding")
{
}

ezTypeScriptBinding::~ezTypeScriptBinding() = default;

ezResult ezTypeScriptBinding::Initialize(ezTypeScriptTranspiler& transpiler, ezWorld& world)
{
  EZ_LOG_BLOCK("Initialize TypeScript Binding");
  EZ_PROFILE_SCOPE("Initialize TypeScript Binding");

  m_pTranspiler = &transpiler;

  m_Duk.EnableModuleSupport(&ezTypeScriptBinding::DukSearchModule);
  m_Duk.StorePointerInStash("Transpiler", m_pTranspiler);

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

  m_bInitialized = true;
  return EZ_SUCCESS;
}

ezResult ezTypeScriptBinding::LoadComponent(const char* szComponent)
{
  if (!m_bInitialized)
  {
    return EZ_FAILURE;
  }

  if (m_LoadedComponents.Contains(szComponent))
  {
    return m_LoadedComponents[szComponent] ? EZ_SUCCESS : EZ_FAILURE;
  }

  EZ_PROFILE_SCOPE("Load TypeScript Component");

  m_LoadedComponents[szComponent] = false;

  ezStringBuilder transpiledCode;
  EZ_SUCCEED_OR_RETURN(m_pTranspiler->TranspileFileAndStoreJS(szComponent, transpiledCode));

  EZ_SUCCEED_OR_RETURN(m_Duk.ExecuteString(transpiledCode, szComponent));

  m_LoadedComponents[szComponent] = true;
  return EZ_SUCCESS;
}

ezVec3 ezTypeScriptBinding::GetVec3(duk_context* pDuk, ezInt32 iObjIdx)
{
  ezVec3 res;

  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "x"), "");
  res.x = duk_get_number_default(pDuk, -1, 0.0f);
  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "y"), "");
  res.y = duk_get_number_default(pDuk, -1, 0.0f);
  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "z"), "");
  res.z = duk_get_number_default(pDuk, -1, 0.0f);
  duk_pop_3(pDuk);

  return res;
}

ezQuat ezTypeScriptBinding::GetQuat(duk_context* pDuk, ezInt32 iObjIdx)
{
  ezQuat res;

  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "w"), "");
  res.w = duk_get_number_default(pDuk, -1, 0.0f);

  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "v"), "");

  EZ_VERIFY(duk_get_prop_string(pDuk, -1, "x"), "");
  res.v.x = duk_get_number_default(pDuk, -1, 0.0f);

  EZ_VERIFY(duk_get_prop_string(pDuk, -2, "y"), "");
  res.v.y = duk_get_number_default(pDuk, -1, 0.0f);

  EZ_VERIFY(duk_get_prop_string(pDuk, -3, "z"), "");
  res.v.z = duk_get_number_default(pDuk, -1, 0.0f);

  duk_pop_n(pDuk, 5);

  return res;
}

void ezTypeScriptBinding::PushVec3(duk_context* pDuk, const ezVec3& value)
{
  ezDuktapeWrapper duk(pDuk);
  ezDuktapeStackValidator validator(duk, 1);

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
  ezDuktapeWrapper duk(pDuk);
  ezDuktapeStackValidator validator(duk, 1);

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
