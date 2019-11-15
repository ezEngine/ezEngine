#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Debug_DrawCross(duk_context* pDuk);
static int __CPP_Debug_DrawLines(duk_context* pDuk);
static int __CPP_Debug_DrawBox(duk_context* pDuk);
static int __CPP_Debug_DrawSphere(duk_context* pDuk);
static int __CPP_Debug_Draw3DText(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_Debug()
{
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawCross", __CPP_Debug_DrawCross, 3);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawLines", __CPP_Debug_DrawLines, 2);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawLineBox", __CPP_Debug_DrawBox, 4, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawSolidBox", __CPP_Debug_DrawBox, 4, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawLineSphere", __CPP_Debug_DrawSphere, 4);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_Draw3DText", __CPP_Debug_Draw3DText, 4);

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

  ezDebugRenderer::DrawLines(pWorld, lines, color);

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
