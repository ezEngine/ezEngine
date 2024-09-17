#include <Core/CorePCH.h>

#include <Core/System/Window.h>
#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/System/SystemInformation.h>

ezResult ezWindow::Initialize()
{
  EZ_LOG_BLOCK("ezWindow::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
  {
    Destroy().AssertSuccess();
  }

  m_hWindowHandle = nullptr; // TODO WebGPU: can we hold a reference to the canvas ? (should we?)
  m_pInputDevice = EZ_DEFAULT_NEW(ezStandardInputDevice);
  m_bInitialized = true;

  return EZ_SUCCESS;
}

ezResult ezWindow::Destroy()
{
  if (!m_bInitialized)
    return EZ_SUCCESS;

  EZ_LOG_BLOCK("ezWindow::Destroy");

  m_bInitialized = false;
  m_pInputDevice.Clear();

  ezLog::Success("Window destroyed.");
  return EZ_SUCCESS;
}

ezResult ezWindow::Resize(const ezSizeU32& newWindowSize)
{
  m_CreationDescription.m_Resolution.width = newWindowSize.width;
  m_CreationDescription.m_Resolution.height = newWindowSize.height;
  return EZ_SUCCESS;
}

void ezWindow::ProcessWindowMessages()
{
}

void ezWindow::OnResize(const ezSizeU32& newWindowSize)
{
  ezLog::Info("Window resized to ({0}, {1})", newWindowSize.width, newWindowSize.height);
}

ezWindowHandle ezWindow::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}
