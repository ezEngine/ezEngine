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

static int __CPP_World_Clock_Get(duk_context* pDuk);
static int __CPP_World_Clock_Set(duk_context* pDuk);

static int __CPP_World_RNG_Get0(duk_context* pDuk);
static int __CPP_World_RNG_Get1(duk_context* pDuk);
static int __CPP_World_RNG_Get2(duk_context* pDuk);

static int __CPP_World_Debug_DrawCross(duk_context* pDuk);
static int __CPP_World_Debug_DrawLines(duk_context* pDuk);
static int __CPP_World_Debug_DrawBox(duk_context* pDuk);
static int __CPP_World_Debug_DrawSphere(duk_context* pDuk);
static int __CPP_World_Debug_Draw3DText(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_World()
{
  m_Duk.RegisterGlobalFunction("__CPP_World_DeleteObjectDelayed", __CPP_World_DeleteObjectDelayed, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_CreateObject", __CPP_World_CreateObject, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_CreateComponent", __CPP_World_CreateComponent, 2);
  m_Duk.RegisterGlobalFunction("__CPP_World_DeleteComponent", __CPP_World_DeleteComponent, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_TryGetObjectWithGlobalKey", __CPP_World_TryGetObjectWithGlobalKey, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_FindObjectsInSphere", __CPP_World_FindObjectsInSphere, 3);
  m_Duk.RegisterGlobalFunction("__CPP_World_FindObjectsInBox", __CPP_World_FindObjectsInBox, 3);

  m_Duk.RegisterGlobalFunction("__CPP_World_Clock_SetSpeed", __CPP_World_Clock_Set, 1, 0);
  m_Duk.RegisterGlobalFunction("__CPP_World_Clock_GetSpeed", __CPP_World_Clock_Get, 0, 0);
  m_Duk.RegisterGlobalFunction("__CPP_World_Clock_GetTimeDiff", __CPP_World_Clock_Get, 0, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_Clock_GetAccumulatedTime", __CPP_World_Clock_Get, 0, 2);

  m_Duk.RegisterGlobalFunction("__CPP_World_RNG_Bool", __CPP_World_RNG_Get0, 0, 0);
  m_Duk.RegisterGlobalFunction("__CPP_World_RNG_DoubleZeroToOneExclusive", __CPP_World_RNG_Get0, 0, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_RNG_DoubleZeroToOneInclusive", __CPP_World_RNG_Get0, 0, 2);

  m_Duk.RegisterGlobalFunction("__CPP_World_RNG_UIntInRange", __CPP_World_RNG_Get1, 1, 0);
  m_Duk.RegisterGlobalFunction("__CPP_World_RNG_DoubleVarianceAroundZero", __CPP_World_RNG_Get1, 1, 1);

  m_Duk.RegisterGlobalFunction("__CPP_World_RNG_IntInRange", __CPP_World_RNG_Get2, 2, 0);
  m_Duk.RegisterGlobalFunction("__CPP_World_RNG_IntMinMax", __CPP_World_RNG_Get2, 2, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_RNG_DoubleInRange", __CPP_World_RNG_Get2, 2, 2);
  m_Duk.RegisterGlobalFunction("__CPP_World_RNG_DoubleMinMax", __CPP_World_RNG_Get2, 2, 3);
  m_Duk.RegisterGlobalFunction("__CPP_World_RNG_DoubleVariance", __CPP_World_RNG_Get2, 2, 4);

  m_Duk.RegisterGlobalFunction("__CPP_World_Debug_DrawCross", __CPP_World_Debug_DrawCross, 3);
  m_Duk.RegisterGlobalFunction("__CPP_World_Debug_DrawLines", __CPP_World_Debug_DrawLines, 2);
  m_Duk.RegisterGlobalFunction("__CPP_World_Debug_DrawLineBox", __CPP_World_Debug_DrawBox, 4, 0);
  m_Duk.RegisterGlobalFunction("__CPP_World_Debug_DrawSolidBox", __CPP_World_Debug_DrawBox, 4, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_Debug_DrawLineSphere", __CPP_World_Debug_DrawSphere, 4);
  m_Duk.RegisterGlobalFunction("__CPP_World_Debug_Draw3DText", __CPP_World_Debug_Draw3DText, 4);

  return EZ_SUCCESS;
}

ezHashTable<duk_context*, ezWorld*> ezTypeScriptBinding::s_DukToWorld;

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

static int __CPP_World_Clock_Get(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      duk.ReturnFloat(pWorld->GetClock().GetSpeed());
      break;
    case 1:
      duk.ReturnFloat(pWorld->GetClock().GetTimeDiff().GetSeconds());
      break;
    case 2:
      duk.ReturnFloat(pWorld->GetClock().GetAccumulatedTime().GetSeconds());
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, 1, +1);
}

static int __CPP_World_Clock_Set(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      pWorld->GetClock().SetSpeed(duk.GetNumberValue(0, 1.0));
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_World_RNG_Get0(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  switch (duk.GetFunctionMagicValue())
  {
    case 0: // Bool()
      duk.ReturnBool(pWorld->GetRandomNumberGenerator().Bool());
      break;

    case 1: // DoubleZeroToOneExclusive()
      duk.ReturnNumber(pWorld->GetRandomNumberGenerator().DoubleZeroToOneExclusive());
      break;

    case 2: // DoubleZeroToOneInclusive()
      duk.ReturnNumber(pWorld->GetRandomNumberGenerator().DoubleZeroToOneInclusive());
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, 1, +1);
}

static int __CPP_World_RNG_Get1(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  switch (duk.GetFunctionMagicValue())
  {
    case 0: // UIntInRange()
      duk.ReturnUInt(pWorld->GetRandomNumberGenerator().UIntInRange(duk.GetUIntValue(0, 1)));
      break;

    case 1: // DoubleVarianceAroundZero()
      duk.ReturnNumber(pWorld->GetRandomNumberGenerator().DoubleVarianceAroundZero(duk.GetIntValue(0, 0)));
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, 1, +1);
}

static int __CPP_World_RNG_Get2(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  switch (duk.GetFunctionMagicValue())
  {
    case 0: // IntInRange()
      duk.ReturnInt(pWorld->GetRandomNumberGenerator().IntInRange(duk.GetIntValue(0, 0), duk.GetUIntValue(1, 1)));
      break;

    case 1: // IntMinMax()
      duk.ReturnNumber(pWorld->GetRandomNumberGenerator().IntMinMax(duk.GetIntValue(0, 0), duk.GetIntValue(1, 0)));
      break;

    case 2: // DoubleInRange()
      duk.ReturnNumber(pWorld->GetRandomNumberGenerator().DoubleInRange(duk.GetNumberValue(0, 0), duk.GetNumberValue(1, 0)));
      break;

    case 3: // DoubleMinMax()
      duk.ReturnNumber(pWorld->GetRandomNumberGenerator().DoubleMinMax(duk.GetNumberValue(0, 0), duk.GetNumberValue(1, 0)));
      break;

    case 4: // DoubleVariance()
      duk.ReturnNumber(pWorld->GetRandomNumberGenerator().DoubleVariance(duk.GetNumberValue(0, 0), duk.GetNumberValue(1, 1)));
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, 1, +1);
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

static int __CPP_World_Debug_DrawCross(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  const ezVec3 pos = ezTypeScriptBinding::GetVec3(pDuk, 0);
  const float size = duk.GetFloatValue(1);
  const ezColor color = ezTypeScriptBinding::GetColor(pDuk, 2);

  ezDebugRenderer::DrawCross(pWorld, pos, size, color);

  return duk.ReturnVoid();
}

static int __CPP_World_Debug_DrawLines(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  const ezColor color = ezTypeScriptBinding::GetColor(pDuk, 1);

  ezUInt32 uiNumLines = (ezUInt32)duk_get_length(pDuk, 0);
  ezHybridArray<ezDebugRenderer::Line, 32> lines;
  lines.SetCount(uiNumLines);

  for (ezUInt32 i = 0; i < uiNumLines; ++i)
  {
    auto& line = lines[i];

    duk_get_prop_index(pDuk, 0, i);

    line.m_start.x = duk.GetFloatProperty("startX", 0.0f, -1);
    line.m_start.y = duk.GetFloatProperty("startY", 0.0f, -1);
    line.m_start.z = duk.GetFloatProperty("startZ", 0.0f, -1);

    line.m_end.x = duk.GetFloatProperty("endX", 0.0f, -1);
    line.m_end.y = duk.GetFloatProperty("endY", 0.0f, -1);
    line.m_end.z = duk.GetFloatProperty("endZ", 0.0f, -1);

    duk_pop(pDuk);
  }

  ezDebugRenderer::DrawLines(pWorld, lines, color);

  return duk.ReturnVoid();
}

static int __CPP_World_Debug_DrawBox(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  const ezVec3 vMin = ezTypeScriptBinding::GetVec3(pDuk, 0);
  const ezVec3 vMax = ezTypeScriptBinding::GetVec3(pDuk, 1);
  const ezColor color = ezTypeScriptBinding::GetColor(pDuk, 2);
  const ezTransform transform = ezTypeScriptBinding::GetTransform(pDuk, 3);

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      ezDebugRenderer::DrawLineBox(pWorld, ezBoundingBox(vMin, vMax), color, transform);
      break;

    case 1:
      ezDebugRenderer::DrawSolidBox(pWorld, ezBoundingBox(vMin, vMax), color, transform);
      break;
  }

  return duk.ReturnVoid();
}

static int __CPP_World_Debug_DrawSphere(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  const ezVec3 vCenter = ezTypeScriptBinding::GetVec3(pDuk, 0);
  const float fRadius = duk.GetFloatValue(1);
  const ezColor color = ezTypeScriptBinding::GetColor(pDuk, 2);
  const ezTransform transform = ezTypeScriptBinding::GetTransform(pDuk, 3);

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      ezDebugRenderer::DrawLineSphere(pWorld, ezBoundingSphere(vCenter, fRadius), color, transform);
      break;
  }

  return duk.ReturnVoid();
}

static int __CPP_World_Debug_Draw3DText(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  const char* szText = duk.GetStringValue(0);
  const ezVec3 vPos = ezTypeScriptBinding::GetVec3(pDuk, 1);
  const ezColor color = ezTypeScriptBinding::GetColor(pDuk, 2);
  const float fSize = duk.GetFloatValue(3, 16.0f);

  ezDebugRenderer::Draw3DText(pWorld, szText, vPos, color, fSize, ezDebugRenderer::HorizontalAlignment::Center, ezDebugRenderer::VerticalAlignment::Center);

  return duk.ReturnVoid();
}
