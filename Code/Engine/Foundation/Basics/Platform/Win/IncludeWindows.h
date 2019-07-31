#pragma once

#include <Foundation/Basics.h>

#define EZ_INCLUDED_WINDOWS_H 1

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
// this is important for code that wants to include winsock2.h later on
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */

// already includes Windows.h, but defines important other things first
//#include <winsock2.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

// unset windows macros
#undef min
#undef max
#undef GetObject
#undef ERROR
#undef DeleteFile
#undef CopyFile
#undef DispatchMessage
#undef PostMessage
#undef SendMessage
#undef OPAQUE
#undef SetPort

#include <Foundation/Basics/Platform/Win/MinWindows.h>

namespace ezMinWindows
{
  template <>
  struct ToNativeImpl<HINSTANCE>
  {
    typedef ::HINSTANCE type;
    static EZ_ALWAYS_INLINE ::HINSTANCE ToNative(HINSTANCE hInstance)
    {
      return reinterpret_cast<::HINSTANCE>(hInstance);
    }
  };

  template <>
  struct ToNativeImpl<HWND>
  {
    typedef ::HWND type;
    static EZ_ALWAYS_INLINE ::HWND ToNative(HWND hWnd)
    {
      return reinterpret_cast<::HWND>(hWnd);
    }
  };

  template <>
  struct FromNativeImpl<::HWND>
  {
    typedef HWND type;
    static EZ_ALWAYS_INLINE HWND FromNative(::HWND hWnd)
    {
      return reinterpret_cast<HWND>(hWnd);
    }
  };

  template <>
  struct FromNativeImpl<::HINSTANCE>
  {
    typedef HINSTANCE type;
    static EZ_ALWAYS_INLINE HINSTANCE FromNative(::HINSTANCE hInstance)
    {
      return reinterpret_cast<HINSTANCE>(hInstance);
    }
  };
}
#endif
