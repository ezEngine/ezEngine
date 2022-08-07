
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
  pipePath.AppendPath(szAddress);
  pipePath.Append(".server");

  m_serverSocketPath = pipePath;
  pipePath.Shrink(0, 7); // strip .server
  pipePath.Append(".client");
  m_clientSocketPath = pipePath;

  const char* thisSocketPath = (Mode == Mode::Server) ? m_serverSocketPath.GetData() : m_clientSocketPath.GetData();

  int& targetSocket = (Mode == Mode::Server) ? m_serverSocketFd : m_clientSocketFd;

  targetSocket = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (targetSocket == -1)
  {
    ezLog::Error("Failed to create unix domain socket. error {}", errno);
    return;
  }

  // If the socket file already exists, delete it
  ezOSFile::DeleteFile(thisSocketPath).IgnoreResult();

  struct sockaddr_un addr = {};
  addr.sun_family = AF_UNIX;

  if (strlen(thisSocketPath) >= EZ_ARRAY_SIZE(addr.sun_path) - 1)
  {
    ezLog::Error("Given ipc channel address is to long. Resulting path '{}' path length limit {}", strlen(thisSocketPath), EZ_ARRAY_SIZE(addr.sun_path) - 1);
    close(targetSocket);
    targetSocket = -1;
    return;
  }

  strcpy(addr.sun_path, thisSocketPath);
  if (bind(targetSocket, (struct sockaddr*)&addr, sizeof(addr)) == -1)
  {
    ezLog::Error("Failed to bind unix domain socket to '{}' error {}", thisSocketPath, errno);
    close(targetSocket);
    targetSocket = -1;
    return;
  }
}

void ezPipeChannel_linux::InternalConnect()
{
  if (m_Connected)
  {
    return;
  }

  if (m_Mode == Mode::Server)
  {
    static_cast<ezMessageLoop_linux*>(m_pOwner)->RegisterWait(this, ezMessageLoop_linux::WaitType::Accept);
  }
  else
  {
    // TODO
  }
}

void ezPipeChannel_linux::AcceptIncomingConnection()
{
  struct sockaddr_un incomingConnection = {};
  socklen_t len = 0;
  m_clientSocketFd = accept(m_serverSocketFd, (struct sockaddr*)&incomingConnection, &len);
  if (m_clientSocketFd == -1)
  {
    ezLog::Error("Failed to accept incoming connection. Error {}", errno);
    // Wait for the next incoming connection
    static_cast<ezMessageLoop_linux*>(m_pOwner)->RegisterWait(this, ezMessageLoop_linux::WaitType::Accept);
  }
  else
  {
    ezLog::Dev("Accepting incoming connection from '{}'", incomingConnection.sun_path);
  }
}

void ezPipeChannel_linux::ProcessConnectSuccessfull()
{
  ezLog::Dev("ezPipeChannel_linux successfully connected");
  // We are connected. Register for incoming messages events.
  static_cast<ezMessageLoop_linux*>(m_pOwner)->RegisterWait(this, ezMessageLoop_linux::WaitType::IncomingMessage);
}

void ezPipeChannel_linux::ProcessIncomingPackages()
{
  while (true)
  {
    ssize_t recieveResult = recv(m_clientSocketFd, m_InputBuffer, EZ_ARRAY_SIZE(m_InputBuffer), 0);
    if (recieveResult == 0)
    {
      return;
    }

    if (recieveResult < 0)
    {
        int errorCode = errno;
        if(errorCode == EWOULDBLOCK)
        {
            return;
        }
        ezLog::Error("ezPipeChannel_linux recieve error {}", errorCode);
        return;
    }

    ReceiveMessageData(ezArrayPtr(m_InputBuffer, static_cast<ezUInt32>(recieveResult)));
  }
}

#endif