#include <FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Basics/Platform/Win/MinWindows.h>
#include <type_traits>

template <typename ezType, typename WindowsType, bool mustBeConvertible>
void ezVerifyWindowsType()
{
  static_assert(sizeof(ezType) == sizeof(WindowsType), "ez <=> windows.h size mismatch");
  static_assert(alignof(ezType) == alignof(WindowsType), "ez <=> windows.h alignment mismatch");
  static_assert(std::is_pointer<ezType>::value == std::is_pointer<WindowsType>::value, "ez <=> windows.h pointer type mismatch");
  static_assert(!mustBeConvertible || ezConversionTest<ezType, WindowsType>::exists == 1, "ez <=> windows.h conversion failure");
  static_assert(!mustBeConvertible || ezConversionTest<WindowsType, ezType>::exists == 1, "windows.h <=> ez conversion failure");
};

void CALLBACK WindowsCallbackTest1();
void EZ_WINDOWS_CALLBACK WindowsCallbackTest2();
void WINAPI WindowsWinapiTest1();
void EZ_WINDOWS_WINAPI WindowsWinapiTest2();

// Will never be called and thus removed by the linker
void ezCheckWindowsTypeSizes()
{
  ezVerifyWindowsType<ezMinWindows::DWORD, DWORD, true>();
  ezVerifyWindowsType<ezMinWindows::UINT, UINT, true>();
  ezVerifyWindowsType<ezMinWindows::BOOL, BOOL, true>();
  ezVerifyWindowsType<ezMinWindows::LPARAM, LPARAM, true>();
  ezVerifyWindowsType<ezMinWindows::WPARAM, WPARAM, true>();
  ezVerifyWindowsType<ezMinWindows::HINSTANCE, HINSTANCE, false>();
  ezVerifyWindowsType<ezMinWindows::HMODULE, HMODULE, false>();
  ezVerifyWindowsType<ezMinWindows::LPSTR, LPSTR, true>();
  ezVerifyWindowsType<ezMinWindows::HWND, HWND, false>();
  ezVerifyWindowsType<ezMinWindows::HRESULT, HRESULT, true>();

  static_assert(
    std::is_same<decltype(&WindowsCallbackTest1), decltype(&WindowsCallbackTest2)>::value, "EZ_WINDOWS_CALLBACK does not match CALLBACK");
  static_assert(
    std::is_same<decltype(&WindowsWinapiTest1), decltype(&WindowsWinapiTest2)>::value, "EZ_WINDOWS_WINAPI does not match WINAPI");

  static_assert(
    EZ_WINDOWS_INVALID_HANDLE_VALUE == INVALID_HANDLE_VALUE, "EZ_WINDOWS_INVALID_HANDLE_VALUE does not match INVALID_HANDLE_VALUE");
}
#endif
