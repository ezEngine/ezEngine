#include <CoreUtils/PCH.h>
#include <CoreUtils/Debugging/DataTransfer.h>

bool ezDataTransfer::s_bInitialized = false;
ezSet<ezDataTransfer*> ezDataTransfer::s_AllTransfers;

ezDataTransferObject::ezDataTransferObject(ezDataTransfer& BelongsTo, const char* szObjectName, const char* szMimeType, const char* szFileExtension) : m_BelongsTo(BelongsTo)
{
  m_bHasBeenTransferred = false;

  m_Msg.SetMessageID('TRAN', 'DATA');
  m_Msg.GetWriter() << BelongsTo.m_sDataName;
  m_Msg.GetWriter() << szObjectName;
  m_Msg.GetWriter() << szMimeType;
  m_Msg.GetWriter() << szFileExtension;
}

ezDataTransferObject::~ezDataTransferObject()
{
  EZ_ASSERT_DEV(m_bHasBeenTransferred, "The data transfer object has never been transmitted.");
}

void ezDataTransferObject::Transmit()
{
  EZ_ASSERT_DEV(!m_bHasBeenTransferred, "The data transfer object has been transmitted already.");

  if (m_bHasBeenTransferred)
    return;

  m_bHasBeenTransferred = true;

  m_BelongsTo.Transfer(*this);
}

ezDataTransfer::ezDataTransfer()
{
  m_bTransferRequested = false;
  m_bEnabled = false;
}

ezDataTransfer::~ezDataTransfer()
{
  DisableDataTransfer();
}

void ezDataTransfer::SendStatus()
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  ezTelemetryMessage msg;
  msg.GetWriter() << m_sDataName;

  if (m_bEnabled)
  {
    msg.SetMessageID('TRAN', 'ENBL');
  }
  else
  {
    msg.SetMessageID('TRAN', 'DSBL');
  }

  ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
}

void ezDataTransfer::DisableDataTransfer()
{
  if (!m_bEnabled)
    return;

  ezDataTransfer::s_AllTransfers.Remove(this);

  m_bEnabled = false;
  SendStatus();

  m_bTransferRequested = false;
  m_sDataName.Clear();
}

void ezDataTransfer::EnableDataTransfer(const char* szDataName)
{
  if (m_bEnabled && m_sDataName == szDataName)
    return;

  DisableDataTransfer();

  Initialize();

  ezDataTransfer::s_AllTransfers.Insert(this);

  m_sDataName = szDataName;

  EZ_ASSERT_DEV(!m_sDataName.IsEmpty(), "The name for the data transfer must not be empty.");

  m_bEnabled = true;
  SendStatus();
}

void ezDataTransfer::RequestDataTransfer()
{
  if (!m_bEnabled)
  {
    m_bTransferRequested = false;
    return;
  }

  ezLog::Dev("Data Transfer Request: %s", m_sDataName.GetData());

  m_bTransferRequested = true;

  OnTransferRequest();
}

bool ezDataTransfer::IsTransferRequested(bool bReset)
{
  const bool bRes = m_bTransferRequested;

  if (bReset)
    m_bTransferRequested = false;

  return bRes;
}

void ezDataTransfer::Transfer(ezDataTransferObject& Object)
{
  if (!m_bEnabled)
    return;

  ezTelemetry::Broadcast(ezTelemetry::Reliable, Object.m_Msg);
}

void ezDataTransfer::Initialize()
{
  if (s_bInitialized)
    return;

  s_bInitialized = true;

  ezTelemetry::AddEventHandler(TelemetryEventsHandler);
  ezTelemetry::AcceptMessagesForSystem('DTRA', true, TelemetryMessage, nullptr);
}

void ezDataTransfer::TelemetryMessage(void* pPassThrough)
{
  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage('DTRA', Msg) == EZ_SUCCESS)
  {
    if (Msg.GetMessageID() == 'REQ')
    {
      ezString sName;
      Msg.GetReader() >> sName;

      for (auto it = s_AllTransfers.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Key()->m_sDataName == sName)
        {
          it.Key()->RequestDataTransfer();
          break;
        }
      }
    }
  }
}

void ezDataTransfer::TelemetryEventsHandler(const ezTelemetry::TelemetryEventData& e)
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  switch (e.m_EventType)
  {
  case ezTelemetry::TelemetryEventData::ConnectedToClient:
    SendAllDataTransfers();
    break;

  default:
    break;
  }
}

void ezDataTransfer::SendAllDataTransfers()
{
  ezTelemetryMessage msg;
  msg.SetMessageID('TRAN', 'CLR');
  ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);

  for (auto it = s_AllTransfers.GetIterator(); it.IsValid(); ++it)
  {
    it.Key()->SendStatus();
  }
}



EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Debugging_Implementation_DataTransfer);

