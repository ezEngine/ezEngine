#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>

#ifdef EZ_USE_DXVK
// These are shims to make DXVK compile our RendererDX11 library

#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
#    define INFINITE 0xFFFFFFFF
#  endif

#  include <Core/System/Window.h>
#  include <Foundation/Platform/Win/Utils/MinWindows.h>
#  include <windows_base.h>
namespace ezMinWindows
{
  template <>
  struct ToNativeImpl<ezWindowHandle>
  {
    using type = ::HWND;
    static EZ_ALWAYS_INLINE ::HWND ToNative(ezWindowHandle hWnd)
    {
      EZ_ASSERT_DEV(hWnd.type == ezWindowHandle::Type::GLFW, "Only GLFW is supported on DXVK");
      return reinterpret_cast<::HWND>(hWnd.glfwWindow);
    }
  };
} // namespace ezMinWindows
#endif
