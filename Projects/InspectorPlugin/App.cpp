#include <PCH.h>

void Inspector_AppDataRequests(void* pPassThrough)
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage('APP', Msg) == EZ_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
    case 'RQDT': // Request Data
      {
        ezTelemetryMessage Out;
        Out.SetMessageID('APP', 'DATA');

        const ezSystemInformation info = ezSystemInformation::Get();

        Out.GetWriter() << info.GetPlatformName();
        Out.GetWriter() << info.GetCPUCoreCount();
        Out.GetWriter() << info.GetInstalledMainMemory();
        Out.GetWriter() << info.Is64BitOS();

        ezTelemetry::Broadcast(ezTelemetry::Reliable, Out);
      }
      break;
    }
  }
}


