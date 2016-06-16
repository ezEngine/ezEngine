
#include <System/PCH.h>
#include <System/Screen/Screen.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <System/Screen/Implementation/Win32/Screen_win32.inl>
#else 
  #error "ezScreen is not implemented on this platform"
#endif
