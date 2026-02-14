#pragma once

#include <Foundation/Types/ScopeExit.h>

/// \file
/// \brief Cross-platform tracing system for emitting structured events to the OS tracing infrastructure.
///
/// This header provides macros to emit instant events, scoped events, and async activities that are captured by ETW (Windows), LTTNG (Linux), or Perfetto (Android). Unlike EZ_PROFILE_SCOPE, which stores data in an in-process ring buffer for later JSON export, these macros send events to the operating system's tracing subsystem for capture by external tools.
///
/// Tracing is controlled by the EZ_USE_TRACING feature toggle (see UserConfig.h). When disabled, all macros expand to nothing with zero overhead.
///
/// \section tracing_usage Emitting Trace Events
///
/// \code{.cpp}
///   // Instant event (point-in-time)
///   EZ_TRACE_EVENT("TextureLoaded", ezTraceLevel::Info,
///     EZ_TRACE_VALUE("Path", szTexturePath),
///     EZ_TRACE_VALUE("Width", uiWidth));
///
///   // Scoped event (RAII begin + end, measures duration)
///   {
///     EZ_TRACE_SCOPE("PhysicsStep", ezTraceLevel::Verbose,
///       EZ_TRACE_VALUE("NumBodies", uiBodyCount));
///     // ... physics work ...
///   } // end event emitted automatically
///
///   // Async activity (correlated across threads by ID)
///   EZ_TRACE_ASYNC_BEGIN("AsyncLoad", uiRequestId, ezTraceLevel::Info,
///     EZ_TRACE_VALUE("Resource", szPath));
///   // ... on another thread or later ...
///   EZ_TRACE_ASYNC_END("AsyncLoad", uiRequestId);
/// \endcode
///
/// \section tracing_provider Provider Setup
///
/// Each library or executable that emits trace events needs its own provider. Create two files in a Tracing/ subfolder:
///
/// \code{.cpp}
///   // MyLibrary/Tracing/TraceProvider.h
///   #pragma once
///   #include <Foundation/Tracing/Tracing.h>
///   EZ_DECLARE_TRACE_PROVIDER(g_ezTrace_MyLibrary);
///   #define EZ_TRACE_PROVIDER g_ezTrace_MyLibrary
/// \endcode
///
/// \code{.cpp}
///   // MyLibrary/Tracing/TraceProvider.cpp
///   #include <MyLibrary/MyLibraryPCH.h>
///   #include <MyLibrary/Tracing/TraceProvider.h>
///   EZ_IMPLEMENT_TRACE_PROVIDER(g_ezTrace_MyLibrary, "ez_MyLibrary");
/// \endcode
///
/// Source files that emit trace events include the provider header instead of Tracing.h:
/// \code{.cpp}
///   #include <MyLibrary/Tracing/TraceProvider.h>
///   void MyFunction()
///   {
///     EZ_TRACE_EVENT("MyEvent", ezTraceLevel::Info, EZ_TRACE_VALUE("Key", 42));
///   }
/// \endcode
///
/// \section tracing_capture Capturing Traces
///
/// Use the cross-platform PowerShell 7 script at Utilities/Tracing/Capture-Trace.ps1. See that script for full documentation on per-platform setup, prerequisites, and manual steps.
///
/// \code{.sh}
///   # Interactive capture: Starts a trace and waits for a key press to stop the trace.
///   pwsh Utilities/Tracing/Capture-Trace.ps1
///
///   # Non-interactive:
///   pwsh Utilities/Tracing/Capture-Trace.ps1 -Start
///   # ... run application ...
///   pwsh Utilities/Tracing/Capture-Trace.ps1 -Stop -OutputPath trace
///
///   # Android (Perfetto) from any host:
///   pwsh Utilities/Tracing/Capture-Trace.ps1 -Android
/// \endcode
///
/// \sa EZ_TRACE_EVENT, EZ_TRACE_SCOPE, EZ_TRACE_ASYNC_BEGIN, EZ_TRACE_ASYNC_END, EZ_TRACE_VALUE

#include <Foundation/Basics.h>

/// \brief Trace event severity levels.
///
/// These are ezEngine's own levels, mapped to ETW / LTTNG / Perfetto by the platform backend.
struct ezTraceLevel
{
  enum Enum : ezUInt8
  {
    Error,   ///< Serious failure.
    Warning, ///< Potential problem.
    Info,    ///< Informational.
    Verbose, ///< Detailed diagnostic.
  };
};

#if EZ_ENABLED(EZ_USE_TRACING)
#  include <Tracing_Platform.h>
#endif

// Ensure provider macros are always defined (no-op when tracing is off or unsupported).
#ifndef EZ_DECLARE_TRACE_PROVIDER
#  define EZ_DECLARE_TRACE_PROVIDER(ProviderSymbol)
#endif

#ifndef EZ_IMPLEMENT_TRACE_PROVIDER
#  define EZ_IMPLEMENT_TRACE_PROVIDER(ProviderSymbol, ProviderName)
#endif

#if EZ_ENABLED(EZ_USE_TRACING) || defined(EZ_DOCS)

/// \brief Field descriptor macro for use inside EZ_TRACE_EVENT, EZ_TRACE_SCOPE, and EZ_TRACE_ASYNC_BEGIN. Auto-detects the C++ type of Value. Cast the value if the auto-detection picks the wrong type.
#  define EZ_TRACE_VALUE(FieldName, Value) \
    EZ_TRACE_INTERNAL_VALUE(FieldName, Value)

/// \brief Fires a single instant event. The Level parameter is an ezTraceLevel::Enum.
///
/// Usage:
/// \code
///   EZ_TRACE_EVENT("CollisionDetected", ezTraceLevel::Info,
///     EZ_TRACE_VALUE("NumContacts", iContacts),
///     EZ_TRACE_VALUE("Force", fImpact));
/// \endcode
///
/// \sa EZ_TRACE_SCOPE
#  define EZ_TRACE_EVENT(EventName, Level, ...) \
    EZ_TRACE_INTERNAL_EVENT(EventName, Level, ##__VA_ARGS__)

/// \brief Opens a scoped trace that records begin time at construction and end time at destruction.
///
/// Fields are attached to the begin event. The Level parameter is an ezTraceLevel::Enum.
///
/// Usage:
/// \code
///   {
///     EZ_TRACE_SCOPE("DrawCalls", ezTraceLevel::Info,
///       EZ_TRACE_VALUE("BatchCount", uiBatches));
///     // ... work ...
///   }
/// \endcode
///
/// \sa EZ_TRACE_EVENT, EZ_TRACE_ASYNC_BEGIN, EZ_TRACE_SCOPE_BEGIN
#  define EZ_TRACE_SCOPE(EventName, Level, ...)                     \
    EZ_TRACE_INTERNAL_SCOPE_BEGIN(EventName, Level, ##__VA_ARGS__); \
    EZ_SCOPE_EXIT(EZ_TRACE_INTERNAL_SCOPE_END(EventName));

/// \brief Manually begins a scoped trace event.
///
/// Unlike EZ_TRACE_SCOPE, this does not automatically emit an end event at scope exit. You must call EZ_TRACE_SCOPE_END with the same EventName when the region is complete. Use this when the begin and end do not fall within the same C++ scope (e.g. across callbacks).
///
/// \sa EZ_TRACE_SCOPE_END, EZ_TRACE_SCOPE
#  define EZ_TRACE_SCOPE_BEGIN(EventName, Level, ...) \
    EZ_TRACE_INTERNAL_SCOPE_BEGIN(EventName, Level, ##__VA_ARGS__)

/// \brief Ends a scoped trace event previously started with EZ_TRACE_SCOPE_BEGIN.
///
/// Must use the same EventName that was passed to EZ_TRACE_SCOPE_BEGIN.
///
/// \sa EZ_TRACE_SCOPE_BEGIN
#  define EZ_TRACE_SCOPE_END(EventName) \
    EZ_TRACE_INTERNAL_SCOPE_END(EventName)

/// \brief Begin a named async activity tied to an ID (ezUInt64). The ID correlates begin/end across threads.
///
/// Usage:
/// \code
///   EZ_TRACE_ASYNC_BEGIN("AsyncLoad", uiRequestId, ezTraceLevel::Info,
///     EZ_TRACE_VALUE("Resource", szPath));
///   // ... on another thread or later ...
///   EZ_TRACE_ASYNC_END("AsyncLoad", uiRequestId);
/// \endcode
///
/// \sa EZ_TRACE_ASYNC_END
#  define EZ_TRACE_ASYNC_BEGIN(EventName, Id, Level, ...) \
    EZ_TRACE_INTERNAL_ASYNC_BEGIN(EventName, Id, Level, ##__VA_ARGS__)

/// \brief End the async activity started with EZ_TRACE_ASYNC_BEGIN. Must use the same EventName and Id.
///
/// \sa EZ_TRACE_ASYNC_BEGIN
#  define EZ_TRACE_ASYNC_END(EventName, Id) \
    EZ_TRACE_INTERNAL_ASYNC_END(EventName, Id)

/// \brief Flushes any buffered trace events to the OS tracing backend.
///
/// Useful before stopping a trace session to ensure all emitted events are captured.
#  ifdef EZ_TRACE_INTERNAL_FLUSH
#    define EZ_TRACE_FLUSH() EZ_TRACE_INTERNAL_FLUSH()
#  else
#    define EZ_TRACE_FLUSH()
#  endif

#else

#  define EZ_TRACE_VALUE(FieldName, Value)
#  define EZ_TRACE_EVENT(EventName, Level, ...)
#  define EZ_TRACE_SCOPE(EventName, Level, ...)
#  define EZ_TRACE_SCOPE_BEGIN(EventName, Level, ...)
#  define EZ_TRACE_SCOPE_END(EventName)
#  define EZ_TRACE_ASYNC_BEGIN(EventName, Id, Level, ...)
#  define EZ_TRACE_ASYNC_END(EventName, Id)
#  define EZ_TRACE_FLUSH()

#endif
