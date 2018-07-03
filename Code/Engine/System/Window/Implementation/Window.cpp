#include <PCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <System/Screen/Screen.h>
#include <System/Window/Window.h>

#if EZ_ENABLED(EZ_SUPPORTS_SFML)
#include <System/Window/Implementation/SFML/InputDevice_SFML.inl>
#include <System/Window/Implementation/SFML/Window_SFML.inl>
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#include <System/Window/Implementation/Win32/InputDevice_win32.inl>
#include <System/Window/Implementation/Win32/Window_win32.inl>
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#include <System/Window/Implementation/uwp/InputDevice_uwp.inl>
#include <System/Window/Implementation/uwp/Window_uwp.inl>
#else
#error "Missing code for ezWindow!"
#endif

ezResult ezWindowCreationDesc::AdjustWindowSizeAndPosition()
{
  if (m_WindowMode == ezWindowMode::WindowFixedResolution || m_WindowMode == ezWindowMode::WindowResizable)
    return EZ_SUCCESS;

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

  m_Position.Set(pScreen->m_iOffsetX, pScreen->m_iOffsetY);

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

  return EZ_SUCCESS;
}

void ezWindowCreationDesc::SaveToDDL(ezOpenDdlWriter& writer)
{
  writer.BeginObject("WindowDesc");

  ezOpenDdlUtils::StoreString(writer, m_Title, "Title");

  switch (m_WindowMode.GetValue())
  {
    case ezWindowMode::FullscreenBorderlessNativeResolution:
      ezOpenDdlUtils::StoreString(writer, "Borderless", "Mode");
      break;
    case ezWindowMode::FullscreenFixedResolution:
      ezOpenDdlUtils::StoreString(writer, "Fullscreen", "Mode");
      break;
    case ezWindowMode::WindowFixedResolution:
      ezOpenDdlUtils::StoreString(writer, "Window", "Mode");
      break;
    case ezWindowMode::WindowResizable:
      ezOpenDdlUtils::StoreString(writer, "ResizableWindow", "Mode");
      break;
  }

  if (m_uiWindowNumber != 0)
    ezOpenDdlUtils::StoreUInt8(writer, m_uiWindowNumber, "Index");

  if (m_iMonitor >= 0)
    ezOpenDdlUtils::StoreInt8(writer, m_iMonitor, "Monitor");

  if (m_Position != ezVec2I32(0x80000000, 0x80000000))
  {
    ezOpenDdlUtils::StoreVec2I(writer, m_Position, "Position");
  }

  ezOpenDdlUtils::StoreVec2U(writer, ezVec2U32(m_Resolution.width, m_Resolution.height), "Resolution");

  ezOpenDdlUtils::StoreBool(writer, m_bClipMouseCursor, "ClipMouseCursor");
  ezOpenDdlUtils::StoreBool(writer, m_bShowMouseCursor, "ShowMouseCursor");

  writer.EndObject();
}


ezResult ezWindowCreationDesc::SaveToDDL(const char* szFile)
{
  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

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
      ezOpenDdlUtils::ConvertToVec2I(pPosition, m_Position);
    }

    if (const ezOpenDdlReaderElement* pPosition = pDesc->FindChild("Resolution"))
    {
      ezVec2U32 res;
      ezOpenDdlUtils::ConvertToVec2U(pPosition, res);
      m_Resolution.width = res.x;
      m_Resolution.height = res.y;
    }

    if (const ezOpenDdlReaderElement* pClipMouseCursor = pDesc->FindChildOfType(ezOpenDdlPrimitiveType::Bool, "ClipMouseCursor"))
      m_bClipMouseCursor = pClipMouseCursor->GetPrimitivesBool()[0];

    if (const ezOpenDdlReaderElement* pShowMouseCursor = pDesc->FindChildOfType(ezOpenDdlPrimitiveType::Bool, "ShowMouseCursor"))
      m_bShowMouseCursor = pShowMouseCursor->GetPrimitivesBool()[0];
  }
}


ezResult ezWindowCreationDesc::LoadFromDDL(const char* szFile)
{
  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  ezOpenDdlReader reader;
  EZ_SUCCEED_OR_RETURN(reader.ParseDocument(file));

  LoadFromDDL(reader.GetRootElement());

  return EZ_SUCCESS;
}

ezWindow::ezWindow() {}

ezWindow::~ezWindow()
{
  if (m_bInitialized)
    Destroy();
}

EZ_STATICLINK_FILE(System, System_Window_Implementation_Window);
