#pragma once

#include <Foundation/Tracing/Tracing.h>

EZ_DECLARE_TRACE_PROVIDER(g_ezTrace_EditorEngineProcessFramework);

/// All EZ_TRACE_* macros in EditorEngineProcessFramework source files use this provider.
#define EZ_TRACE_PROVIDER g_ezTrace_EditorEngineProcessFramework
