
#ifdef EZ_GCC_WARNING_NAME

#  if EZ_ENABLED(EZ_COMPILER_GCC)

#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored EZ_GCC_WARNING_NAME

#  endif

#  undef EZ_GCC_WARNING_NAME

#endif
