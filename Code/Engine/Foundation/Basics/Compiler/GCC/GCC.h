
#pragma once

/// \todo re-investigate: attribute(always inline) does not work for some reason
#define EZ_ALWAYS_INLINE inline
#define EZ_FORCE_INLINE inline

#define EZ_ALIGN(decl, alignment) __attribute__((aligned(alignment))) decl
#define EZ_ALIGNMENT_OF(type) __alignof(type)

#define EZ_DEBUG_BREAK { __builtin_trap(); }

#define EZ_SOURCE_FUNCTION __FUNCTION__
#define EZ_SOURCE_LINE __LINE__
#define EZ_SOURCE_FILE __FILE__


#define EZ_ANALYSIS_ASSUME(code_to_be_true)
#define EZ_ANALYSIS_IGNORE_WARNING_ONCE(x)
#define EZ_ANALYSIS_IGNORE_WARNING_START(x)
#define EZ_ANALYSIS_IGNORE_WARNING_END
#define EZ_ANALYSIS_IGNORE_ALL_START
#define EZ_ANALYSIS_IGNORE_ALL_END

