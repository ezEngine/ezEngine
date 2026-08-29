#pragma once

#include <Mcp/McpTransport.h>

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>

/// A minimal MCP (Model Context Protocol) server, speaking the 'streamable HTTP' transport.
///
/// Hosts the JSON-RPC half; the sockets and the main-thread hand-off are ezMcpTransport's job, and the
/// tools themselves come from ezMcpToolRegistry. That split is what lets the editor and a game process
/// run the same server.
///
/// The protocol implemented here is a subset: `initialize`, `tools/list` and `tools/call`, no sessions,
/// no SSE, no batching. See Status.md in EditorPluginMcp for why, and for what that costs.
class EZ_MCP_DLL ezMcpServer
{
public:
  /// \param sServerName Reported to the client as `serverInfo.name`, so that an agent talking to two
  ///        of these at once can tell the editor from the game.
  ezMcpServer(ezStringView sServerName);
  ~ezMcpServer();

  /// Starts listening on 127.0.0.1 at the given port.
  ezResult Start(ezUInt16 uiPort);

  /// Stops the server and drops all pending connections. Safe to call when not running.
  void Stop();

  bool IsRunning() const { return m_Transport.IsRunning(); }

  /// The port that is currently being listened on, or 0 while not running.
  ezUInt16 GetPort() const { return m_Transport.GetPort(); }

  /// Answers whatever request is waiting. Must be called regularly from the main thread, or
  /// clients never get an answer.
  void ProcessPendingRequests() { m_Transport.ProcessPendingRequests(); }

  /// Whether a client is currently waiting for an answer.
  ///
  /// A host whose main loop sleeps when there is nothing to draw has to wake up and pump when this is
  /// true - see the editor's engine process.
  bool HasPendingRequest() const { return m_Transport.HasPendingRequest(); }

  /// The running server, or nullptr if none was started. Exists so that tools can report how to
  /// reach this process, which is not otherwise discoverable from inside a tool call.
  static ezMcpServer* GetInstance() { return s_pInstance; }

private:
  /// Turns one HTTP request into a response. Runs on the main thread.
  void HandleRequest(const ezMcpHttpRequest& request, ezMcpHttpResponse& ref_response);

  /// Turns one JSON-RPC message into a response body. Returns EZ_FAILURE for notifications,
  /// which must not be answered with a body at all.
  ///
  /// \param out_bDeferred Set when the tool asked to be re-run later. The response body is then
  ///        meaningless and the caller must not send anything - see ezMcpToolResult::m_bNotFinished.
  ezResult HandleMessage(const ezVariantDictionary& msg, ezStringBuilder& out_sResponse, bool& out_bDeferred);

  ezString m_sServerName;
  ezMcpTransport m_Transport;

  static ezMcpServer* s_pInstance;
};
