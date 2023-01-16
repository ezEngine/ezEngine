
#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <Foundation/Communication/Implementation/Linux/PipeChannel_linux.h>

#  include <Foundation/Communication/Implementation/Linux/MessageLoop_linux.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>

#  include <fcntl.h>
#  include <sys/socket.h>
#  include <sys/un.h>

ezPipeChannel_linux::ezPipeChannel_linux(const char* szAddress, Mode::Enum Mode)
  : ezIpcChannel(szAddress, Mode)
{
  ezStringBuilder pipePath = ezOSFile::GetTempDataFolder("ez-pipes");

  // Make sure the directory exists that we want to place the pipes in.
  ezOSFile::CreateDirectoryStructure(pipePath).IgnoreResult();

  pipePath.AppendPath(szAddress);
  pipePath.Append(".server");

  m_serverSocketPath = pipePath;
  pipePath.Shrink(0, 7); // strip .server
  pipePath.Append(".client");
  m_clientSocketPath = pipePath;

  const char* thisSocketPath = (Mode == Mode::Server) ? m_serverSocketPath.GetData() : m_clientSocketPath.GetData();

  int& targetSocket = (Mode == Mode::Server) ? m_serverSocketFd : m_clientSocketFd;

  targetSocket = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (targetSocket == -1)
  {
    ezLog::Error("[IPC]Failed to create unix domain socket. error {}", errno);
    return;
  }

  // If the socket file already exists, delete it
  ezOSFile::DeleteFile(thisSocketPath).IgnoreResult();

  struct sockaddr_un addr = {};
  addr.sun_family = AF_UNIX;

  if (strlen(thisSocketPath) >= EZ_ARRAY_SIZE(addr.sun_path) - 1)
  {
    ezLog::Error("[IPC]Given ipc channel address is to long. Resulting path '{}' path length limit {}", strlen(thisSocketPath), EZ_ARRAY_SIZE(addr.sun_path) - 1);
    close(targetSocket);
    targetSocket = -1;
    return;
  }

  strcpy(addr.sun_path, thisSocketPath);
  if (bind(targetSocket, (struct sockaddr*)&addr, SUN_LEN(&addr)) == -1)
  {
    ezLog::Error("[IPC]Failed to bind unix domain socket to '{}' error {}", thisSocketPath, errno);
    close(targetSocket);
    targetSocket = -1;
    return;
  }

  m_pOwner->AddChannel(this);
}

ezPipeChannel_linux::~ezPipeChannel_linux()
{
  if (m_pOwner)
  {
    static_cast<ezMessageLoop_linux*>(m_pOwner)->RemovePendingWaits(this);
  }

  if (m_serverSocketFd >= 0)
  {
    close(m_serverSocketFd);
    m_serverSocketFd = -1;
  }

  if (m_clientSocketFd >= 0)
  {
    close(m_clientSocketFd);
    m_clientSocketFd = -1;
  }

  if (m_Mode == Mode::Server)
  {
    ezOSFile::DeleteFile(m_serverSocketPath).IgnoreResult();
  }
  else
  {
    ezOSFile::DeleteFile(m_clientSocketPath).IgnoreResult();
  }
}

void ezPipeChannel_linux::AddToMessageLoop(ezMessageLoop* pMessageLoop)
{
}

void ezPipeChannel_linux::InternalConnect()
{
  if (m_bConnected || m_Connecting)
  {
    return;
  }


  if (m_Mode == Mode::Server)
  {
    if (m_serverSocketFd < 0)
    {
      return;
    }
    m_Connecting = true;
    listen(m_serverSocketFd, 1);
    static_cast<ezMessageLoop_linux*>(m_pOwner)->RegisterWait(this, ezMessageLoop_linux::WaitType::Accept, m_serverSocketFd);
  }
  else
  {
    if (m_clientSocketFd < 0)
    {
      return;
    }
    m_Connecting = true;
    struct sockaddr_un serverAddress = {};
    serverAddress.sun_family = AF_UNIX;
    strcpy(serverAddress.sun_path, m_serverSocketPath.GetData());

    int connectResult = connect(m_clientSocketFd, (struct sockaddr*)&serverAddress, SUN_LEN(&serverAddress));

    static_cast<ezMessageLoop_linux*>(m_pOwner)->RegisterWait(this, ezMessageLoop_linux::WaitType::Connect, m_clientSocketFd);
  }
}

void ezPipeChannel_linux::InternalDisconnect()
{
  if (!m_bConnected && !m_Connecting)
  {
    return;
  }

  static_cast<ezMessageLoop_linux*>(m_pOwner)->RemovePendingWaits(this);

  close(m_clientSocketFd);
  m_clientSocketFd = -1;

  {
    EZ_LOCK(m_OutputQueueMutex);
    m_OutputQueue.Clear();
  }

  m_bConnected = false;
  m_Connecting = false;

  m_Events.Broadcast(ezIpcChannelEvent(m_Mode == Mode::Server ? ezIpcChannelEvent::DisconnectedFromClient : ezIpcChannelEvent::DisconnectedFromServer, this));

  m_IncomingMessages.RaiseSignal(); // Wakeup anyone still waiting for messages
}

void ezPipeChannel_linux::InternalSend()
{
  const ezMemoryStreamStorageInterface* storage = nullptr;
  {
    EZ_LOCK(m_OutputQueueMutex);
    if (m_OutputQueue.IsEmpty())
    {
      return;
    }
    storage = &m_OutputQueue.PeekFront();
  }

  while (true)
  {

    ezUInt64 uiToWrite = storage->GetStorageSize64() - m_previousSendOffset;
    ezUInt64 uiNextOffset = m_previousSendOffset;
    while (uiToWrite > 0)
    {
      const ezArrayPtr<const ezUInt8> range = storage->GetContiguousMemoryRange(uiNextOffset);

      int res = send(m_clientSocketFd, range.GetPtr(), range.GetCount(), 0);

      if (res < 0)
      {
        int errorCode = errno;
        // We can't send at the moment. Wait until we can send again.
        if (errorCode == EWOULDBLOCK)
        {
          m_previousSendOffset = uiNextOffset;
          static_cast<ezMessageLoop_linux*>(m_pOwner)->RegisterWait(this, ezMessageLoop_linux::WaitType::Send, m_clientSocketFd);
          return;
        }
        ezLog::Error("[IPC]ezPipeChannel_linux failed to send. Error {}", errorCode);
        InternalDisconnect();
        return;
      }

      uiToWrite -= static_cast<ezUInt64>(res);
      uiNextOffset += res;
    }
    m_previousSendOffset = 0;

    {
      EZ_LOCK(m_OutputQueueMutex);
      m_OutputQueue.PopFront();
      if (m_OutputQueue.IsEmpty())
      {
        return;
      }
      storage = &m_OutputQueue.PeekFront();
    }
  }
}

void ezPipeChannel_linux::AcceptIncomingConnection()
{
  struct sockaddr_un incomingConnection = {};
  socklen_t len = sizeof(incomingConnection);
  m_clientSocketFd = accept4(m_serverSocketFd, (struct sockaddr*)&incomingConnection, &len, SOCK_NONBLOCK);
  if (m_clientSocketFd == -1)
  {
    ezLog::Error("[IPC]Failed to accept incoming connection. Error {}", errno);
    // Wait for the next incoming connection
    listen(m_serverSocketFd, 1);
    static_cast<ezMessageLoop_linux*>(m_pOwner)->RegisterWait(this, ezMessageLoop_linux::WaitType::Accept, m_serverSocketFd);
  }
  else
  {
    m_bConnected = true;
    m_Events.Broadcast(ezIpcChannelEvent(ezIpcChannelEvent::ConnectedToClient, this));

    // We are connected. Register for incoming messages events.
    static_cast<ezMessageLoop_linux*>(m_pOwner)->RegisterWait(this, ezMessageLoop_linux::WaitType::IncomingMessage, m_clientSocketFd);
  }
}

bool ezPipeChannel_linux::NeedWakeup() const
{
  return true;
}

void ezPipeChannel_linux::ProcessConnectSuccessfull()
{
  m_Connecting = false;
  m_bConnected = true;
  m_Events.Broadcast(ezIpcChannelEvent(ezIpcChannelEvent::ConnectedToServer, this));

  // We are connected. Register for incoming messages events.
  static_cast<ezMessageLoop_linux*>(m_pOwner)->RegisterWait(this, ezMessageLoop_linux::WaitType::IncomingMessage, m_clientSocketFd);
}

void ezPipeChannel_linux::ProcessIncomingPackages()
{
  while (true)
  {
    ssize_t recieveResult = recv(m_clientSocketFd, m_InputBuffer, EZ_ARRAY_SIZE(m_InputBuffer), 0);
    if (recieveResult == 0)
    {
      InternalDisconnect();
      return;
    }

    if (recieveResult < 0)
    {
      int errorCode = errno;
      if (errorCode == EWOULDBLOCK)
      {
        return;
      }
      if (errorCode != ECONNRESET)
      {
        ezLog::Error("[IPC]ezPipeChannel_linux recieve error {}", errorCode);
      }
      InternalDisconnect();
      return;
    }

    ReceiveMessageData(ezArrayPtr(m_InputBuffer, static_cast<ezUInt32>(recieveResult)));
  }
}

#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Linux_PipeChannel_linux);

