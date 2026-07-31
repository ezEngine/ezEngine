#pragma once

#include <Mcp/McpDLL.h>

#include <Foundation/Time/Time.h>

/// \brief The little bit of TCP that the MCP transport needs, with the platform differences taken out.
///
/// Deliberately minimal and blocking: one listening socket, one connection at a time, one request per
/// connection. The transport runs this on its own thread, so blocking is the simple option rather than
/// the expensive one. Everything above this - HTTP framing, JSON-RPC, dispatch - is platform free and
/// lives in ezMcpTransport and ezMcpServer.
///
/// Not copyable; a socket has an owner, and closing it twice closes a handle somebody else got given.
class EZ_MCP_DLL ezMcpSocket
{
public:
  ezMcpSocket();
  ~ezMcpSocket();

  ezMcpSocket(const ezMcpSocket&) = delete;
  ezMcpSocket& operator=(const ezMcpSocket&) = delete;

  /// \brief Binds to 127.0.0.1 at the given port and starts listening.
  ///
  /// Loopback only, never INADDR_ANY: this server has no authentication of any kind, so it must not be
  /// reachable from another machine.
  ///
  /// A port of 0 lets the OS pick one - use GetPort() to find out which. That is only useful to a
  /// caller that can publish the result, since a client needs the number to build its URL.
  ezResult Listen(ezUInt16 uiPort);

  /// \brief The port that is actually bound, read back from the socket rather than from the argument.
  ezUInt16 GetPort() const { return m_uiPort; }

  bool IsValid() const { return m_iSocket >= 0; }

  void Close();

  /// \brief Waits up to \a timeout for an incoming connection.
  ///
  /// Returns EZ_FAILURE when nothing arrived in time, which is not an error - it is how the accept
  /// loop gets a chance to notice that it was asked to stop. There is no portable way to interrupt a
  /// blocking accept(), so it is polled instead.
  ezResult Accept(ezMcpSocket& out_client, ezTime timeout);

  /// \brief Reads whatever has arrived, blocking until something does.
  ///
  /// Returns the number of bytes read, 0 when the peer closed the connection in an orderly way, and a
  /// negative value on error or timeout. Accepted sockets carry a receive timeout so that a client
  /// which opens a connection and then says nothing cannot wedge the transport thread.
  ezInt32 Receive(void* pBuffer, ezUInt32 uiSize);

  /// \brief Writes the whole buffer, looping over partial writes.
  ezResult SendAll(const void* pBuffer, ezUInt32 uiSize);

private:
  /// The platform handle. Windows' SOCKET is an unsigned pointer and POSIX' is an int, but both use an
  /// all-ones value for 'invalid', so a signed 64 bit integer holds either and -1 means the same thing.
  ezInt64 m_iSocket = -1;
  ezUInt16 m_uiPort = 0;
};
