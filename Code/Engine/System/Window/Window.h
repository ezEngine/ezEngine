#pragma once

#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Strings/String.h>
#include <System/Basics.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

typedef HWND ezWindowHandle;

#elif EZ_ENABLED(EZ_PLATFORM_OSX)

typedef void* ezWindowHandle; // TODO

#elif EZ_ENABLED(EZ_PLATFORM_LINUX)

typedef void* ezWindowHandle; // TODO

#else
  #error "Missing Platform Code!"
#endif

struct EZ_SYSTEM_DLL ezWindowCreationDesc
{

  ezWindowCreationDesc()
    : m_ClientAreaSize(1280, 720),
      m_Title("ezWindow"),
      m_bFullscreenWindow(false)
  {
  }

  ezSizeU32 m_ClientAreaSize;

  ezHybridString<64> m_Title;

  bool m_bFullscreenWindow;

};

/// \brief A simple abstraction for platform specific window creation.
class EZ_SYSTEM_DLL ezWindow
{
  public:

    inline ezWindow(const ezWindowCreationDesc& CreationDescription)
      : m_CreationDescription(CreationDescription), m_WindowHandle(0)
    {
    }

    virtual ~ezWindow()
    {
    }

    inline const ezWindowCreationDesc& GetCreationDescription() const
    {
      return m_CreationDescription;
    }

    inline ezWindowHandle GetNativeWindowHandle() const
    {
      return m_WindowHandle;
    }

    ezResult Initialize();

    ezResult Destroy();

    // Platform specific message functions for the window class, this needs to be overridden in your subclass
    #if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
      virtual LRESULT WindowsMessageFunction(HWND hWnd, UINT Msg, WPARAM WParam, LPARAM LParam) = 0;
    #else
      #warning "Message function implementation missing for ezWindow!"
    #endif

  private:



    ezWindowCreationDesc m_CreationDescription;

    mutable ezWindowHandle m_WindowHandle;
};