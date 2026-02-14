#include <FoundationTest/FoundationTestPCH.h>

// This test emits various trace events (including cross-thread) to exercise the tracing code path. Use an external tool to capture and verify events (see Utilities/Tracing/Capture-Trace.ps1).

#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Time.h>
#include <FoundationTest/Tracing/TraceProvider.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  class TracingTestThread : public ezThread
  {
  public:
    TracingTestThread(ezUInt64 uiAsyncId)
      : ezThread("TracingTestThread")
      , m_uiAsyncId(uiAsyncId)
    {
    }

    virtual ezUInt32 Run() override
    {
      EZ_TRACE_SCOPE("WorkerThreadScope", ezTraceLevel::Info);
      EZ_TRACE_EVENT("WorkerThreadEvent", ezTraceLevel::Info,
        EZ_TRACE_VALUE("ThreadValue", (ezInt32)99));

      // Complete the async activity that was started on the main thread.
      EZ_TRACE_ASYNC_END("CrossThreadActivity", m_uiAsyncId);
      return 0;
    }

  private:
    ezUInt64 m_uiAsyncId;
  };
} // namespace

EZ_CREATE_SIMPLE_TEST_GROUP(Tracing);

EZ_CREATE_SIMPLE_TEST(Tracing, EmitEvents)
{
  EZ_LOG_BLOCK("Tracing Test Start");
  ezLog::Info("Visible In Trace");

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Emit Events")
  {
    // Instant event with fields.
    EZ_TRACE_EVENT("TestInstantEvent", ezTraceLevel::Info,
      EZ_TRACE_VALUE("IntField", (ezInt32)42),
      EZ_TRACE_VALUE("StringField", "hello"));

    // Scoped event (RAII begin + end).
    {
      EZ_TRACE_SCOPE("TestScopedWork", ezTraceLevel::Verbose,
        EZ_TRACE_VALUE("Detail", "scope-payload"));
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(5));
    }

    // Manual scope begin + end (for cases where RAII is not applicable).
    EZ_TRACE_SCOPE_BEGIN("TestManualScope", ezTraceLevel::Info,
      EZ_TRACE_VALUE("Detail", "manual-scope-payload"));
    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(5));
    EZ_TRACE_SCOPE_END("TestManualScope");

    // Async activity that spans across threads.
    const ezUInt64 uiAsyncId = 123456789ULL;
    EZ_TRACE_ASYNC_BEGIN("CrossThreadActivity", uiAsyncId, ezTraceLevel::Info,
      EZ_TRACE_VALUE("Resource", "test-resource.dat"));

    // Worker thread: emits its own events and completes the async activity.
    ezUniquePtr<TracingTestThread> pThread = EZ_DEFAULT_NEW(TracingTestThread, uiAsyncId);
    pThread->Start();
    pThread->Join();

    // Flush buffered events to the tracing backend.
    EZ_TRACE_FLUSH();
  }
}
