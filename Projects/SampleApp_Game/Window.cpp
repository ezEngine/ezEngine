#include "Window.h"
#include <gl/GL.h>

GameWindow::GameWindow()
{
  m_CreationDescription.m_ClientAreaSize.width = 500;
  m_CreationDescription.m_ClientAreaSize.height = 500;
  m_CreationDescription.m_Title = "SampleApp_Game";
  m_CreationDescription.m_GraphicsAPI = ezGraphicsAPI::OpenGL;
  Initialize();
}

GameWindow::~GameWindow()
{
  Destroy();
}

void GameWindow::OnResizeMessage(const ezSizeU32& newWindowSize)
{
  m_CreationDescription.m_ClientAreaSize = newWindowSize;
}

ezSizeU32 GameWindow::GetResolution() const
{
  return m_CreationDescription.m_ClientAreaSize;
}

