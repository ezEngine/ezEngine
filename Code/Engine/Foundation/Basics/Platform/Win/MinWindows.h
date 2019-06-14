#pragma once

namespace ezMinWindows
{
  typedef int BOOL;
  typedef unsigned long DWORD;
  typedef unsigned int UINT;
  typedef char* LPSTR;
  struct ezHINSTANCE;
  typedef ezHINSTANCE* HINSTANCE;
  typedef HINSTANCE HMODULE;
  struct ezHWND;
  typedef ezHWND* HWND;
  typedef long HRESULT;
  typedef void* HANDLE;

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  typedef ezUInt64 WPARAM;
  typedef ezUInt64 LPARAM;
#else
  typedef ezUInt32 WPARAM;
  typedef ezUInt32 LPARAM;
#endif

  template <typename T>
  struct ToNativeImpl
  {
  };

  template <typename T>
  struct FromNativeImpl
  {
  };

  /// Helper function to convert ezMinWindows types into native windows.h types.
  /// Include IncludeWindows.h before using it.
  template <typename T>
  EZ_ALWAYS_INLINE typename ToNativeImpl<T>::type ToNative(T t)
  {
    return ToNativeImpl<T>::ToNative(t);
  }

  /// Helper function to native windows.h types to ezMinWindows types.
  /// Include IncludeWindows.h before using it.
  template <typename T>
  EZ_ALWAYS_INLINE typename FromNativeImpl<T>::type FromNative(T t)
  {
    return FromNativeImpl<T>::FromNative(t);
  }
}
#define EZ_WINDOWS_CALLBACK __stdcall
#define EZ_WINDOWS_WINAPI __stdcall
#define EZ_WINDOWS_INVALID_HANDLE_VALUE (void*)-1
