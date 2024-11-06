#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/VirtualThumbStick.h>
#include <Foundation/Math/Rect.h>
#include <GameEngine/Input/InputDebugVis.h>
#include <RendererCore/Debug/DebugRenderer.h>

void ezInputDebugVis::DebugRender(const ezDebugRendererContext& context, const ezVec2& resolution, const ezVirtualThumbStick& stick)
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
  area.x = vLL.x * resolution.x;
  area.y = vLL.y * resolution.y;
  area.width = vAreaSize.x * resolution.x;
  area.height = vAreaSize.y * resolution.y;

  ezDebugRenderer::Draw2DLineRectangle(context, area, 0.0f, bActive ? ezColor::Yellow : ezColor::Grey);

  area.x = (vCenter.x - fRadius) * resolution.x;
  area.y = (vCenter.y * resolution.y) - (fRadius * resolution.y * fAspect);
  area.width = fRadius * 2 * resolution.x;
  area.height = fRadius * 2 * resolution.y * fAspect;
  ezDebugRenderer::Draw2DLineRectangle(context, area, 0.0f, bActive ? ezColor::GreenYellow : ezColor::Yellow);

  if (bActive)
  {
    const float size = 0.03f;
    area.x = (vTouchPos.x - size) * resolution.x;
    area.y = (vTouchPos.y * resolution.y) - (size * resolution.y * fAspect);
    area.width = size * 2 * resolution.x;
    area.height = size * 2 * resolution.y * fAspect;
    ezDebugRenderer::Draw2DRectangle(context, area, 0.0f, ezColor::OrangeRed);


    ezVec2I32 pos;
    pos.x = ezMath::RoundToInt(vCenter.x * resolution.x);
    pos.y = ezMath::RoundToInt(vCenter.y * resolution.y);

    ezDebugRenderer::Draw2DText(context, ezFmt("{}", ezArgF(fStrength, 2)), pos, ezColor::OrangeRed, 16, ezDebugTextHAlign::Center, ezDebugTextVAlign::Center);
  }
}
