#pragma once

#include <GameFoundation/Basics.h>
#include <GameFoundation/Declarations.h>
#include <System/Window/Window.h>

/// \brief A window class that expands a little on ezWindow. Default type used by ezGameState to create a window.
class ezGameStateWindow : public ezWindow
{
public:
  ezGameStateWindow(const ezWindowCreationDesc& windowdesc);
  ~ezGameStateWindow();

private:
  virtual void OnResizeMessage(const ezSizeU32& newWindowSize) override;

};
