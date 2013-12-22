
#include <System/PCH.h>
#include <System/Window/Window.h>

#if EZ_ENABLED(EZ_SUPPORTS_SFML)
  #include <System/Window/Implementation/SFML/InputDevice_SFML.inl>
  #include <System/Window/Implementation/SFML/Window_SFML.inl>
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <System/Window/Implementation/Win32/InputDevice_win32.inl>
  #include <System/Window/Implementation/Win32/Window_win32.inl>
#else 
  #error "Missing code for ezWindow!"
#endif

ezWindow::~ezWindow()
{
  if (m_bInitialized)
    Destroy();
}

EZ_STATICLINK_FILE(System, System_Window_Implementation_Window);

