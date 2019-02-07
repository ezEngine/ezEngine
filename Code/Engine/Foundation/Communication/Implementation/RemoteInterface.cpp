#include <FoundationPCH.h>

#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Utilities/ConversionUtils.h>

ezRemoteInterface::~ezRemoteInterface()
{
  // unfortunately we cannot do that ourselves here, because ShutdownConnection() calls virtual functions
  // and this object is already partially destructed here (derived class is already shut down)
  EZ_ASSERT_DEV(m_RemoteMode == ezRemoteMode::None, "ezRemoteInterface::ShutdownConnection() has to be called before destroying the interface");
}

ezResult ezRemoteInterface::CreateConnection(ezUInt32 uiConnectionToken, ezRemoteMode mode, const char* szServerAddress, bool bStartUpdateThread)
{
  ezUInt32 uiPrevID = m_uiApplicationID;
  ShutdownConnection();
  m_uiApplicationID = uiPrevID;

  EZ_LOCK(GetMutex());

  m_uiConnectionToken = uiConnectionToken;
  m_sServerAddress = szServerAddress;

  if (m_uiApplicationID == 0)
  {
    // create a 'unique' ID to identify this application
    m_uiApplicationID = (ezUInt32)ezTime::Now().GetSeconds();
  }

  if (InternalCreateConnection(mode, szServerAddress).Failed())
  {
    ShutdownConnection();
    return EZ_FAILURE;
  }

  m_RemoteMode = mode;

  UpdateRemoteInterface();

  if (bStartUpdateThread)
  {
    StartUpdateThread();
  }

  return EZ_SUCCESS;
}

ezResult ezRemoteInterface::StartServer(ezUInt32 uiConnectionToken, const char* szAddress, bool bStartUpdateThread /*= true*/)
{
  return CreateConnection(uiConnectionToken, ezRemoteMode::Server, szAddress, bStartUpdateThread);
}

ezResult ezRemoteInterface::ConnectToServer(ezUInt32 uiConnectionToken, const char* szAddress, bool bStartUpdateThread /*= true*/)
{
  return CreateConnection(uiConnectionToken, ezRemoteMode::Client, szAddress, bStartUpdateThread);
}

ezResult ezRemoteInterface::WaitForConnectionToServer(ezTime timeout /*= ezTime::Seconds(10)*/)
{
  if (m_RemoteMode != ezRemoteMode::Client)
    return EZ_FAILURE;

  const ezTime tStart = ezTime::Now();

  while (true)
  {
    UpdateRemoteInterface();

    if (IsConnectedToServer())
      return EZ_SUCCESS;

    if (timeout.GetSeconds() != 0)
    {
      if (ezTime::Now() - tStart > timeout)
        return EZ_FAILURE;
    }

    ezThreadUtils::Sleep(ezTime::Milliseconds(10));
  }
}

void ezRemoteInterface::ShutdownConnection()
{
  StopUpdateThread();

  EZ_LOCK(GetMutex());

  if (m_RemoteMode != ezRemoteMode::None)
  {
    InternalShutdownConnection();

    m_RemoteMode = ezRemoteMode::None;
    m_uiApplicationID = 0;
    m_uiConnectionToken = 0;
  }
}

void ezRemoteInterface::UpdatePingToServer()
{
  if (m_RemoteMode == ezRemoteMode::Server)
  {
    EZ_LOCK(GetMutex());
    m_PingToServer = InternalGetPingToServer();
  }
}

void ezRemoteInterface::UpdateRemoteInterface()
{
  EZ_LOCK(GetMutex());

  InternalUpdateRemoteInterface();
}

ezResult ezRemoteInterface::Transmit(ezRemoteTransmitMode tm, const ezArrayPtr<const ezUInt8>& data)
{
  if (m_RemoteMode == ezRemoteMode::None)
    return EZ_FAILURE;

  EZ_LOCK(GetMutex());

  if (InternalTransmit(tm, data).Failed())
    return EZ_FAILURE;

  // make sure the message is processed immediately
  UpdateRemoteInterface();

  return EZ_SUCCESS;
}


void ezRemoteInterface::Send(ezUInt32 uiSystemID, ezUInt32 uiMsgID)
{
  Send(ezRemoteTransmitMode::Reliable, uiSystemID, uiMsgID, ezArrayPtr<const ezUInt8>());
}

void ezRemoteInterface::Send(ezRemoteTransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const ezArrayPtr<const ezUInt8>& data)
{
  if (m_RemoteMode == ezRemoteMode::None)
    return;

  //if (!IsConnectedToOther())
  //  return;

  m_TempSendBuffer.SetCountUninitialized(12 + data.GetCount());
  *((ezUInt32*)&m_TempSendBuffer[0]) = m_uiApplicationID;
  *((ezUInt32*)&m_TempSendBuffer[4]) = uiSystemID;
  *((ezUInt32*)&m_TempSendBuffer[8]) = uiMsgID;

  if (!data.IsEmpty())
  {
    ezUInt8* pCopyDst = &m_TempSendBuffer[12];
    ezMemoryUtils::Copy(pCopyDst, data.GetPtr(), data.GetCount());
  }

  Transmit(tm, m_TempSendBuffer);
}

void ezRemoteInterface::Send(ezRemoteTransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData /*= nullptr*/, ezUInt32 uiDataBytes /*= 0*/)
{
  Send(tm, uiSystemID, uiMsgID, ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(pData), uiDataBytes));
}

void ezRemoteInterface::Send(ezRemoteTransmitMode tm, ezRemoteMessage& msg)
{
  Send(tm, msg.GetSystemID(), msg.GetMessageID(), ezArrayPtr<const ezUInt8>(msg.GetMessageData(), msg.GetMessageSize()));
}


void ezRemoteInterface::SetMessageHandler(ezUInt32 uiSystemID, ezRemoteMessageHandler messageHandler)
{
  m_MessageQueues[uiSystemID].m_MessageHandler = messageHandler;
}

ezUInt32 ezRemoteInterface::ExecuteMessageHandlers(ezUInt32 uiSystem)
{
  EZ_LOCK(m_Mutex);

  return ExecuteMessageHandlersForQueue(m_MessageQueues[uiSystem]);
}

ezUInt32 ezRemoteInterface::ExecuteAllMessageHandlers()
{
  EZ_LOCK(m_Mutex);

  ezUInt32 ret = 0;
  for (auto it = m_MessageQueues.GetIterator(); it.IsValid(); ++it)
  {
    ret += ExecuteMessageHandlersForQueue(it.Value());
  }

  return ret;
}

ezUInt32 ezRemoteInterface::ExecuteMessageHandlersForQueue(ezRemoteMessageQueue& queue)
{
  const ezUInt32 ret = queue.m_MessageQueue.GetCount();

  if (queue.m_MessageHandler.IsValid())
  {
    for (auto& msg : queue.m_MessageQueue)
    {
      queue.m_MessageHandler(msg);
    }
  }

  queue.m_MessageQueue.Clear();
  return ret;
}

void ezRemoteInterface::StartUpdateThread()
{
  StopUpdateThread();

  if (m_pUpdateThread == nullptr)
  {
    EZ_LOCK(m_Mutex);

    m_pUpdateThread = EZ_DEFAULT_NEW(ezRemoteThread);
    m_pUpdateThread->m_pRemoteInterface = this;
    m_pUpdateThread->Start();
  }
}

void ezRemoteInterface::StopUpdateThread()
{
  if (m_pUpdateThread != nullptr)
  {
    m_pUpdateThread->m_bKeepRunning = false;
    m_pUpdateThread->Join();

    EZ_LOCK(m_Mutex);
    EZ_DEFAULT_DELETE(m_pUpdateThread);
  }
}


void ezRemoteInterface::ReportConnectionToServer(ezUInt32 uiServerID)
{
  if (m_uiConnectedToServerWithID == uiServerID)
    return;

  m_uiConnectedToServerWithID = uiServerID;

  ezRemoteEvent e;
  e.m_Type = ezRemoteEvent::ConnectedToServer;
  e.m_uiOtherAppID = uiServerID;
  m_RemoteEvents.Broadcast(e);
}


void ezRemoteInterface::ReportConnectionToClient(ezUInt32 uiApplicationID)
{
  m_iConnectionsToClients++;

  ezRemoteEvent e;
  e.m_Type = ezRemoteEvent::ConnectedToClient;
  e.m_uiOtherAppID = uiApplicationID;
  m_RemoteEvents.Broadcast(e);
}

void ezRemoteInterface::ReportDisconnectedFromServer()
{
  m_uiConnectedToServerWithID = 0;

  ezRemoteEvent e;
  e.m_Type = ezRemoteEvent::DisconnectedFromServer;
  e.m_uiOtherAppID = m_uiConnectedToServerWithID;
  m_RemoteEvents.Broadcast(e);
}

void ezRemoteInterface::ReportDisconnectedFromClient(ezUInt32 uiApplicationID)
{
  m_iConnectionsToClients--;

  ezRemoteEvent e;
  e.m_Type = ezRemoteEvent::DisconnectedFromClient;
  e.m_uiOtherAppID = uiApplicationID;
  m_RemoteEvents.Broadcast(e);
}


void ezRemoteInterface::ReportMessage(ezUInt32 uiApplicationID, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const ezArrayPtr<const ezUInt8>& data)
{
  EZ_LOCK(m_Mutex);

  auto& queue = m_MessageQueues[uiSystemID];

  // discard messages for which we have no message handler
  if (!queue.m_MessageHandler.IsValid())
    return;

  // store the data for later
  auto& msg = queue.m_MessageQueue.ExpandAndGetRef();
  msg.m_uiApplicationID = uiApplicationID;
  msg.SetMessageID(uiSystemID, uiMsgID);
  msg.GetWriter().WriteBytes(data.GetPtr(), data.GetCount());
}

ezResult ezRemoteInterface::DetermineTargetAddress(const char* szConnectTo, ezUInt32& out_IP, ezUInt16& out_Port)
{
  out_IP = 0;
  out_Port = 0;

  ezStringBuilder sConnectTo = szConnectTo;

  const char* szColon = sConnectTo.FindLastSubString(":");
  if (szColon != nullptr)
  {
    sConnectTo.Shrink(0, ezStringUtils::GetStringElementCount(szColon));

    ezStringBuilder sPort = szColon + 1;

    ezInt32 tmp;
    if (ezConversionUtils::StringToInt(sPort, tmp).Succeeded())
      out_Port = tmp;
  }

  ezInt32 ip1 = 0;
  ezInt32 ip2 = 0;
  ezInt32 ip3 = 0;
  ezInt32 ip4 = 0;

  if (sConnectTo.IsEmpty() || sConnectTo.IsEqual_NoCase("localhost"))
  {
    ip1 = 127;
    ip2 = 0;
    ip3 = 0;
    ip4 = 1;
  }
  else if (sConnectTo.FindSubString(".") != nullptr)
  {
    ezHybridArray<ezString, 8> IP;
    sConnectTo.Split(false, IP, ".");

    if (IP.GetCount() != 4)
      return EZ_FAILURE;

    if (ezConversionUtils::StringToInt(IP[0], ip1).Failed())
      return EZ_FAILURE;
    if (ezConversionUtils::StringToInt(IP[1], ip2).Failed())
      return EZ_FAILURE;
    if (ezConversionUtils::StringToInt(IP[2], ip3).Failed())
      return EZ_FAILURE;
    if (ezConversionUtils::StringToInt(IP[3], ip4).Failed())
      return EZ_FAILURE;
  }
  else
  {
    return EZ_FAILURE;
  }

  out_IP = ((ip1 & 0xFF) | (ip2 & 0xFF) << 8 | (ip3 & 0xFF) << 16 | (ip4 & 0xFF) << 24);
  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezRemoteThread::ezRemoteThread()
    : ezThread("ezRemoteThread")
{
}

ezUInt32 ezRemoteThread::Run()
{
  ezTime lastPing;

  while (m_bKeepRunning && m_pRemoteInterface)
  {
    m_pRemoteInterface->UpdateRemoteInterface();

    // Send a Ping every once in a while
    if (m_pRemoteInterface->GetRemoteMode() == ezRemoteMode::Client)
    {
      ezTime tNow = ezTime::Now();

      if (tNow - lastPing > ezTime::Milliseconds(500))
      {
        lastPing = tNow;

        m_pRemoteInterface->UpdatePingToServer();
      }
    }

    ezThreadUtils::Sleep(ezTime::Milliseconds(10));
  }

  return 0;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_RemoteInterface);

