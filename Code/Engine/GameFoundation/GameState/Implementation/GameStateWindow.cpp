#include <GameFoundation/PCH.h>
#include <GameFoundation/GameState/GameStateWindow.h>


ezGameStateWindow::ezGameStateWindow(const ezWindowCreationDesc& windowdesc)
{
  m_CreationDescription = windowdesc;
  m_CreationDescription.AdjustWindowSizeAndPosition();

  Initialize();
}

ezGameStateWindow::~ezGameStateWindow()
{
  Destroy();
}

void ezGameStateWindow::OnResizeMessage(const ezSizeU32& newWindowSize)
{
  ezLog::Info("Resolution changed to {0} * {1}", newWindowSize.width, newWindowSize.height);

  m_CreationDescription.m_Resolution = newWindowSize;
}
