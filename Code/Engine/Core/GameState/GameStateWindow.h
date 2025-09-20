#pragma once

#include <Core/System/Window.h>

/// A window class that expands on ezWindow with game-specific functionality.
///
/// Default window type used by ezGameState to create a game window.
/// Provides customizable close behavior through delegate callbacks.
class EZ_CORE_DLL ezGameStateWindow : public ezWindow
{
public:
  ezGameStateWindow(const ezWindowCreationDesc& windowdesc, ezDelegate<void()> onClickClose = {});
  ~ezGameStateWindow();

  void ResetOnClickClose(ezDelegate<void()> onClickClose);

private:
  virtual void OnResize(const ezSizeU32& newWindowSize) override;
  virtual void OnClickClose() override;

  ezDelegate<void()> m_OnClickClose;
};
