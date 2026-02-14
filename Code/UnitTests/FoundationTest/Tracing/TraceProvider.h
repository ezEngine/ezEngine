#pragma once

#include <Foundation/Tracing/Tracing.h>

// BEGIN-DOCS-CODE-SNIPPET: tracing-provider-header
EZ_DECLARE_TRACE_PROVIDER(g_ezTrace_FoundationTest);

/// All EZ_TRACE_* macros in FoundationTest source files use this provider.
#define EZ_TRACE_PROVIDER g_ezTrace_FoundationTest
// END-DOCS-CODE-SNIPPET
