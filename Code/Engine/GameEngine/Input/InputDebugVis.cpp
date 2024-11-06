#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/VirtualThumbStick.h>
#include <Foundation/Math/Rect.h>
#include <GameEngine/Input/InputDebugVis.h>
#include <RendererCore/Debug/DebugRenderer.h>

void ezInputDebugVis::DebugRender(const ezDebugRendererContext& context, const ezVec2& vResolution, const ezVirtualThumbStick& stick)
{
  if (!stick.IsEnabled())
    return;

  const bool bActive = stick.IsActive();

  ezVec2 vLL, vUR;
  stick.GetInputArea(vLL, vUR);
  const ezVec2 vAreaSize = vUR - vLL;

  const ezVec2 vCenter = stick.GetCurrentCenter();
  const ezVec2 vTouchPos = stick.GetCurrentTouchPos();
  const float fRadius = stick.GetThumbstickRadius();
  const float fStrength = stick.GetInputStrength();
  const float fAspect = stick.GetInputCoordinateAspectRatio();


  ezRectFloat area;
  area.x = vLL.x * vResolution.x;
  area.y = vLL.y * vResolution.y;
  area.width = vAreaSize.x * vResolution.x;
  area.height = vAreaSize.y * vResolution.y;

  ezDebugRenderer::Draw2DLineRectangle(context, area, 0.0f, bActive ? ezColor::Yellow : ezColor::Grey);

  area.x = (vCenter.x - fRadius) * vResolution.x;
  area.y = (vCenter.y * vResolution.y) - (fRadius * vResolution.y * fAspect);
  area.width = fRadius * 2 * vResolution.x;
  area.height = fRadius * 2 * vResolution.y * fAspect;
  ezDebugRenderer::Draw2DLineRectangle(context, area, 0.0f, bActive ? ezColor::GreenYellow : ezColor::Yellow);

  if (bActive)
  {
    const float size = 0.03f;
    area.x = (vTouchPos.x - size) * vResolution.x;
    area.y = (vTouchPos.y * vResolution.y) - (size * vResolution.y * fAspect);
    area.width = size * 2 * vResolution.x;
    area.height = size * 2 * vResolution.y * fAspect;
    ezDebugRenderer::Draw2DRectangle(context, area, 0.0f, ezColor::OrangeRed);


    ezVec2I32 pos;
    pos.x = ezMath::RoundToInt(vCenter.x * vResolution.x);
    pos.y = ezMath::RoundToInt(vCenter.y * vResolution.y);

    ezDebugRenderer::Draw2DText(context, ezFmt("{}", ezArgF(fStrength, 2)), pos, ezColor::OrangeRed, 16, ezDebugTextHAlign::Center, ezDebugTextVAlign::Center);
  }
}
