#include <FoundationTest/FoundationTestPCH.h>

// This test emits various trace events (including cross-thread) to exercise the tracing code path. Use an external tool to capture and verify events (see Utilities/Tracing/Capture-Trace.ps1).

#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/UniquePtr.h>
#include <FoundationTest/Tracing/TraceProvider.h>

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

      // BEGIN-DOCS-CODE-SNIPPET: tracing-async-activity-end
      // Complete the async activity that was started on the main thread.
      EZ_TRACE_ASYNC_END("CrossThreadActivity", m_uiAsyncId);
      // END-DOCS-CODE-SNIPPET
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
    // BEGIN-DOCS-CODE-SNIPPET: tracing-instant-event
    // Instant event demonstrating all supported value types.
    const void* pDemoPtr = nullptr;
    EZ_TRACE_EVENT("TestInstantEvent", ezTraceLevel::Info,
      EZ_TRACE_VALUE("BoolField", true),
      EZ_TRACE_VALUE("Int8Field", (ezInt8)-1),
      EZ_TRACE_VALUE("Int16Field", (ezInt16)-16),
      EZ_TRACE_VALUE("Int32Field", (ezInt32)42),
      EZ_TRACE_VALUE("Int64Field", (ezInt64)1234567890LL),
      EZ_TRACE_VALUE("UInt8Field", (ezUInt8)255),
      EZ_TRACE_VALUE("UInt16Field", (ezUInt16)65535),
      EZ_TRACE_VALUE("UInt32Field", (ezUInt32)100),
      EZ_TRACE_VALUE("UInt64Field", (ezUInt64)9876543210ULL),
      EZ_TRACE_VALUE("FloatField", 3.14f),
      EZ_TRACE_VALUE("DoubleField", 2.71828),
      EZ_TRACE_VALUE("StringField", "hello"),
      EZ_TRACE_VALUE("PointerField", pDemoPtr));
    // END-DOCS-CODE-SNIPPET

    // BEGIN-DOCS-CODE-SNIPPET: tracing-scoped-event
    // Scoped event (RAII begin + end).
    {
      EZ_TRACE_SCOPE("TestScopedWork", ezTraceLevel::Verbose,
        EZ_TRACE_VALUE("Detail", "scope-payload"));
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(5));
    }
    // END-DOCS-CODE-SNIPPET

    // BEGIN-DOCS-CODE-SNIPPET: tracing-manual-scope
    // Manual scope begin + end (for cases where RAII is not applicable).
    EZ_TRACE_SCOPE_BEGIN("TestManualScope", ezTraceLevel::Info,
      EZ_TRACE_VALUE("Detail", "manual-scope-payload"));
    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(5));
    EZ_TRACE_SCOPE_END("TestManualScope");
    // END-DOCS-CODE-SNIPPET

    // BEGIN-DOCS-CODE-SNIPPET: tracing-async-activity-start
    // Async activity that spans across threads.
    const ezUInt64 uiAsyncId = 123456789ULL;
    EZ_TRACE_ASYNC_BEGIN("CrossThreadActivity", uiAsyncId, ezTraceLevel::Info,
      EZ_TRACE_VALUE("Resource", "test-resource.dat"));
    // END-DOCS-CODE-SNIPPET

    // Worker thread: emits its own events and completes the async activity.
    ezUniquePtr<TracingTestThread> pThread = EZ_DEFAULT_NEW(TracingTestThread, uiAsyncId);
    pThread->Start();
    pThread->Join();


    // BEGIN-DOCS-CODE-SNIPPET: tracing-flush
    // Flush buffered events to the tracing backend.
    EZ_TRACE_FLUSH();
    // END-DOCS-CODE-SNIPPET
  }
}
