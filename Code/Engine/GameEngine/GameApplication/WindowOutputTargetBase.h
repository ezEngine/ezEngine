#pragma once

#include <GameEngine/Basics.h>

class ezImage;

/// \brief Base class for window output targets
///
/// A window output target is usually tied tightly to a window (\sa ezWindowBase) and represents the
/// graphics APIs side of the render output.
/// E.g. in a DirectX implementation this would be a swapchain.
///
/// This interface provides the high level functionality that is needed by ezGameApplication to work with
/// the render output.
class EZ_GAMEENGINE_DLL ezWindowOutputTargetBase
{
public:
  virtual ~ezWindowOutputTargetBase() {}
  virtual void Present(bool bEnableVSync) = 0;
  virtual ezResult CaptureImage(ezImage& out_Image) = 0;
};
