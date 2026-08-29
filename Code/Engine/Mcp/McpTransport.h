#pragma once

#include <Mcp/McpDLL.h>

#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>

class ezMcpTransportThread;

/// One HTTP request, as far as the transport understands it.
struct ezMcpHttpRequest
{
  ezString m_sMethod;
  ezString m_sPath;
  ezStringBuilder m_sBody;
};

/// The answer to one request.
struct ezMcpHttpResponse
{
  /// E.g. "200 OK". The transport turns this into a status line and adds the headers.
  ezStringBuilder m_sStatus = "200 OK";

  /// May be empty, e.g. for the 202 that a JSON-RPC notification is answered with.
  ezStringBuilder m_sBody;

  /// Set by the handler to mean "not yet - ask me again next time you pump".
  ///
  /// The request stays pending and the client keeps waiting, which is what a tool that has to let the
  /// host run for a while needs. The transport resets it before each attempt, so a handler only has to
  /// set it, never clear it. See ezMcpToolResult::m_bNotFinished.
  bool m_bDeferred = false;
};

/// Accepts HTTP requests on a socket and hands them to the main thread to be answered.
///
/// Deliberately not an event loop. A tool call may touch anything in the host - the world, the editor's
/// documents, the renderer - none of which is thread safe, so the answer has to be produced on the main
/// thread. The socket work is what happens on the transport's own thread:
///
///   transport thread                      main thread
///   ----------------                      -----------
///   accept, read the request
///   publish it, then wait  --------->     ProcessPendingRequests() picks it up
///                                         the handler runs the tool
///                          <---------     publishes the response and wakes the transport
///   write the response, close
///
/// Only one request is in flight at a time. That mirrors what the server does anyway - one request per
/// connection, `Connection: close` - and it means a tool never has to reason about a second call
/// arriving while it runs.
///
/// The consequence to be aware of: while the main thread does not call ProcessPendingRequests(), a
/// client just waits. That is the same behaviour a blocking tool call has always had, but it now also
/// applies to a host whose main loop is idle.
class EZ_MCP_DLL ezMcpTransport
{
public:
  /// Produces the response to one request. Always called on the main thread.
  using RequestHandler = ezDelegate<void(const ezMcpHttpRequest& request, ezMcpHttpResponse& ref_response)>;

  ezMcpTransport();
  ~ezMcpTransport();

  /// Binds to 127.0.0.1 at the given port and starts the transport thread.
  ezResult Start(ezUInt16 uiPort, RequestHandler handler);

  /// Stops the thread and closes the socket. Safe to call when not running.
  ///
  /// A request that is waiting for an answer at that moment is abandoned rather than answered - the
  /// client sees the connection close, which is the honest report of a server that went away.
  void Stop();

  bool IsRunning() const { return m_pThread != nullptr; }

  /// The port that is actually bound, or 0 while not running.
  ezUInt16 GetPort() const { return m_uiPort; }

  /// Answers a request if one is waiting. Call this once per frame from the main thread.
  ///
  /// Does nothing when no request is pending, so it is cheap enough to call unconditionally.
  void ProcessPendingRequests();

  /// Whether a request is waiting to be answered.
  ///
  /// For a host whose main loop sleeps when idle: it has to wake up and pump when this turns true, or
  /// the client waits forever.
  bool HasPendingRequest() const;

private:
  friend class ezMcpTransportThread;

  /// Called by the transport thread. Publishes the request, waits for the main thread to answer it, and
  /// returns EZ_FAILURE if the transport was stopped before that happened.
  ezResult DispatchAndWait(const ezMcpHttpRequest& request, ezMcpHttpResponse& out_response);

  RequestHandler m_Handler;
  ezUniquePtr<ezMcpTransportThread> m_pThread;
  ezUInt16 m_uiPort = 0;

  mutable ezMutex m_Mutex;
  const ezMcpHttpRequest* m_pPendingRequest = nullptr;
  ezMcpHttpResponse* m_pPendingResponse = nullptr;
  bool m_bRequestAnswered = false;
  bool m_bShutdown = false;

  /// Raised by the main thread once it has filled in the response, and by Stop() to release a transport
  /// thread that would otherwise wait for an answer that is never coming.
  ezThreadSignal m_ResponseSignal;
};
