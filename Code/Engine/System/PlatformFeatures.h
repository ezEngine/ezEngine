#pragma once


#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

  #define EZ_SUPPORTS_SFML EZ_OFF

#elif EZ_ENABLED(EZ_PLATFORM_OSX)

  #define EZ_SUPPORTS_SFML EZ_ON

#elif EZ_ENABLED(EZ_PLATFORM_LINUX)

  #define EZ_SUPPORTS_SFML EZ_ON

#else
  #error "Undefined platform!"
#endif


// now check that the defines for each feature are set (either to EZ_ON or EZ_OFF, but they must be defined)

#ifndef EZ_SUPPORTS_SFML
  #error "EZ_SUPPORTS_SFML is not defined."
#endif

