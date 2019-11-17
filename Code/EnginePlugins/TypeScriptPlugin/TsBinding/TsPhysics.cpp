#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Physics_CastRay(duk_context* pDuk);
static int __CPP_Physics_SweepTestSphere(duk_context* pDuk);
static int __CPP_Physics_SweepTestBox(duk_context* pDuk);
static int __CPP_Physics_SweepTestCapsule(duk_context* pDuk);
static int __CPP_Physics_OverlapTestSphere(duk_context* pDuk);
static int __CPP_Physics_OverlapTestCapsule(duk_context* pDuk);
static int __CPP_Physics_GetGravity(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_Physics()
{
  m_Duk.RegisterGlobalFunction("__CPP_Physics_CastRay", __CPP_Physics_CastRay, 6);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_SweepTestSphere", __CPP_Physics_SweepTestSphere, 6);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_SweepTestBox", __CPP_Physics_SweepTestBox, 6);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_SweepTestCapsule", __CPP_Physics_SweepTestCapsule, 7);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_OverlapTestSphere", __CPP_Physics_OverlapTestSphere, 4);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_OverlapTestCapsule", __CPP_Physics_OverlapTestCapsule, 5);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_GetGravity", __CPP_Physics_GetGravity, 0);

  return EZ_SUCCESS;
}

static int __CPP_Physics_CastRay(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);


  return duk.ReturnCustom();
}

static int __CPP_Physics_SweepTestSphere(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);


  return duk.ReturnCustom();
}

static int __CPP_Physics_SweepTestBox(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);


  return duk.ReturnCustom();
}

static int __CPP_Physics_SweepTestCapsule(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);


  return duk.ReturnCustom();
}

static int __CPP_Physics_OverlapTestSphere(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);


  return duk.ReturnCustom();
}

static int __CPP_Physics_OverlapTestCapsule(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);


  return duk.ReturnCustom();
}

static int __CPP_Physics_GetGravity(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  ezPhysicsWorldModuleInterface* pModule = pWorld->GetOrCreateModule<ezPhysicsWorldModuleInterface>();
  if (pModule == nullptr)
  {
    duk.Error("No Physics World Module available.");
    return 0;
  }

  ezTypeScriptBinding::PushVec3(duk, pModule->GetGravity());
  return duk.ReturnCustom();
}
