
#ifdef EZ_GCC_WARNING_NAME

#  if EZ_ENABLED(EZ_COMPILER_GCC)

#    define GCC_DISABLE_WARNING(W) _Pragma GCC diagnostic ignored W

#    pragma GCC diagnostic push
GCC_DISABLE_WARNING(EZ_GCC_WARNING_NAME)

#    undef GCC_DISABLE_WARNING

#  endif

#  undef EZ_GCC_WARNING_NAME

#endif
