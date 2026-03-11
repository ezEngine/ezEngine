#pragma once

// Null tracing backend — all macros expand to nothing. Used when EZ_USE_TRACING is on but no platform backend is available.

#define EZ_TRACE_INTERNAL_VALUE(FieldName, Value)

#define EZ_TRACE_INTERNAL_EVENT(EventName, Level, ...) \
  do                                                   \
  {                                                    \
  } while (false)

#define EZ_TRACE_INTERNAL_SCOPE_BEGIN(EventName, Level, ...) \
  do                                                         \
  {                                                          \
  } while (false)

#define EZ_TRACE_INTERNAL_SCOPE_END(EventName) \
  do                                           \
  {                                            \
  } while (false)

#define EZ_TRACE_INTERNAL_ASYNC_BEGIN(EventName, Id, Level, ...) \
  do                                                             \
  {                                                              \
  } while (false)

#define EZ_TRACE_INTERNAL_ASYNC_END(EventName, Id) \
  do                                               \
  {                                                \
  } while (false)

#ifndef EZ_DECLARE_TRACE_PROVIDER
#  define EZ_DECLARE_TRACE_PROVIDER(ProviderSymbol)
#endif

#ifndef EZ_IMPLEMENT_TRACE_PROVIDER
#  define EZ_IMPLEMENT_TRACE_PROVIDER(ProviderSymbol, ProviderName)
#endif
