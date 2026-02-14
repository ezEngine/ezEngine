#pragma once

#if defined(BUILDSYSTEM_ENABLE_PERFETTO_SUPPORT)

#  include <perfetto.h>

PERFETTO_DEFINE_CATEGORIES(
  perfetto::Category("ez")
    .SetDescription("ezEngine trace events"));

class EZ_FOUNDATION_DLL ezPerfettoRegistration
{
public:
  static void EnsureInitialized();
  static void Flush();
};

#  define EZ_TRACE_INTERNAL_FLUSH() ezPerfettoRegistration::Flush()

#  define EZ_TRACE_INTERNAL_VALUE(FieldName, Value) FieldName, (Value)

#  define EZ_TRACE_LEVEL_TO_STRING_(Level)                                                   \
    ((Level) == ezTraceLevel::Error ? "Error" : (Level) == ezTraceLevel::Warning ? "Warning" \
                                              : (Level) == ezTraceLevel::Info    ? "Info"    \
                                                                                 : "Verbose")

#  define EZ_TRACE_INTERNAL_EVENT(EventName, Level, ...)           \
    do                                                             \
    {                                                              \
      ezPerfettoRegistration::EnsureInitialized();                 \
      TRACE_EVENT_INSTANT("ez", EventName,                         \
        "Level", EZ_TRACE_LEVEL_TO_STRING_(Level), ##__VA_ARGS__); \
    } while (false)

#  define EZ_TRACE_INTERNAL_SCOPE_BEGIN(EventName, Level, ...)     \
    do                                                             \
    {                                                              \
      ezPerfettoRegistration::EnsureInitialized();                 \
      TRACE_EVENT_BEGIN("ez", EventName,                           \
        "Level", EZ_TRACE_LEVEL_TO_STRING_(Level), ##__VA_ARGS__); \
    } while (false)

#  define EZ_TRACE_INTERNAL_SCOPE_END(EventName) \
    TRACE_EVENT_END("ez")

#  define EZ_TRACE_INTERNAL_ASYNC_BEGIN(EventName, Id, Level, ...) \
    do                                                             \
    {                                                              \
      ezPerfettoRegistration::EnsureInitialized();                 \
      TRACE_EVENT_INSTANT("ez", EventName,                         \
        perfetto::Flow::ProcessScoped(Id),                         \
        "Level", EZ_TRACE_LEVEL_TO_STRING_(Level), ##__VA_ARGS__); \
    } while (false)

#  define EZ_TRACE_INTERNAL_ASYNC_END(EventName, Id)                                      \
    do                                                                                    \
    {                                                                                     \
      TRACE_EVENT_INSTANT("ez", EventName, perfetto::TerminatingFlow::ProcessScoped(Id)); \
    } while (false)

#else

// Perfetto support not available — redirect to no-op stubs.
#  include <Foundation/Platform/NoImpl/Tracing_NoImpl.h>

#endif
