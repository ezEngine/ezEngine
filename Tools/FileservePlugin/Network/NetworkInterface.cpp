#include <PCH.h>
#include <FileservePlugin/Network/NetworkInterface.h>

ezResult ezNetworkInterface::CreateConnection(ezUInt32 uiConnectionToken, ezNetworkMode mode, ezUInt16 uiPort, const char* szServerAddress, bool bStartUpdateThread)
{
  EZ_LOCK(GetMutex());

  ShutdownConnection();

  m_uiConnectionToken = uiConnectionToken;
  m_uiApplicationID = (ezUInt32)ezTime::Now().GetSeconds();
  ezLog::Dev("Application ID is {0}", m_uiApplicationID);

  if (InternalCreateConnection(mode, uiPort, szServerAddress).Failed())
  {
    ShutdownConnection();
    return EZ_FAILURE;
  }

  m_uiPort = uiPort;
  m_NetworkMode = mode;

  UpdateNetwork();

  if (bStartUpdateThread)
  {
    StartUpdateThread();
  }

  return EZ_SUCCESS;
}

ezResult ezNetworkInterface::StartServer(ezUInt32 uiConnectionToken, ezUInt16 uiPort, bool bStartUpdateThread /*= true*/)
{
  return CreateConnection(uiConnectionToken, ezNetworkMode::Server, uiPort, nullptr, bStartUpdateThread);
}

ezResult ezNetworkInterface::ConnectToServer(ezUInt32 uiConnectionToken, const char* szAddress, bool bStartUpdateThread /*= true*/)
{
  return CreateConnection(uiConnectionToken, ezNetworkMode::Client, 0, szAddress, bStartUpdateThread);
}

void ezNetworkInterface::ShutdownConnection()
{
  StopUpdateThread();

  EZ_LOCK(GetMutex());

  if (m_NetworkMode != ezNetworkMode::None)
  {
    InternalShutdownConnection();

    m_NetworkMode = ezNetworkMode::None;
    m_uiPort = 0;
    m_uiApplicationID = 0;
    m_uiConnectionToken = 0;
  }
}

void ezNetworkInterface::UpdatePingToServer()
{
  if (m_NetworkMode == ezNetworkMode::Server)
  {
    EZ_LOCK(GetMutex());
    m_PingToServer = InternalGetPingToServer();
  }
}

void ezNetworkInterface::UpdateNetwork()
{
  EZ_LOCK(GetMutex());

  InternalUpdateNetwork();
}

ezResult ezNetworkInterface::Transmit(ezNetworkTransmitMode tm, const ezArrayPtr<const ezUInt8>& data)
{
  if (m_NetworkMode == ezNetworkMode::None)
    return EZ_FAILURE;

  EZ_LOCK(GetMutex());

  if (InternalTransmit(tm, data).Failed())
    return EZ_FAILURE;

  // make sure the message is processed immediately
  UpdateNetwork();

  return EZ_SUCCESS;
}


void ezNetworkInterface::SendShortMessage(ezUInt32 uiSystemID, ezUInt32 uiMsgID)
{
  Send(ezNetworkTransmitMode::Reliable, uiSystemID, uiMsgID, ezArrayPtr<const ezUInt8>());
}

void ezNetworkInterface::Send(ezNetworkTransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const ezArrayPtr<const ezUInt8>& data)
{
  if (m_NetworkMode == ezNetworkMode::None)
    return;

  //if (!IsConnectedToOther())
  //  return;

  m_TempSendBuffer.SetCountUninitialized(8 + data.GetCount());
  *((ezUInt32*)&m_TempSendBuffer[0]) = uiSystemID;
  *((ezUInt32*)&m_TempSendBuffer[4]) = uiMsgID;

  if (!data.IsEmpty())
  {
    ezUInt8* pCopyDst = &m_TempSendBuffer[8];
    ezMemoryUtils::Copy(pCopyDst, data.GetPtr(), data.GetCount());
  }

  Transmit(tm, m_TempSendBuffer);
}

void ezNetworkInterface::StartUpdateThread()
{
  StopUpdateThread();

  if (m_pUpdateThread == nullptr)
  {
    m_pUpdateThread = EZ_DEFAULT_NEW(ezNetworkThread);
    m_pUpdateThread->m_pNetwork = this;
    m_pUpdateThread->Start();
  }
}

void ezNetworkInterface::StopUpdateThread()
{
  if (m_pUpdateThread != nullptr)
  {
    m_pUpdateThread->m_bKeepRunning = false;
    m_pUpdateThread->Join();

    EZ_LOCK(GetMutex());
    EZ_DEFAULT_DELETE(m_pUpdateThread);
  }
}


void ezNetworkInterface::ReportConnectionToServer(ezUInt32 uiServerID)
{
  m_uiConnectedToServerWithID = uiServerID;

  ezLog::Info("Connected to server with ID {0}", uiServerID);
  /// \todo Broadcast
}


void ezNetworkInterface::ReportConnectionToClient()
{
  m_iConnectionsToClients++;

  ezLog::Info("Connected with client {0}", m_iConnectionsToClients);
  /// \todo Broadcast
}

void ezNetworkInterface::ReportDisconnectedFromServer()
{
  m_uiConnectedToServerWithID = 0;

  ezLog::Info("Disconnected from server");
  /// \todo Broadcast

}

void ezNetworkInterface::ReportDisconnectedFromClient()
{
  ezLog::Info("Disconnected from client {0}", m_iConnectionsToClients);

  m_iConnectionsToClients--;
  /// \todo Broadcast
}

ezResult ezNetworkInterface::DetermineTargetAddress(const char* szConnectTo, ezUInt32& out_IP, ezUInt16& out_Port)
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

ezNetworkThread::ezNetworkThread()
  : ezThread("ezNetworkThread")
{
}

ezUInt32 ezNetworkThread::Run()
{
  ezTime lastPing;

  while (m_bKeepRunning && m_pNetwork)
  {
    m_pNetwork->UpdateNetwork();

    // Send a Ping every once in a while
    if (m_pNetwork->GetNetworkMode() == ezNetworkMode::Client)
    {
      ezTime tNow = ezTime::Now();

      if (tNow - lastPing > ezTime::Milliseconds(500))
      {
        lastPing = tNow;

        m_pNetwork->UpdatePingToServer();
      }
    }

    ezThreadUtils::Sleep(10);
  }

  return 0;
}
