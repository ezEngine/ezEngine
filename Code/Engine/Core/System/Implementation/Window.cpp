#include <Core/CorePCH.h>

#include <Core/System/Window.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/System/Screen.h>

#if EZ_ENABLED(EZ_SUPPORTS_GLFW)
#  include <Core/System/Implementation/glfw/InputDevice_glfw.inl>
#  include <Core/System/Implementation/glfw/Window_glfw.inl>
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  include <Core/System/Implementation/Win/InputDevice_win32.inl>
#  include <Core/System/Implementation/Win/Window_win32.inl>
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#  include <Core/System/Implementation/uwp/InputDevice_uwp.inl>
#  include <Core/System/Implementation/uwp/Window_uwp.inl>
#elif EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Core/System/Implementation/android/InputDevice_android.inl>
#  include <Core/System/Implementation/android/Window_android.inl>
#else
#  include <Core/System/Implementation/null/InputDevice_null.inl>
#  include <Core/System/Implementation/null/Window_null.inl>
#endif

ezUInt8 ezWindow::s_uiNextUnusedWindowNumber = 0;

ezResult ezWindowCreationDesc::AdjustWindowSizeAndPosition()
{
  ezHybridArray<ezScreenInfo, 2> screens;
  if (ezScreen::EnumerateScreens(screens).Failed() || screens.IsEmpty())
    return EZ_FAILURE;

  ezInt32 iShowOnMonitor = m_iMonitor;

  if (iShowOnMonitor >= (ezInt32)screens.GetCount())
    iShowOnMonitor = -1;

  const ezScreenInfo* pScreen = nullptr;

  // this means 'pick the primary screen'
  if (iShowOnMonitor < 0)
  {
    pScreen = &screens[0];

    for (ezUInt32 i = 0; i < screens.GetCount(); ++i)
    {
      if (screens[i].m_bIsPrimary)
      {
        pScreen = &screens[i];
        break;
      }
    }
  }
  else
  {
    pScreen = &screens[iShowOnMonitor];
  }

  if (m_WindowMode == ezWindowMode::FullscreenBorderlessNativeResolution)
  {
    m_Resolution.width = pScreen->m_iResolutionX;
    m_Resolution.height = pScreen->m_iResolutionY;
  }
  else
  {
    // clamp the resolution to the native resolution ?
    // m_ClientAreaSize.width = ezMath::Min<ezUInt32>(m_ClientAreaSize.width, pScreen->m_iResolutionX);
    // m_ClientAreaSize.height= ezMath::Min<ezUInt32>(m_ClientAreaSize.height,pScreen->m_iResolutionY);
  }

  if (m_bCenterWindowOnDisplay)
  {
    m_Position.Set(pScreen->m_iOffsetX + (pScreen->m_iResolutionX - (ezInt32)m_Resolution.width) / 2, pScreen->m_iOffsetY + (pScreen->m_iResolutionY - (ezInt32)m_Resolution.height) / 2);
  }
  else
  {
    m_Position.Set(pScreen->m_iOffsetX, pScreen->m_iOffsetY);
  }

  return EZ_SUCCESS;
}

void ezWindowCreationDesc::SaveToDDL(ezOpenDdlWriter& ref_writer)
{
  ref_writer.BeginObject("WindowDesc");

  ezOpenDdlUtils::StoreString(ref_writer, m_Title, "Title");

  switch (m_WindowMode.GetValue())
  {
    case ezWindowMode::FullscreenBorderlessNativeResolution:
      ezOpenDdlUtils::StoreString(ref_writer, "Borderless", "Mode");
      break;
    case ezWindowMode::FullscreenFixedResolution:
      ezOpenDdlUtils::StoreString(ref_writer, "Fullscreen", "Mode");
      break;
    case ezWindowMode::WindowFixedResolution:
      ezOpenDdlUtils::StoreString(ref_writer, "Window", "Mode");
      break;
    case ezWindowMode::WindowResizable:
      ezOpenDdlUtils::StoreString(ref_writer, "ResizableWindow", "Mode");
      break;
  }

  if (m_uiWindowNumber != 0)
    ezOpenDdlUtils::StoreUInt8(ref_writer, m_uiWindowNumber, "Index");

  if (m_iMonitor >= 0)
    ezOpenDdlUtils::StoreInt8(ref_writer, m_iMonitor, "Monitor");

  if (m_Position != ezVec2I32(0x80000000, 0x80000000))
  {
    ezOpenDdlUtils::StoreVec2I(ref_writer, m_Position, "Position");
  }

  ezOpenDdlUtils::StoreVec2U(ref_writer, ezVec2U32(m_Resolution.width, m_Resolution.height), "Resolution");

  ezOpenDdlUtils::StoreBool(ref_writer, m_bClipMouseCursor, "ClipMouseCursor");
  ezOpenDdlUtils::StoreBool(ref_writer, m_bShowMouseCursor, "ShowMouseCursor");
  ezOpenDdlUtils::StoreBool(ref_writer, m_bSetForegroundOnInit, "SetForegroundOnInit");
  ezOpenDdlUtils::StoreBool(ref_writer, m_bCenterWindowOnDisplay, "CenterWindowOnDisplay");

  ref_writer.EndObject();
}


ezResult ezWindowCreationDesc::SaveToDDL(ezStringView sFile)
{
  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(sFile));

  ezOpenDdlWriter writer;
  writer.SetOutputStream(&file);

  SaveToDDL(writer);

  return EZ_SUCCESS;
}

void ezWindowCreationDesc::LoadFromDDL(const ezOpenDdlReaderElement* pParentElement)
{
  if (const ezOpenDdlReaderElement* pDesc = pParentElement->FindChildOfType("WindowDesc"))
  {
    if (const ezOpenDdlReaderElement* pTitle = pDesc->FindChildOfType(ezOpenDdlPrimitiveType::String, "Title"))
      m_Title = pTitle->GetPrimitivesString()[0];

    if (const ezOpenDdlReaderElement* pMode = pDesc->FindChildOfType(ezOpenDdlPrimitiveType::String, "Mode"))
    {
      auto mode = pMode->GetPrimitivesString()[0];

      if (mode == "Borderless")
        m_WindowMode = ezWindowMode::FullscreenBorderlessNativeResolution;
      else if (mode == "Fullscreen")
        m_WindowMode = ezWindowMode::FullscreenFixedResolution;
      else if (mode == "Window")
        m_WindowMode = ezWindowMode::WindowFixedResolution;
      else if (mode == "ResizableWindow")
        m_WindowMode = ezWindowMode::WindowResizable;
    }

    if (const ezOpenDdlReaderElement* pIndex = pDesc->FindChildOfType(ezOpenDdlPrimitiveType::UInt8, "Index"))
    {
      m_uiWindowNumber = pIndex->GetPrimitivesUInt8()[0];
    }

    if (const ezOpenDdlReaderElement* pMonitor = pDesc->FindChildOfType(ezOpenDdlPrimitiveType::Int8, "Monitor"))
    {
      m_iMonitor = pMonitor->GetPrimitivesInt8()[0];
    }

    if (const ezOpenDdlReaderElement* pPosition = pDesc->FindChild("Position"))
    {
      ezOpenDdlUtils::ConvertToVec2I(pPosition, m_Position).IgnoreResult();
    }

    if (const ezOpenDdlReaderElement* pPosition = pDesc->FindChild("Resolution"))
    {
      ezVec2U32 res;
      ezOpenDdlUtils::ConvertToVec2U(pPosition, res).IgnoreResult();
      m_Resolution.width = res.x;
      m_Resolution.height = res.y;
    }

    if (const ezOpenDdlReaderElement* pClipMouseCursor = pDesc->FindChildOfType(ezOpenDdlPrimitiveType::Bool, "ClipMouseCursor"))
      m_bClipMouseCursor = pClipMouseCursor->GetPrimitivesBool()[0];

    if (const ezOpenDdlReaderElement* pShowMouseCursor = pDesc->FindChildOfType(ezOpenDdlPrimitiveType::Bool, "ShowMouseCursor"))
      m_bShowMouseCursor = pShowMouseCursor->GetPrimitivesBool()[0];

    if (const ezOpenDdlReaderElement* pSetForegroundOnInit = pDesc->FindChildOfType(ezOpenDdlPrimitiveType::Bool, "SetForegroundOnInit"))
      m_bSetForegroundOnInit = pSetForegroundOnInit->GetPrimitivesBool()[0];

    if (const ezOpenDdlReaderElement* pCenterWindowOnDisplay = pDesc->FindChildOfType(ezOpenDdlPrimitiveType::Bool, "CenterWindowOnDisplay"))
      m_bCenterWindowOnDisplay = pCenterWindowOnDisplay->GetPrimitivesBool()[0];
  }
}

ezResult ezWindowCreationDesc::LoadFromDDL(ezStringView sFile)
{
  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(sFile));

  ezOpenDdlReader reader;
  EZ_SUCCEED_OR_RETURN(reader.ParseDocument(file));

  LoadFromDDL(reader.GetRootElement());

  return EZ_SUCCESS;
}

ezWindow::ezWindow()
{
  ++s_uiNextUnusedWindowNumber;
}

ezWindow::~ezWindow()
{
  if (m_bInitialized)
  {
    Destroy().IgnoreResult();
  }
  EZ_ASSERT_DEV(m_iReferenceCount == 0, "The window is still being referenced, probably by a swapchain. Make sure to destroy all swapchains and call ezGALDevice::WaitIdle before destroying a window.");
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
void ezWindow::OnWindowMessage(ezMinWindows::HWND hWnd, ezMinWindows::UINT msg, ezMinWindows::WPARAM wparam, ezMinWindows::LPARAM lparam)
{
  EZ_IGNORE_UNUSED(hWnd);
  EZ_IGNORE_UNUSED(msg);
  EZ_IGNORE_UNUSED(wparam);
  EZ_IGNORE_UNUSED(lparam);
}
#endif

ezUInt8 ezWindow::GetNextUnusedWindowNumber()
{
  return s_uiNextUnusedWindowNumber;
}


