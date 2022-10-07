
#ifdef EZ_GCC_WARNING_NAME

#  if EZ_ENABLED(EZ_COMPILER_GCC)

#    pragma GCC diagnostic push
_Pragma(EZ_STRINGIZE(GCC diagnostic ignored EZ_GCC_WARNING_NAME))
_Pragma(EZ_STRINGIZE(message EZ_GCC_WARNING_NAME))

#  endif

#  undef EZ_GCC_WARNING_NAME

#endif
