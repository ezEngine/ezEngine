#include <Foundation/PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/Thread.h>

class ezTelemetryThread : public ezThread
{
public:
  ezTelemetryThread() : ezThread("ezTelemetryThread")
  {
    m_bKeepRunning = true;
  }

  volatile bool m_bKeepRunning;

private:
  virtual ezUInt32 Run()
  {
    static ezTime LastPing;

    while (m_bKeepRunning)
    {
      ezTelemetry::UpdateNetwork();

      // Send a Ping every once in a while
      if (ezTelemetry::s_ConnectionMode == ezTelemetry::Client)
      {
        ezTime tNow = ezTime::Now();

        if (tNow - LastPing > ezTime::Milliseconds(500))
        {
          LastPing = tNow;

          ezTelemetry::UpdateServerPing();
        }
      }

      ezThreadUtils::Sleep(10);
    }

    return 0;
  }
};

static ezTelemetryThread* g_pBroadcastThread = nullptr;
ezMutex ezTelemetry::s_TelemetryMutex;


ezMutex& ezTelemetry::GetTelemetryMutex()
{
  return s_TelemetryMutex;
}

void ezTelemetry::StartTelemetryThread()
{
  if (!g_pBroadcastThread)
  {
    g_pBroadcastThread = EZ_DEFAULT_NEW(ezTelemetryThread);
    g_pBroadcastThread->Start();
  }
}

void ezTelemetry::StopTelemetryThread()
{
  if (g_pBroadcastThread)
  {
    g_pBroadcastThread->m_bKeepRunning = false;
    g_pBroadcastThread->Join();

    EZ_DEFAULT_DELETE(g_pBroadcastThread);
  }
}




EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_TelemetryThread);

