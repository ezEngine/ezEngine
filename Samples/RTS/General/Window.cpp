#include <PCH.h>
#include <RTS/General/Window.h>
#include <gl/GL.h>
#include <Foundation/Logging/Log.h>

GameWindow::GameWindow()
{
  m_CreationDescription.m_ClientAreaSize.width = 800;
  m_CreationDescription.m_ClientAreaSize.height = 800;
  m_CreationDescription.m_Title = "RTS Sample";
  m_CreationDescription.m_bFullscreenWindow = false;
  m_CreationDescription.m_bResizable = true;
  Initialize();

  m_bMinimized = false;
}

GameWindow::~GameWindow()
{
  Destroy();
}

void GameWindow::OnResizeMessage(const ezSizeU32& newWindowSize)
{
  m_bMinimized = (newWindowSize.width == 0) && (newWindowSize.height == 0);

  ezLog::Info("Resolution changed to %i * %i", newWindowSize.width, newWindowSize.height);

  m_CreationDescription.m_ClientAreaSize = newWindowSize;
}

ezSizeU32 GameWindow::GetResolution() const
{
  return m_CreationDescription.m_ClientAreaSize;
}

