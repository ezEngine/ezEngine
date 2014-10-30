#include "Window.h"
#include <Foundation/Logging/Log.h>

GameWindow::GameWindow()
{
  m_CreationDescription.m_ClientAreaSize.width = 500;
  m_CreationDescription.m_ClientAreaSize.height = 500;
  m_CreationDescription.m_Title = "SampleApp_Game";
  m_CreationDescription.m_bFullscreenWindow = false;
  m_CreationDescription.m_bResizable = true;
  Initialize();
}

GameWindow::~GameWindow()
{
  Destroy();
}

void GameWindow::OnResizeMessage(const ezSizeU32& newWindowSize)
{
  ezLog::Info("Resolution changed to %i * %i", newWindowSize.width, newWindowSize.height);

  m_CreationDescription.m_ClientAreaSize = newWindowSize;
}

