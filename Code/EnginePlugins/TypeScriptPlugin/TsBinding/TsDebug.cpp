#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/Console/ConsoleFunction.h>
#include <Duktape/duktape.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Debug_DrawCross(duk_context* pDuk);
static int __CPP_Debug_DrawLines(duk_context* pDuk);
static int __CPP_Debug_DrawBox(duk_context* pDuk);
static int __CPP_Debug_DrawSphere(duk_context* pDuk);
static int __CPP_Debug_Draw2DText(duk_context* pDuk);
static int __CPP_Debug_Draw3DText(duk_context* pDuk);
static int __CPP_Debug_DrawInfoText(duk_context* pDuk);
static int __CPP_Debug_GetResolution(duk_context* pDuk);
static int __CPP_Debug_ReadCVar(duk_context* pDuk);
static int __CPP_Debug_WriteCVar(duk_context* pDuk);
static int __CPP_Debug_RegisterCVar(duk_context* pDuk);
static int __CPP_Debug_RegisterCFunc(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_Debug()
{
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawCross", __CPP_Debug_DrawCross, 4);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawLines", __CPP_Debug_DrawLines, 2, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_Draw2DLines", __CPP_Debug_DrawLines, 2, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawLineBox", __CPP_Debug_DrawBox, 4, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawSolidBox", __CPP_Debug_DrawBox, 4, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawLineSphere", __CPP_Debug_DrawSphere, 4);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_Draw2DText", __CPP_Debug_Draw2DText, 5);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_Draw3DText", __CPP_Debug_Draw3DText, 4);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawInfoText", __CPP_Debug_DrawInfoText, 3);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_GetResolution", __CPP_Debug_GetResolution, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_ReadCVarBool", __CPP_Debug_ReadCVar, 1, ezCVarType::Bool);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_ReadCVarInt", __CPP_Debug_ReadCVar, 1, ezCVarType::Int);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_ReadCVarFloat", __CPP_Debug_ReadCVar, 1, ezCVarType::Float);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_ReadCVarString", __CPP_Debug_ReadCVar, 1, ezCVarType::String);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_WriteCVarBool", __CPP_Debug_WriteCVar, 2, ezCVarType::Bool);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_WriteCVarInt", __CPP_Debug_WriteCVar, 2, ezCVarType::Int);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_WriteCVarFloat", __CPP_Debug_WriteCVar, 2, ezCVarType::Float);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_WriteCVarString", __CPP_Debug_WriteCVar, 2, ezCVarType::String);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_RegisterCVar", __CPP_Debug_RegisterCVar, 4);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_Debug_RegisterCFunc", __CPP_Debug_RegisterCFunc);

  return EZ_SUCCESS;
}

static int __CPP_Debug_DrawCross(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  const ezVec3 pos = ezTypeScriptBinding::GetVec3(pDuk, 0);
  const float size = duk.GetFloatValue(1);
  const ezColor color = ezTypeScriptBinding::GetColor(pDuk, 2);
  const ezTransform transform = ezTypeScriptBinding::GetTransform(pDuk, 3);

  ezDebugRenderer::DrawCross(pWorld, pos, size, color, transform);

  return duk.ReturnVoid();
}

static int __CPP_Debug_DrawLines(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  const ezColor color = ezTypeScriptBinding::GetColor(pDuk, 1);

  ezUInt32 uiNumLines = (ezUInt32)duk_get_length(pDuk, 0);
  ezHybridArray<ezDebugRendererLine, 32> lines;
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

  if (duk.GetFunctionMagicValue() == 0)
  {
    ezDebugRenderer::DrawLines(pWorld, lines, color);
  }
  else
  {
    ezDebugRenderer::Draw2DLines(pWorld, lines, color);
  }

  return duk.ReturnVoid();
}

static int __CPP_Debug_DrawBox(duk_context* pDuk)
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
      ezDebugRenderer::DrawLineBox(pWorld, ezBoundingBox::MakeFromMinMax(vMin, vMax), color, transform);
      break;

    case 1:
      ezDebugRenderer::DrawSolidBox(pWorld, ezBoundingBox::MakeFromMinMax(vMin, vMax), color, transform);
      break;
  }

  return duk.ReturnVoid();
}

static int __CPP_Debug_DrawSphere(duk_context* pDuk)
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
      ezDebugRenderer::DrawLineSphere(pWorld, ezBoundingSphere::MakeFromCenterAndRadius(vCenter, fRadius), color, transform);
      break;
  }

  return duk.ReturnVoid();
}

static int __CPP_Debug_Draw2DText(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  const char* szText = duk.GetStringValue(0);
  const ezVec2 vPos = ezTypeScriptBinding::GetVec2(pDuk, 1);
  const ezColor color = ezTypeScriptBinding::GetColor(pDuk, 2);
  const float fSize = duk.GetFloatValue(3, 16.0f);
  ezDebugTextHAlign::Enum halign = static_cast<ezDebugTextHAlign::Enum>(duk.GetIntValue(4));

  ezDebugRenderer::Draw2DText(pWorld, szText, ezVec2I32((int)vPos.x, (int)vPos.y), color, (ezUInt32)fSize, halign);

  return duk.ReturnVoid();
}

static int __CPP_Debug_Draw3DText(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  const char* szText = duk.GetStringValue(0);
  const ezVec3 vPos = ezTypeScriptBinding::GetVec3(pDuk, 1);
  const ezColor color = ezTypeScriptBinding::GetColor(pDuk, 2);
  const float fSize = duk.GetFloatValue(3, 16.0f);

  ezDebugRenderer::Draw3DText(pWorld, szText, vPos, color, (ezUInt32)fSize);

  return duk.ReturnVoid();
}

static int __CPP_Debug_DrawInfoText(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  const ezInt32 corner = duk.GetIntValue(0);
  const char* szText = duk.GetStringValue(1);
  const ezColor color = ezTypeScriptBinding::GetColor(pDuk, 2);

  ezDebugRenderer::DrawInfoText(pWorld, static_cast<ezDebugTextPlacement::Enum>(corner), "Script", szText, color);

  return duk.ReturnVoid();
}

static int __CPP_Debug_GetResolution(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezVec2 resolution;

  for (const ezViewHandle& hView : ezRenderWorld::GetMainViews())
  {
    ezView* pView;
    if (ezRenderWorld::TryGetView(hView, pView))
    {
      resolution.x = pView->GetViewport().width;
      resolution.y = pView->GetViewport().height;
      break;
    }
  }

  ezTypeScriptBinding::PushVec2(pDuk, resolution);
  return duk.ReturnCustom();
}

static int __CPP_Debug_ReadCVar(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  const char* szVarName = duk.GetStringValue(0);

  ezCVar* pCVar = ezCVar::FindCVarByName(szVarName);

  if (pCVar == nullptr || pCVar->GetType() != duk.GetFunctionMagicValue())
  {
    return duk.ReturnUndefined();
  }

  switch (duk.GetFunctionMagicValue())
  {
    case ezCVarType::Bool:
    {
      ezCVarBool* pVar = static_cast<ezCVarBool*>(pCVar);
      return duk.ReturnBool(pVar->GetValue());
    }

    case ezCVarType::Int:
    {
      ezCVarInt* pVar = static_cast<ezCVarInt*>(pCVar);
      return duk.ReturnInt(pVar->GetValue());
    }

    case ezCVarType::Float:
    {
      ezCVarFloat* pVar = static_cast<ezCVarFloat*>(pCVar);
      return duk.ReturnNumber(pVar->GetValue());
    }

    case ezCVarType::String:
    {
      ezCVarString* pVar = static_cast<ezCVarString*>(pCVar);
      return duk.ReturnString(pVar->GetValue());
    }

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnUndefined();
}

static int __CPP_Debug_WriteCVar(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  const char* szVarName = duk.GetStringValue(0);

  ezCVar* pCVar = ezCVar::FindCVarByName(szVarName);

  if (pCVar == nullptr || pCVar->GetType() != duk.GetFunctionMagicValue())
  {
    duk.Error(ezFmt("CVar '{}' does not exist.", szVarName));
    return duk.ReturnVoid();
  }

  switch (duk.GetFunctionMagicValue())
  {
    case ezCVarType::Bool:
    {
      ezCVarBool* pVar = static_cast<ezCVarBool*>(pCVar);
      *pVar = duk.GetBoolValue(1, pVar->GetValue());
      break;
    }

    case ezCVarType::Int:
    {
      ezCVarInt* pVar = static_cast<ezCVarInt*>(pCVar);
      *pVar = duk.GetIntValue(1, pVar->GetValue());
      break;
    }

    case ezCVarType::Float:
    {
      ezCVarFloat* pVar = static_cast<ezCVarFloat*>(pCVar);
      *pVar = duk.GetFloatValue(1, pVar->GetValue());
      break;
    }

    case ezCVarType::String:
    {
      ezCVarString* pVar = static_cast<ezCVarString*>(pCVar);
      *pVar = duk.GetStringValue(1, pVar->GetValue());
      break;
    }

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnVoid();
}


static int __CPP_Debug_RegisterCVar(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(pDuk);

  const char* szVarName = duk.GetStringValue(0);
  const ezCVarType::Enum type = (ezCVarType::Enum)duk.GetIntValue(1);
  const char* szDesc = duk.GetStringValue(3);

  auto& pCVar = pBinding->m_CVars[szVarName];

  if (pCVar != nullptr)
    return duk.ReturnVoid();

  switch (type)
  {
    case ezCVarType::Int:
      pCVar = EZ_DEFAULT_NEW(ezCVarInt, szVarName, duk.GetIntValue(2), ezCVarFlags::Default, szDesc);
      break;
    case ezCVarType::Float:
      pCVar = EZ_DEFAULT_NEW(ezCVarFloat, szVarName, duk.GetFloatValue(2), ezCVarFlags::Default, szDesc);
      break;
    case ezCVarType::Bool:
      pCVar = EZ_DEFAULT_NEW(ezCVarBool, szVarName, duk.GetBoolValue(2), ezCVarFlags::Default, szDesc);
      break;
    case ezCVarType::String:
      pCVar = EZ_DEFAULT_NEW(ezCVarString, szVarName, duk.GetStringValue(2), ezCVarFlags::Default, szDesc);
      break;

    default:
      duk.Error(ezFmt("CVar '{}': invalid type {}", szVarName, (int)type));
      return duk.ReturnVoid();
  }

  ezCVar::ListOfCVarsChanged("Scripting");

  return duk.ReturnVoid();
}

class TsConsoleFunc : public ezConsoleFunctionBase
{

public:
  TsConsoleFunc(const char* szFunctionName, const char* szDescription)
    : ezConsoleFunctionBase(szFunctionName, szDescription)
  {
  }

  ezStaticArray<ezVariant::Type::Enum, 8> m_Args;

  ezUInt32 GetNumParameters() const override { return m_Args.GetCount(); }


  ezVariant::Type::Enum GetParameterType(ezUInt32 uiParam) const override { return m_Args[uiParam]; }

  ezResult Call(ezArrayPtr<ezVariant> params) override
  {
    m_pBinding->StoreConsoleFuncCall(this, params);
    return EZ_SUCCESS;
  }

  ezResult DoCall(const ezArrayPtr<ezVariant>& params)
  {
    auto& cm = m_pBinding->m_ConsoleFuncs[GetName()];

    const ezUInt32 uiNumArgs = params.GetCount();

    if (uiNumArgs != m_Args.GetCount())
      return EZ_FAILURE;

    ezDuktapeContext& duk = m_pBinding->GetDukTapeContext();

    // accessing the ezWorld is the reason why the function call is stored and delayed
    // otherwise this would hang indefinitely
    EZ_LOCK(m_pBinding->GetWorld()->GetWriteMarker());

    for (auto& reg : cm.m_Registered)
    {
      ezComponent* pComponent;
      if (!reg.m_hOwner.IsInvalidated() && !m_pBinding->GetWorld()->TryGetComponent(reg.m_hOwner, pComponent))
      {
        reg.m_hOwner.Invalidate();
        continue;
      }

      duk.PushGlobalStash();                             // [ stash ]
      duk_get_prop_index(duk, -1, reg.m_uiFuncStashIdx); // [ stash func ]

      for (ezUInt32 arg = 0; arg < uiNumArgs; ++arg)
      {
        ezResult r = EZ_FAILURE;

        switch (m_Args[arg])
        {
          case ezVariant::Type::Bool:
            duk.PushBool(params[arg].ConvertTo<bool>(&r));
            break;
          case ezVariant::Type::Double:
            duk.PushNumber(params[arg].ConvertTo<double>(&r));
            break;
          case ezVariant::Type::String:
            duk.PushString(params[arg].ConvertTo<ezString>(&r).GetData());
            break;

            EZ_DEFAULT_CASE_NOT_IMPLEMENTED
        }

        if (r.Failed())
        {
          duk.Error(ezFmt("Could not convert cfunc argument {} to expected type {}", arg, (int)m_Args[arg]));
          return EZ_FAILURE;
        }
      }

      duk_call(duk, uiNumArgs); // [ stash result ]
      duk.PopStack(2);          // [ ]
    }

    return EZ_SUCCESS;
  }

  ezTypeScriptBinding* m_pBinding = nullptr;
};

void ezTypeScriptBinding::StoreConsoleFuncCall(ezConsoleFunctionBase* pFunc, const ezArrayPtr<ezVariant>& params)
{
  auto& call = m_CFuncCalls.ExpandAndGetRef();
  call.m_pFunc = pFunc;
  call.m_Arguments = params;
}

void ezTypeScriptBinding::ExecuteConsoleFuncs()
{
  for (auto& call : m_CFuncCalls)
  {
    TsConsoleFunc* pFunc = static_cast<TsConsoleFunc*>(call.m_pFunc);
    pFunc->DoCall(call.m_Arguments.GetArrayPtr()).IgnoreResult();
  }

  m_CFuncCalls.Clear();
}

static int __CPP_Debug_RegisterCFunc(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezTypeScriptBinding* pBinding = ezTypeScriptBinding::RetrieveBinding(pDuk);

  ezComponentHandle hComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk, 0)->GetHandle();
  const char* szName = duk.GetStringValue(1);
  const char* szDesc = duk.GetStringValue(2);

  duk_require_function(pDuk, 3);

  auto& fb1 = pBinding->m_ConsoleFuncs[szName];

  if (fb1.m_pFunc == nullptr)
  {
    auto f = EZ_DEFAULT_NEW(TsConsoleFunc, szName, szDesc);
    f->m_pBinding = pBinding;

    for (ezUInt32 arg = 4; arg < duk.GetNumVarArgFunctionParameters(); ++arg)
    {
      const ezInt32 iArgType = duk.GetIntValue(arg, ezVariant::Type::Invalid);
      f->m_Args.PushBack(static_cast<ezVariant::Type::Enum>(iArgType));
    }

    fb1.m_pFunc = f;
  }
  else
  {
    for (ezUInt32 arg = 4; arg < duk.GetNumVarArgFunctionParameters(); ++arg)
    {
      const ezInt32 iArgType = duk.GetIntValue(arg, ezVariant::Type::Invalid);
      TsConsoleFunc* func = static_cast<TsConsoleFunc*>(fb1.m_pFunc.Borrow());

      if (func->m_Args[arg - 4] != static_cast<ezVariant::Type::Enum>(iArgType))
      {
        duk.Error(ezFmt("Re-registering cfunc '{}' with differing argument {} ({} != {}).", szName, arg - 4, iArgType, func->m_Args[arg - 4]));
        return duk.ReturnVoid();
      }
    }
  }

  auto& fb2 = fb1.m_Registered.ExpandAndGetRef();
  fb2.m_hOwner = hComponent;
  fb2.m_uiFuncStashIdx = pBinding->AcquireStashObjIndex();

  // store a reference to the console function in the stash
  {
    duk.PushGlobalStash();                             // [ stash ]
    duk_dup(duk, 3);                                   // [ stash func ]
    duk_put_prop_index(duk, -2, fb2.m_uiFuncStashIdx); // [ stash ]
    duk.PopStack();                                    // [ ]
  }

  return duk.ReturnVoid();
}
