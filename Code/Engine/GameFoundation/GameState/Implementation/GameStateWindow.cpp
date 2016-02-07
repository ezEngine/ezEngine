#include <GameFoundation/PCH.h>
#include <GameFoundation/GameState/GameStateWindow.h>


ezGameStateWindow::ezGameStateWindow()
{
  /// \todo Make this configurable

  m_CreationDescription.m_ClientAreaSize.width = 720 * 16 / 9;
  m_CreationDescription.m_ClientAreaSize.height = 720;
  m_CreationDescription.m_Title = "SampleApp";
  m_CreationDescription.m_bFullscreenWindow = false;
  m_CreationDescription.m_bResizable = false;

  Initialize();
}

ezGameStateWindow::~ezGameStateWindow()
{
  Destroy();
}

void ezGameStateWindow::OnResizeMessage(const ezSizeU32& newWindowSize)
{
  ezLog::Info("Resolution changed to %i * %i", newWindowSize.width, newWindowSize.height);

  m_CreationDescription.m_ClientAreaSize = newWindowSize;
}
