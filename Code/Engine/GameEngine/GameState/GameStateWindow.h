#pragma once

#include <GameEngine/Declarations.h>
#include <GameEngine/GameEngineDLL.h>
#include <System/Window/Window.h>

/// \brief A window class that expands a little on ezWindow. Default type used by ezGameState to create a window.
class EZ_GAMEENGINE_DLL ezGameStateWindow : public ezWindow
{
public:
  ezGameStateWindow(const ezWindowCreationDesc& windowdesc, ezDelegate<void()> onClickClose = nullptr);
  ~ezGameStateWindow();

  void ResetOnClickClose(ezDelegate<void()> onClickClose);

private:
  virtual void OnResize(const ezSizeU32& newWindowSize) override;
  virtual void OnClickClose() override;

  ezDelegate<void()> m_OnClickClose;
};
