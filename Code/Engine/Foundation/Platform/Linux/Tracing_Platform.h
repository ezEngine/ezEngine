#pragma once

#if defined(BUILDSYSTEM_ENABLE_TRACELOGGING_LTTNG_SUPPORT)

#  include <tracelogging/TraceLoggingProvider.h>

// The tracelogging-lttng wrapper derives the LTTNG event ID from the event name. Each unique ID must map to exactly one schema (field layout). If two TraceLoggingWrite calls share the same name but different fields, the second one corrupts the metadata stream and babeltrace2 cannot decode events from other threads.
// Scope and async events naturally have different schemas for begin vs end (begin carries user payload fields, end does not). We disambiguate them by appending ":Begin" / ":End" to the event name via string-literal concatenation.

#  define EZ_DECLARE_TRACE_PROVIDER(ProviderSymbol) \
    TRACELOGGING_DECLARE_PROVIDER(ProviderSymbol);  \
    extern bool EZ_PP_CONCAT(g_bEzTraceRegistered_, ProviderSymbol)

/// Tracing provider GUID (shared across all platforms).
// {BFD4350A-BA77-463D-B4BE-E30374E42494}
#  define EZ_TRACE_PROVIDER_GUID \
    (0xbfd4350a, 0xba77, 0x463d, 0xb4, 0xbe, 0xe3, 0x3, 0x74, 0xe4, 0x24, 0x94)

#  define EZ_IMPLEMENT_TRACE_PROVIDER(ProviderSymbol, ProviderName)                     \
    TRACELOGGING_DEFINE_PROVIDER(ProviderSymbol, ProviderName, EZ_TRACE_PROVIDER_GUID); \
    bool EZ_PP_CONCAT(g_bEzTraceRegistered_, ProviderSymbol) = false;                   \
    static struct EZ_PP_CONCAT(ezTraceCleanup_, ProviderSymbol)                         \
    {                                                                                   \
      EZ_PP_CONCAT(ezTraceCleanup_, ProviderSymbol)                                     \
      ()                                                                                \
      {                                                                                 \
        EZ_PP_CONCAT(g_bEzTraceRegistered_, ProviderSymbol) = true;                     \
        TraceLoggingRegister(ProviderSymbol);                                           \
      }                                                                                 \
      ~EZ_PP_CONCAT(ezTraceCleanup_, ProviderSymbol)()                                  \
      {                                                                                 \
        TraceLoggingUnregister(ProviderSymbol);                                         \
      }                                                                                 \
    } EZ_PP_CONCAT(s_ezTraceCleanup_, ProviderSymbol)

#  define EZ_TRACE_INTERNAL_VALUE(FieldName, Value) TraceLoggingValue((Value), FieldName)

#  define EZ_TRACE_LEVEL_TO_PLATFORM_(Level)                                                                           \
    ((Level) == ezTraceLevel::Error ? WINEVENT_LEVEL_ERROR : (Level) == ezTraceLevel::Warning ? WINEVENT_LEVEL_WARNING \
                                                           : (Level) == ezTraceLevel::Info    ? WINEVENT_LEVEL_INFO    \
                                                                                              : WINEVENT_LEVEL_VERBOSE)

#  define EZ_TRACE_ENSURE_REGISTERED_()                              \
    if (!EZ_PP_CONCAT(g_bEzTraceRegistered_, EZ_TRACE_PROVIDER))     \
    {                                                                \
      EZ_PP_CONCAT(g_bEzTraceRegistered_, EZ_TRACE_PROVIDER) = true; \
      TraceLoggingRegister(EZ_TRACE_PROVIDER);                       \
    }

#  define EZ_TRACE_INTERNAL_EVENT(EventName, Level, ...)                                                                     \
    do                                                                                                                       \
    {                                                                                                                        \
      EZ_TRACE_ENSURE_REGISTERED_();                                                                                         \
      TraceLoggingWrite(EZ_TRACE_PROVIDER, EventName, TraceLoggingLevel(EZ_TRACE_LEVEL_TO_PLATFORM_(Level)), ##__VA_ARGS__); \
    } while (false)

#  define EZ_TRACE_INTERNAL_SCOPE_BEGIN(EventName, Level, ...)                                                                                                                   \
    do                                                                                                                                                                           \
    {                                                                                                                                                                            \
      EZ_TRACE_ENSURE_REGISTERED_();                                                                                                                                             \
      TraceLoggingWrite(EZ_TRACE_PROVIDER, EventName ":Begin", TraceLoggingLevel(EZ_TRACE_LEVEL_TO_PLATFORM_(Level)), TraceLoggingOpcode(WINEVENT_OPCODE_START), ##__VA_ARGS__); \
    } while (false)

#  define EZ_TRACE_INTERNAL_SCOPE_END(EventName) \
    TraceLoggingWrite(EZ_TRACE_PROVIDER, EventName ":End", TraceLoggingOpcode(WINEVENT_OPCODE_STOP))

#  define EZ_TRACE_INTERNAL_ASYNC_BEGIN(EventName, Id, Level, ...) \
    do                                                             \
    {                                                              \
      EZ_TRACE_ENSURE_REGISTERED_();                               \
      TraceLoggingWrite(EZ_TRACE_PROVIDER, EventName ":Begin",     \
        TraceLoggingLevel(EZ_TRACE_LEVEL_TO_PLATFORM_(Level)),     \
        TraceLoggingOpcode(WINEVENT_OPCODE_START),                 \
        TraceLoggingUInt64((Id), "CorrelationId"), ##__VA_ARGS__); \
    } while (false)

#  define EZ_TRACE_INTERNAL_ASYNC_END(EventName, Id)         \
    do                                                       \
    {                                                        \
      TraceLoggingWrite(EZ_TRACE_PROVIDER, EventName ":End", \
        TraceLoggingOpcode(WINEVENT_OPCODE_STOP),            \
        TraceLoggingUInt64((Id), "CorrelationId"));          \
    } while (false)

#else

// No LTTNG support — all tracing macros become no-ops.
#  include <Foundation/Platform/NoImpl/Tracing_NoImpl.h>

#endif
