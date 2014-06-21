#pragma once

#ifndef NULL
  #error "NULL is not defined."
#endif

#ifndef EZ_FORCE_INLINE
  #error "EZ_FORCE_INLINE is not defined."
#endif

#ifndef EZ_RESTRICT
  #error "EZ_RESTRICT is not defined."
#endif

#ifndef EZ_ANALYSIS_ASSUME
  #error "EZ_ANALYSIS_ASSUME is not defined."
#endif

#ifndef EZ_ANALYSIS_IGNORE_WARNING_ONCE
  #error "EZ_ANALYSIS_IGNORE_WARNING_ONCE is not defined."
#endif

#ifndef EZ_ANALYSIS_IGNORE_WARNING_START
  #error "EZ_ANALYSIS_IGNORE_WARNING_START is not defined."
#endif

#ifndef EZ_ANALYSIS_IGNORE_WARNING_END
  #error "EZ_ANALYSIS_IGNORE_WARNING_END is not defined."
#endif

#ifndef EZ_ANALYSIS_IGNORE_ALL_START
  #error "EZ_ANALYSIS_IGNORE_ALL_START is not defined."
#endif

#ifndef EZ_ANALYSIS_IGNORE_ALL_END
  #error "EZ_ANALYSIS_IGNORE_ALL_END is not defined."
#endif

#ifndef EZ_ALIGN
  #error "EZ_ALIGN is not defined."
#endif

#ifndef EZ_ALIGNMENT_OF
  #error "EZ_ALIGNMENT_OF is not defined."
#endif

#if EZ_IS_NOT_EXCLUSIVE(EZ_PLATFORM_32BIT, EZ_PLATFORM_64BIT)
  #error "Platform is not defined as 32 Bit or 64 Bit"
#endif

#ifndef EZ_DEBUG_BREAK
  #error "EZ_DEBUG_BREAK is not defined."
#endif

#ifndef EZ_SOURCE_FUNCTION
  #error "EZ_SOURCE_FUNCTION is not defined."
#endif

#ifndef EZ_SOURCE_FILE
  #error "EZ_SOURCE_FILE is not defined."
#endif

#ifndef EZ_SOURCE_LINE
  #error "EZ_SOURCE_LINE is not defined."
#endif

#if EZ_IS_NOT_EXCLUSIVE(EZ_PLATFORM_LITTLE_ENDIAN, EZ_PLATFORM_BIG_ENDIAN)
  #error "Endianess is not correctly defined!"
#endif

#ifndef EZ_MATH_CHECK_FOR_NAN
  #error "EZ_MATH_CHECK_FOR_NAN is not defined."
#endif

