
// Does nothing when not on MSVC
#if EZ_ENABLED(EZ_COMPILER_MSVC) && defined(EZ_MSVC_WARNING_NUMBER)
  #include <Foundation/Basics/Compiler/MSVC/DisableWarning_MSVC.h>
#endif


#undef EZ_MSVC_WARNING_NUMBER

