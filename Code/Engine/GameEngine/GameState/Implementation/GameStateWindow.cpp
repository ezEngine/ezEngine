#include <PCH.h>
#include <GameEngine/GameState/GameStateWindow.h>


ezGameStateWindow::ezGameStateWindow(const ezWindowCreationDesc& windowdesc, ezDelegate<void()> onClickClose)
  : m_OnClickClose(onClickClose)
{
  m_CreationDescription = windowdesc;
  m_CreationDescription.AdjustWindowSizeAndPosition();

  Initialize();
}

ezGameStateWindow::~ezGameStateWindow()
{
  Destroy();
}


void ezGameStateWindow::OnClickCloseMessage()
{
  if (m_OnClickClose.IsValid())
  {
    m_OnClickClose();
  }
}

void ezGameStateWindow::OnResizeMessage(const ezSizeU32& newWindowSize)
{
  ezLog::Info("Resolution changed to {0} * {1}", newWindowSize.width, newWindowSize.height);

  m_CreationDescription.m_Resolution = newWindowSize;
}
