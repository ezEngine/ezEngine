
#include <System/PCH.h>
#include <System/Window/Window.h>
#include <Foundation/Math/Size.h>


ezResult ezWindow::Initialize()
{
  EZ_LOG_BLOCK("ezWindow");

  if (m_bInitialized)
    Destroy();

  EZ_ASSERT_RELEASE(m_CreationDescription.m_ClientAreaSize.HasNonZeroArea(), "The client area size can't be zero sized!");

  m_WindowHandle = EZ_DEFAULT_NEW(sf::Window);

  sf::VideoMode Mode;
  Mode.bitsPerPixel = 32;
  Mode.width  = m_CreationDescription.m_ClientAreaSize.width;
  Mode.height = m_CreationDescription.m_ClientAreaSize.height;

  sf::Uint32 uiStyle = 0;

  if (m_CreationDescription.m_bFullscreenWindow)
    uiStyle |= sf::Style::Fullscreen;
  else
  if (m_CreationDescription.m_bResizable)
    uiStyle |= sf::Style::Resize;
  else
    uiStyle |= sf::Style::Titlebar;

  sf::ContextSettings context;
  context.depthBits = 24;
  context.stencilBits = 8;
  context.majorVersion = 4;
  context.minorVersion = 0;
  context.antialiasingLevel = 0;

  m_WindowHandle->create(Mode, m_CreationDescription.m_Title.GetData(), uiStyle, context);
  
  m_bInitialized = true;
  ezLog::Success("Created window successfully.");

  m_pInputDevice = EZ_DEFAULT_NEW(ezStandardInputDevice)(m_WindowHandle, m_CreationDescription.m_uiWindowNumber);

  // make sure the system knows about the actual window dimensions (especially for fullscreen windows)
  OnResizeMessage(ezSizeU32(m_WindowHandle->getSize().x, m_WindowHandle->getSize().y));

  return CreateGraphicsContext();
}

ezResult ezWindow::Destroy()
{
  EZ_DEFAULT_DELETE(m_pInputDevice);

  DestroyGraphicsContext();

  m_WindowHandle->close();

  EZ_DEFAULT_DELETE(m_WindowHandle);
  m_WindowHandle = INVALID_WINDOW_HANDLE_VALUE;

  m_bInitialized = false;

  ezLog::Success("Window destroyed.");
  return EZ_SUCCESS;

}

void ezWindow::ProcessWindowMessages()
{
  if (!m_bInitialized)
    return;

  sf::Event event;
  while (m_WindowHandle->pollEvent(event))
  {
    switch (event.type)
    {
    case sf::Event::Closed:
      OnClickCloseMessage();
      break;

    case sf::Event::Resized:
      OnResizeMessage(ezSizeU32(event.size.width, event.size.height));
      break;

    case sf::Event::LostFocus:
      OnFocusMessage(false);
      break;

    case sf::Event::GainedFocus:
      OnFocusMessage(true);
      break;
    }

    m_pInputDevice->WindowMessage(event);
  }
}

void ezWindow::PresentFrame()
{
  m_WindowHandle->display();
}

ezResult ezWindow::CreateGraphicsContext()
{
  switch (m_CreationDescription.m_GraphicsAPI)
  {
  case ezGraphicsAPI::OpenGL:
    return EZ_SUCCESS;

  default:
    EZ_REPORT_FAILURE("Unknown Graphics API selected.");
  }

  return EZ_FAILURE;
}

ezResult ezWindow::DestroyGraphicsContext()
{
  switch (m_CreationDescription.m_GraphicsAPI)
  {
  case ezGraphicsAPI::OpenGL:
    return EZ_SUCCESS;

  default:
    EZ_REPORT_FAILURE("Unknown Graphics API selected.");
  }

  return EZ_FAILURE;
}

