
#ifdef EZ_MSVC_WARNING_NUMBER

#  if EZ_ENABLED(EZ_COMPILER_MSVC)

#    pragma warning(push)
#    pragma warning(disable \
                    : EZ_MSVC_WARNING_NUMBER)

#  endif

#  undef EZ_MSVC_WARNING_NUMBER

#endif
