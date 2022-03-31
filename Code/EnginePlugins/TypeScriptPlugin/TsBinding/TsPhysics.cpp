#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Physics_Raycast(duk_context* pDuk);
static int __CPP_Physics_SweepTestSphere(duk_context* pDuk);
static int __CPP_Physics_SweepTestBox(duk_context* pDuk);
static int __CPP_Physics_SweepTestCapsule(duk_context* pDuk);
static int __CPP_Physics_OverlapTestSphere(duk_context* pDuk);
static int __CPP_Physics_OverlapTestCapsule(duk_context* pDuk);
static int __CPP_Physics_GetGravity(duk_context* pDuk);
static int __CPP_Physics_QueryShapesInSphere(duk_context* pDuk);

#define GetPhysicsModule()                                    \
  pWorld->GetOrCreateModule<ezPhysicsWorldModuleInterface>(); \
                                                              \
  if (pModule == nullptr)                                     \
  {                                                           \
    duk.Error("No Physics World Module available.");          \
    return 0;                                                 \
  }

ezResult ezTypeScriptBinding::Init_Physics()
{
  m_Duk.RegisterGlobalFunction("__CPP_Physics_Raycast", __CPP_Physics_Raycast, 6);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_SweepTestSphere", __CPP_Physics_SweepTestSphere, 6);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_SweepTestBox", __CPP_Physics_SweepTestBox, 6);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_SweepTestCapsule", __CPP_Physics_SweepTestCapsule, 7);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_OverlapTestSphere", __CPP_Physics_OverlapTestSphere, 4);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_OverlapTestCapsule", __CPP_Physics_OverlapTestCapsule, 5);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_GetGravity", __CPP_Physics_GetGravity, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_QueryShapesInSphere", __CPP_Physics_QueryShapesInSphere, 5);

  return EZ_SUCCESS;
}

static void PutHitResult(ezDuktapeHelper& duk, const ezPhysicsCastResult& res)
{
  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(duk);

  duk.PushGlobalObject();                                       // [ global ]
  duk.PushLocalObject("__Physics").IgnoreResult();              // [ global __Physics ]
  duk.PushLocalObject("Physics").IgnoreResult();                // [ global __Physics Physics ]
  duk_get_prop_string(duk, -1, "HitResult");                    // [ global __Physics Physics HitResult ]
  duk_new(duk, 0);                                              // [ global __Physics Physics HitResultObj ]
  duk_remove(duk, -2);                                          // [ global __Physics HitResultObj ]
  duk_remove(duk, -2);                                          // [ global HitResultObj ]
  duk_remove(duk, -2);                                          // [ HitResultObj ]
  ;                                                             //
  duk.SetNumberProperty("distance", res.m_fDistance, -1);       // [ HitResultObj ]
  duk.SetNumberProperty("shapeId", res.m_uiObjectFilterID, -1); // [ HitResultObj ]
  ;                                                             //
  ezTypeScriptBinding::PushVec3(duk, res.m_vPosition);          // [ HitResultObj pos ]
  duk.SetCustomProperty("position", -1);                        // [ HitResultObj ]
  ;                                                             //
  ezTypeScriptBinding::PushVec3(duk, res.m_vNormal);            // [ HitResultObj normal ]
  duk.SetCustomProperty("normal", -1);                          // [ HitResultObj ]
  ;                                                             //
  pBinding->DukPutGameObject(res.m_hShapeObject);               // [ HitResultObj GO ]
  duk.SetCustomProperty("shapeObject", -1);                     // [ HitResultObj ]
  ;                                                             //
  pBinding->DukPutGameObject(res.m_hActorObject);               // [ HitResultObj GO ]
  duk.SetCustomProperty("actorObject", -1);                     // [ HitResultObj ]
}

static int __CPP_Physics_Raycast(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  ezPhysicsWorldModuleInterface* pModule = GetPhysicsModule();

  const ezVec3 vStart = ezTypeScriptBinding::GetVec3(duk, 0);
  ezVec3 vDir = ezTypeScriptBinding::GetVec3(duk, 1);
  const float fDistance = duk.GetFloatValue(2);
  const ezUInt32 uiCollisionLayer = duk.GetUIntValue(3);
  const ezPhysicsShapeType::Enum shapeTypes = static_cast<ezPhysicsShapeType::Enum>(duk.GetUIntValue(4));
  const ezUInt32 uiIgnoreShapeID = duk.GetIntValue(5);

  if (vDir.NormalizeIfNotZero(ezVec3::UnitXAxis()).Failed())
  {
    ezLog::Warning("TS: ez.Physics.Raycast() called with degenerate ray direction.");
    return duk.ReturnNull();
  }

  ezPhysicsCastResult res;
  ezPhysicsQueryParameters queryParams(uiCollisionLayer, shapeTypes, uiIgnoreShapeID);
  queryParams.m_bIgnoreInitialOverlap = true;

  if (!pModule->Raycast(res, vStart, vDir, fDistance, queryParams))
  {
    return duk.ReturnNull();
  }

  PutHitResult(duk, res);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_Physics_SweepTestSphere(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  ezPhysicsWorldModuleInterface* pModule = GetPhysicsModule();

  const float fRadius = duk.GetFloatValue(0);
  const ezVec3 vStart = ezTypeScriptBinding::GetVec3(duk, 1);
  const ezVec3 vDir = ezTypeScriptBinding::GetVec3(duk, 2);
  const float fDistance = duk.GetFloatValue(3);
  const ezUInt32 uiCollisionLayer = duk.GetUIntValue(4);
  const ezUInt32 uiIgnoreShapeID = duk.GetIntValue(5);

  ezPhysicsCastResult res;
  if (!pModule->SweepTestSphere(res, fRadius, vStart, vDir, fDistance, ezPhysicsQueryParameters(uiCollisionLayer, ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic, uiIgnoreShapeID)))
  {
    return duk.ReturnNull();
  }

  PutHitResult(duk, res);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_Physics_SweepTestBox(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  ezPhysicsWorldModuleInterface* pModule = GetPhysicsModule();

  const ezVec3 vExtents = ezTypeScriptBinding::GetVec3(duk, 0);
  const ezTransform transform = ezTypeScriptBinding::GetTransform(duk, 1);
  const ezVec3 vDir = ezTypeScriptBinding::GetVec3(duk, 2);
  const float fDistance = duk.GetFloatValue(3);
  const ezUInt32 uiCollisionLayer = duk.GetUIntValue(4);
  const ezUInt32 uiIgnoreShapeID = duk.GetIntValue(5);

  ezPhysicsCastResult res;
  if (!pModule->SweepTestBox(res, vExtents, transform, vDir, fDistance, ezPhysicsQueryParameters(uiCollisionLayer, ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic, uiIgnoreShapeID)))
  {
    return duk.ReturnNull();
  }

  PutHitResult(duk, res);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_Physics_SweepTestCapsule(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  ezPhysicsWorldModuleInterface* pModule = GetPhysicsModule();

  const float fCapsuleRadius = duk.GetFloatValue(0);
  const float fCapsuleHeight = duk.GetFloatValue(1);
  const ezTransform transform = ezTypeScriptBinding::GetTransform(duk, 2);
  const ezVec3 vDir = ezTypeScriptBinding::GetVec3(duk, 3);
  const float fDistance = duk.GetFloatValue(4);
  const ezUInt32 uiCollisionLayer = duk.GetUIntValue(5);
  const ezUInt32 uiIgnoreShapeID = duk.GetIntValue(6);

  ezPhysicsCastResult res;
  if (!pModule->SweepTestCapsule(res, fCapsuleRadius, fCapsuleHeight, transform, vDir, fDistance, ezPhysicsQueryParameters(uiCollisionLayer, ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic, uiIgnoreShapeID)))
  {
    return duk.ReturnNull();
  }

  PutHitResult(duk, res);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_Physics_OverlapTestSphere(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  ezPhysicsWorldModuleInterface* pModule = GetPhysicsModule();

  const float fRadius = duk.GetFloatValue(0);
  const ezVec3 vPos = ezTypeScriptBinding::GetVec3(duk, 1);
  const ezUInt32 uiCollisionLayer = duk.GetUIntValue(2);
  const ezUInt32 uiIgnoreShapeID = duk.GetIntValue(3);

  return duk.ReturnBool(pModule->OverlapTestSphere(fRadius, vPos, ezPhysicsQueryParameters(uiCollisionLayer, ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic, uiIgnoreShapeID)));
}

static int __CPP_Physics_OverlapTestCapsule(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  ezPhysicsWorldModuleInterface* pModule = GetPhysicsModule();

  const float fCapsuleRadius = duk.GetFloatValue(0);
  const float fCapsuleHeight = duk.GetFloatValue(1);
  const ezTransform transform = ezTypeScriptBinding::GetTransform(duk, 2);
  const ezUInt32 uiCollisionLayer = duk.GetUIntValue(3);
  const ezUInt32 uiIgnoreShapeID = duk.GetIntValue(4);

  return duk.ReturnBool(pModule->OverlapTestCapsule(fCapsuleRadius, fCapsuleHeight, transform, ezPhysicsQueryParameters(uiCollisionLayer, ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic, uiIgnoreShapeID)));
}

static int __CPP_Physics_GetGravity(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  ezPhysicsWorldModuleInterface* pModule = GetPhysicsModule();

  ezTypeScriptBinding::PushVec3(duk, pModule->GetGravity());
  return duk.ReturnCustom();
}

int __CPP_Physics_QueryShapesInSphere(duk_context* pDuk)
{
  duk_require_function(pDuk, -1);

  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(pDuk);

  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  ezPhysicsWorldModuleInterface* pModule = GetPhysicsModule();

  const float fRadius = duk.GetFloatValue(0);
  const ezVec3 vPos = ezTypeScriptBinding::GetVec3(duk, 1);
  const ezUInt8 uiCollisionLayer = static_cast<ezUInt8>(duk.GetUIntValue(2));
  const ezUInt32 uiIgnoreShapeID = duk.GetIntValue(3);

  ezPhysicsOverlapResultArray result;
  pModule->QueryShapesInSphere(result, fRadius, vPos, ezPhysicsQueryParameters(uiCollisionLayer, ezPhysicsShapeType::Dynamic, uiIgnoreShapeID));

  // forward the results via a callback
  for (const auto& res : result.m_Results)
  {
    duk_dup(pDuk, -1); // [ func ]

    pBinding->DukPutGameObject(res.m_hActorObject); // [ func go1 ]
    pBinding->DukPutGameObject(res.m_hShapeObject); // [ func go1 go2 ]
    duk.PushUInt(res.m_uiObjectFilterID);           // [ func go1 go2 sid ]

    duk_call(pDuk, 3); // [ result ]

    if (duk_get_boolean_default(pDuk, -1, false) == false)
    {
      duk_pop(pDuk); // [ ]
      break;
    }

    duk_pop(pDuk); // [ ]
  }

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}
