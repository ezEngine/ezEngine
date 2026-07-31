#include <Mcp/McpPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Mcp/McpSocket.h>

#  include <Foundation/Threading/AtomicUtils.h>

#  include <WS2tcpip.h>
#  include <WinSock2.h>

namespace
{
  ezAtomicInteger32 g_iWinsockRefCount;

  /// Winsock has to be initialised per process, but the MCP library has no startup of its own and may
  /// be used by a plugin that is loaded and unloaded repeatedly. Ref counting it around the sockets
  /// that need it keeps that self contained.
  void AddWinsockRef()
  {
    if (g_iWinsockRefCount.Increment() != 1)
      return;

    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
    {
      ezLog::Error("MCP: WSAStartup failed: {}", WSAGetLastError());
    }
  }

  void ReleaseWinsockRef()
  {
    if (g_iWinsockRefCount.Decrement() == 0)
    {
      WSACleanup();
    }
  }

  SOCKET ToSocket(ezInt64 iSocket)
  {
    return iSocket < 0 ? INVALID_SOCKET : static_cast<SOCKET>(iSocket);
  }
} // namespace

ezMcpSocket::ezMcpSocket()
{
  AddWinsockRef();
}

ezMcpSocket::~ezMcpSocket()
{
  Close();
  ReleaseWinsockRef();
}

ezResult ezMcpSocket::Listen(ezUInt16 uiPort)
{
  Close();

  const SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (sock == INVALID_SOCKET)
  {
    ezLog::Error("MCP: Could not create a socket: {}", WSAGetLastError());
    return EZ_FAILURE;
  }

  // Not SO_REUSEADDR: on Windows that does not only cover a port left over from an exited process, it
  // lets a *second* live process bind the same port, after which which of the two gets a connection is
  // undefined. Two editors on one port would then silently answer each other's tool calls instead of
  // the second one failing to bind, which is what the per-editor port exists to make happen.
  // SO_EXCLUSIVEADDRUSE is the opposite request: refuse to share, and refuse to be taken over.
  const BOOL bExclusive = TRUE;
  setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&bExclusive), sizeof(bExclusive));

  sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(uiPort);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
  {
    ezLog::Warning("MCP: Could not bind to port {}: {}", uiPort, WSAGetLastError());
    closesocket(sock);
    return EZ_FAILURE;
  }

  if (listen(sock, SOMAXCONN) == SOCKET_ERROR)
  {
    ezLog::Error("MCP: Could not listen on port {}: {}", uiPort, WSAGetLastError());
    closesocket(sock);
    return EZ_FAILURE;
  }

  // read the port back off the socket, so that a caller which passed 0 learns what it actually got
  sockaddr_in bound = {};
  int iBoundSize = sizeof(bound);

  if (getsockname(sock, reinterpret_cast<sockaddr*>(&bound), &iBoundSize) == 0)
  {
    m_uiPort = ntohs(bound.sin_port);
  }
  else
  {
    m_uiPort = uiPort;
  }

  m_iSocket = static_cast<ezInt64>(sock);
  return EZ_SUCCESS;
}

void ezMcpSocket::Close()
{
  if (m_iSocket < 0)
    return;

  closesocket(ToSocket(m_iSocket));

  m_iSocket = -1;
  m_uiPort = 0;
}

ezResult ezMcpSocket::Accept(ezMcpSocket& out_client, ezTime timeout)
{
  if (m_iSocket < 0)
    return EZ_FAILURE;

  const SOCKET listener = ToSocket(m_iSocket);

  fd_set readable;
  FD_ZERO(&readable);
  FD_SET(listener, &readable);

  timeval tv = {};
  tv.tv_sec = static_cast<long>(timeout.GetSeconds());
  tv.tv_usec = static_cast<long>((timeout - ezTime::MakeFromSeconds(tv.tv_sec)).GetMicroseconds());

  // the first argument is ignored on Windows, but is part of the signature
  if (select(0, &readable, nullptr, nullptr, &tv) != 1)
    return EZ_FAILURE;

  const SOCKET client = accept(listener, nullptr, nullptr);

  if (client == INVALID_SOCKET)
    return EZ_FAILURE;

  // A client that connects and then never sends anything would otherwise hold the transport thread
  // forever, and with it every later request.
  const DWORD uiTimeoutMS = 10000;
  setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&uiTimeoutMS), sizeof(uiTimeoutMS));

  out_client.Close();
  out_client.m_iSocket = static_cast<ezInt64>(client);
  return EZ_SUCCESS;
}

ezInt32 ezMcpSocket::Receive(void* pBuffer, ezUInt32 uiSize)
{
  if (m_iSocket < 0)
    return -1;

  return static_cast<ezInt32>(recv(ToSocket(m_iSocket), static_cast<char*>(pBuffer), static_cast<int>(uiSize), 0));
}

ezResult ezMcpSocket::SendAll(const void* pBuffer, ezUInt32 uiSize)
{
  if (m_iSocket < 0)
    return EZ_FAILURE;

  const char* pBytes = static_cast<const char*>(pBuffer);
  ezUInt32 uiSent = 0;

  while (uiSent < uiSize)
  {
    const int iResult = send(ToSocket(m_iSocket), pBytes + uiSent, static_cast<int>(uiSize - uiSent), 0);

    if (iResult <= 0)
      return EZ_FAILURE;

    uiSent += static_cast<ezUInt32>(iResult);
  }

  return EZ_SUCCESS;
}

#endif
