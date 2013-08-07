
#include <System/PCH.h>
#include <System/Window/Window.h>


#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <System/Window/Implementation/Win32/Window_win32.inl>
#else
  #error "Missing code for ezWindow!"
#endif