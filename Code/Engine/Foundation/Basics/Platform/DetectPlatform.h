#pragma once

#if defined(_WINDOWS)
  #define EZ_PLATFORM_WINDOWS 1
//#elif defined(...)
//  #define EZ_PLATFORM_LINUX 1
#else
  #error "Unknown Platform."
#endif
