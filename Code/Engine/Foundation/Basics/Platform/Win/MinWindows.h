#pragma once

namespace ezMinWindows
{
  using BOOL = int;
  using DWORD = unsigned long;
  using UINT = unsigned int;
  using LPSTR = char*;
  struct ezHINSTANCE;
  using HINSTANCE = ezHINSTANCE*;
  using HMODULE = HINSTANCE;
  struct ezHWND;
  using HWND = ezHWND*;
  using HRESULT = long;
  using HANDLE = void*;

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  using WPARAM = ezUInt64;
  using LPARAM = ezUInt64;
#else
  using WPARAM = ezUInt32;
  using LPARAM = ezUInt32;
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
} // namespace ezMinWindows
#define EZ_WINDOWS_CALLBACK __stdcall
#define EZ_WINDOWS_WINAPI __stdcall
#define EZ_WINDOWS_INVALID_HANDLE_VALUE ((void*)(long long)-1)
