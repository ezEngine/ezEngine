#pragma once

#include <GameEngine/GameEngineDLL.h>

class ezDebugRendererContext;
class ezVirtualThumbStick;

namespace ezInputDebugVis
{
  /// \brief Renders a debug visualization of the given thumbstick using the 2D screen space debug render functions.
  EZ_GAMEENGINE_DLL void DebugRender(const ezDebugRendererContext& context, const ezVec2& vResolution, const ezVirtualThumbStick& stick);

}; // namespace ezInputDebugVis
