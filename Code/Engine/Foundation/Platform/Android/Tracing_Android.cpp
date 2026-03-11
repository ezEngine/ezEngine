#include <Foundation/FoundationPCH.h>

#if defined(BUILDSYSTEM_ENABLE_PERFETTO_SUPPORT)

#  include <Foundation/Platform/Android/Tracing_Platform.h>

#  include <Foundation/Threading/ThreadUtils.h>
#  include <Foundation/Time/Time.h>

#  include <android/log.h>

#  define EZ_PERFETTO_LOG(...) __android_log_print(ANDROID_LOG_INFO, "ezEngine", __VA_ARGS__)

// Allocate storage for the Perfetto track event categories defined in the header.
PERFETTO_TRACK_EVENT_STATIC_STORAGE();

static bool s_bPerfettoInitialized = false;

void ezPerfettoRegistration::EnsureInitialized()
{
  if (s_bPerfettoInitialized)
    return;

  s_bPerfettoInitialized = true;

  perfetto::TracingInitArgs args;

  // Use system backend to merge with OS-level ftrace, scheduling, and other system traces. On Android 9+, the traced daemon is built into the platform.
  args.backends |= perfetto::kSystemBackend;
  // Also enable in-process backend for standalone trace capture without the system daemon.
  args.backends |= perfetto::kInProcessBackend;

  perfetto::Tracing::Initialize(args);
  perfetto::TrackEvent::Register();
  // The connection to the traced daemon is asynchronous. If an external trace session is already running (e.g. started via `perfetto` CLI), the data source needs a moment to be activated. Without this wait, the first events are silently dropped because the data source is not yet enabled.
  constexpr int iMaxWaitMs = 1000;
  constexpr int iPollMs = 100;
  int iElapsed = 0;
  while (!perfetto::TrackEvent::IsEnabled() && iElapsed < iMaxWaitMs)
  {
    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(iPollMs));
    iElapsed += iPollMs;
  }

  if (perfetto::TrackEvent::IsEnabled())
  {
    EZ_PERFETTO_LOG("track_event data source ACTIVE after %dms.", iElapsed);
  }
  else
  {
    EZ_PERFETTO_LOG("track_event data source NOT active after %dms. Events will be emitted but may not appear.", iMaxWaitMs);
  }
}

void ezPerfettoRegistration::Flush()
{
  if (s_bPerfettoInitialized)
  {
    perfetto::TrackEvent::Flush();
  }
}

#endif
