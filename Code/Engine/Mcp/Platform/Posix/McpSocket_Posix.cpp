#include <Mcp/McpPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)

#  include <Mcp/McpSocket.h>

#  include <arpa/inet.h>
#  include <errno.h>
#  include <netinet/in.h>
#  include <sys/select.h>
#  include <sys/socket.h>
#  include <unistd.h>

// macOS has no MSG_NOSIGNAL; it spells the same thing SO_NOSIGPIPE, as a socket option
#  ifndef MSG_NOSIGNAL
#    define MSG_NOSIGNAL 0
#  endif

ezMcpSocket::ezMcpSocket() = default;

ezMcpSocket::~ezMcpSocket()
{
  Close();
}

ezResult ezMcpSocket::Listen(ezUInt16 uiPort)
{
  Close();

  const int iSocket = socket(AF_INET, SOCK_STREAM, 0);

  if (iSocket < 0)
  {
    ezLog::Error("MCP: Could not create a socket: {}", errno);
    return EZ_FAILURE;
  }

  // Without this a port stays unusable for a couple of minutes after the process that held it exited,
  // which turns 'restart the editor' into 'restart the editor and wait'.
  const int iReuse = 1;
  setsockopt(iSocket, SOL_SOCKET, SO_REUSEADDR, &iReuse, sizeof(iReuse));

  sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(uiPort);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (bind(iSocket, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) < 0)
  {
    ezLog::Warning("MCP: Could not bind to port {}: {}", uiPort, errno);
    close(iSocket);
    return EZ_FAILURE;
  }

  if (listen(iSocket, SOMAXCONN) < 0)
  {
    ezLog::Error("MCP: Could not listen on port {}: {}", uiPort, errno);
    close(iSocket);
    return EZ_FAILURE;
  }

  // read the port back off the socket, so that a caller which passed 0 learns what it actually got
  sockaddr_in bound = {};
  socklen_t boundSize = sizeof(bound);

  if (getsockname(iSocket, reinterpret_cast<sockaddr*>(&bound), &boundSize) == 0)
  {
    m_uiPort = ntohs(bound.sin_port);
  }
  else
  {
    m_uiPort = uiPort;
  }

  m_iSocket = iSocket;
  return EZ_SUCCESS;
}

void ezMcpSocket::Close()
{
  if (m_iSocket < 0)
    return;

  close(static_cast<int>(m_iSocket));

  m_iSocket = -1;
  m_uiPort = 0;
}

ezResult ezMcpSocket::Accept(ezMcpSocket& out_client, ezTime timeout)
{
  if (m_iSocket < 0)
    return EZ_FAILURE;

  const int iListener = static_cast<int>(m_iSocket);

  fd_set readable;
  FD_ZERO(&readable);
  FD_SET(iListener, &readable);

  timeval tv = {};
  tv.tv_sec = static_cast<time_t>(timeout.GetSeconds());
  tv.tv_usec = static_cast<suseconds_t>((timeout - ezTime::MakeFromSeconds(static_cast<double>(tv.tv_sec))).GetMicroseconds());

  if (select(iListener + 1, &readable, nullptr, nullptr, &tv) != 1)
    return EZ_FAILURE;

  const int iClient = accept(iListener, nullptr, nullptr);

  if (iClient < 0)
    return EZ_FAILURE;

  // A client that connects and then never sends anything would otherwise hold the transport thread
  // forever, and with it every later request.
  timeval recvTimeout = {};
  recvTimeout.tv_sec = 10;
  setsockopt(iClient, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout));

#  ifdef SO_NOSIGPIPE
  // the macOS spelling of MSG_NOSIGNAL: a client hanging up mid-response must not raise SIGPIPE
  const int iNoSigPipe = 1;
  setsockopt(iClient, SOL_SOCKET, SO_NOSIGPIPE, &iNoSigPipe, sizeof(iNoSigPipe));
#  endif

  out_client.Close();
  out_client.m_iSocket = iClient;
  return EZ_SUCCESS;
}

ezInt32 ezMcpSocket::Receive(void* pBuffer, ezUInt32 uiSize)
{
  if (m_iSocket < 0)
    return -1;

  return static_cast<ezInt32>(recv(static_cast<int>(m_iSocket), pBuffer, uiSize, 0));
}

ezResult ezMcpSocket::SendAll(const void* pBuffer, ezUInt32 uiSize)
{
  if (m_iSocket < 0)
    return EZ_FAILURE;

  const char* pBytes = static_cast<const char*>(pBuffer);
  ezUInt32 uiSent = 0;

  while (uiSent < uiSize)
  {
    // MSG_NOSIGNAL: a client that hangs up mid-response must not take the process down with SIGPIPE
    const ssize_t iResult = send(static_cast<int>(m_iSocket), pBytes + uiSent, uiSize - uiSent, MSG_NOSIGNAL);

    if (iResult <= 0)
      return EZ_FAILURE;

    uiSent += static_cast<ezUInt32>(iResult);
  }

  return EZ_SUCCESS;
}

#endif
