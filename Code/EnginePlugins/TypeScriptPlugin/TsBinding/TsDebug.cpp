#include <TypeScriptPluginPCH.h>

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
static int __CPP_Debug_GetResolution(duk_context* pDuk);
static int __CPP_Debug_ReadCVar(duk_context* pDuk);
static int __CPP_Debug_WriteCVar(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_Debug()
{
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawCross", __CPP_Debug_DrawCross, 3);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawLines", __CPP_Debug_DrawLines, 2, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_Draw2DLines", __CPP_Debug_DrawLines, 2, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawLineBox", __CPP_Debug_DrawBox, 4, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawSolidBox", __CPP_Debug_DrawBox, 4, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawLineSphere", __CPP_Debug_DrawSphere, 4);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_Draw2DText", __CPP_Debug_Draw2DText, 5);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_Draw3DText", __CPP_Debug_Draw3DText, 4);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_GetResolution", __CPP_Debug_GetResolution, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_ReadCVarBool", __CPP_Debug_ReadCVar, 1, ezCVarType::Bool);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_ReadCVarInt", __CPP_Debug_ReadCVar, 1, ezCVarType::Int);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_ReadCVarFloat", __CPP_Debug_ReadCVar, 1, ezCVarType::Float);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_ReadCVarString", __CPP_Debug_ReadCVar, 1, ezCVarType::String);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_WriteCVarBool", __CPP_Debug_WriteCVar, 2, ezCVarType::Bool);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_WriteCVarInt", __CPP_Debug_WriteCVar, 2, ezCVarType::Int);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_WriteCVarFloat", __CPP_Debug_WriteCVar, 2, ezCVarType::Float);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_WriteCVarString", __CPP_Debug_WriteCVar, 2, ezCVarType::String);

  return EZ_SUCCESS;
}

static int __CPP_Debug_DrawCross(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  const ezVec3 pos = ezTypeScriptBinding::GetVec3(pDuk, 0);
  const float size = duk.GetFloatValue(1);
  const ezColor color = ezTypeScriptBinding::GetColor(pDuk, 2);

  ezDebugRenderer::DrawCross(pWorld, pos, size, color);

  return duk.ReturnVoid();
}

static int __CPP_Debug_DrawLines(duk_context* pDuk)
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
      ezDebugRenderer::DrawLineBox(pWorld, ezBoundingBox(vMin, vMax), color, transform);
      break;

    case 1:
      ezDebugRenderer::DrawSolidBox(pWorld, ezBoundingBox(vMin, vMax), color, transform);
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
      ezDebugRenderer::DrawLineSphere(pWorld, ezBoundingSphere(vCenter, fRadius), color, transform);
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
  ezDebugRenderer::HorizontalAlignment::Enum halign = (ezDebugRenderer::HorizontalAlignment::Enum)duk.GetIntValue(4);

  ezDebugRenderer::Draw2DText(pWorld, szText, ezVec2I32((int)vPos.x, (int)vPos.y), color, fSize, halign, ezDebugRenderer::VerticalAlignment::Top);

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

  ezDebugRenderer::Draw3DText(pWorld, szText, vPos, color, fSize, ezDebugRenderer::HorizontalAlignment::Center, ezDebugRenderer::VerticalAlignment::Center);

  return duk.ReturnVoid();
}

static int __CPP_Debug_GetResolution(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  ezVec2 resolution;

  for (const ezViewHandle& hView : ezRenderWorld::GetMainViews())
  {
    ezView* pView;
    if (ezRenderWorld::TryGetView(hView, pView))
    {
      resolution.x = pView->GetViewport().width;
      resolution.y = pView->GetViewport().height;
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
