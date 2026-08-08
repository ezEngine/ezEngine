#include <McpPlugin/McpPluginPCH.h>

#include <McpPlugin/McpEngineHost.h>

#include <Mcp/McpServer.h>
#include <Mcp/McpToolRegistry.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Utilities/CommandLineUtils.h>

/// The port to serve MCP on.
///
/// This is the option a standalone game is launched with. It is spelled without the 'editor-' prefix
/// that the editor's own option carries, because the editor forwards its entire command line to the
/// engine process that runs the game: with a single name, both processes would read the same number and
/// the second one to start would fail to bind.
///
/// There is deliberately no default. This plugin is mandatory and therefore loaded by every project, so
/// a default would have every game and every ezEditorProcessor child open a port that nobody asked for.
/// The server only exists when it was asked for by name.
static ezCommandLineOptionInt s_opt_McpPort("_Mcp", "-mcpport",
  "The port that this game process serves MCP on, on 127.0.0.1.\n"
  "Without it - and without '-editor-mcpport' - no server is started at all.",
  0, 0, 0xFFFF);

/// The editor's port, read here only to derive one from it.
///
/// Declared so that it shows up in app_command_line_options, and so that the +1 rule is stated in one
/// place that an agent can actually read. The editor is what interprets the value itself.
static ezCommandLineOptionInt s_opt_EditorMcpPort("_Mcp", "-editor-mcpport",
  "The editor's own MCP port. This process is started by the editor with the editor's whole command line,\n"
  "so when '-mcpport' is absent this is what it derives its port from: editor port + 1.",
  0, 0, 0xFFFF);

ezMcpServer* ezMcpEngineHost::s_pServer = nullptr;
ezUInt64 ezMcpEngineHost::s_uiFrameCount = 0;
bool ezMcpEngineHost::s_bQuitRequested = false;

ezUInt16 ezMcpEngineHost::ResolvePort()
{
  const ezInt32 iOwnPort = s_opt_McpPort.GetOptionValue(ezCommandLineOption::LogMode::FirstTimeIfSpecified);

  if (iOwnPort > 0)
    return static_cast<ezUInt16>(iOwnPort);

  const ezInt32 iEditorPort = s_opt_EditorMcpPort.GetOptionValue(ezCommandLineOption::LogMode::FirstTimeIfSpecified);

  // The editor's port + 1. An agent that knows the editor's port can compute this one without being
  // told, and nothing had to be added to the editor to pass it along.
  if (iEditorPort > 0 && iEditorPort < 0xFFFF)
    return static_cast<ezUInt16>(iEditorPort + 1);

  return 0;
}

void ezMcpEngineHost::Startup()
{
  const ezUInt16 uiPort = ResolvePort();

  if (uiPort == 0)
    return;

  ezGameApplicationBase* pApp = ezGameApplicationBase::GetGameApplicationBaseInstance();

  if (pApp == nullptr)
  {
    ezLog::Warning("MCP: no ezGameApplicationBase, so there is no frame loop to answer requests from. Not starting the server.");
    return;
  }

  s_pServer = EZ_DEFAULT_NEW(ezMcpServer, "ezEngine");

  if (s_pServer->Start(uiPort).Failed())
  {
    // Most likely another process already holds the port - two games launched with the same '-mcpport',
    // or an editor whose engine process derived the same number. Logged rather than fatal: the game
    // itself is unaffected, and log_read is not reachable to explain it afterwards.
    ezLog::Error("MCP: could not listen on port {}. The game runs without an MCP server.", uiPort);

    EZ_DEFAULT_DELETE(s_pServer);
    return;
  }

  pApp->m_ExecutionEvents.AddEventHandler(&ezMcpEngineHost::ExecutionEventHandler);

  // The editor's engine process sleeps in its IPC wait until the editor sends it something, and an
  // arriving MCP request is not something the editor knows about. Without this, a call to an idle
  // engine process would simply never be answered. ezPlayer free-runs and ignores this.
  pApp->m_HasPendingExternalWork = []() -> bool
  {
    return s_pServer != nullptr && s_pServer->HasPendingRequest();
  };
}

void ezMcpEngineHost::Shutdown()
{
  if (s_pServer == nullptr)
    return;

  if (ezGameApplicationBase* pApp = ezGameApplicationBase::GetGameApplicationBaseInstance())
  {
    pApp->m_ExecutionEvents.RemoveEventHandler(&ezMcpEngineHost::ExecutionEventHandler);
    pApp->m_HasPendingExternalWork = {};
  }

  // Stop() joins the transport thread, which finishes writing whatever response it was in the middle
  // of. That is what makes it safe for app_quit to trigger shutdown one frame after being answered.
  EZ_DEFAULT_DELETE(s_pServer);
}

void ezMcpEngineHost::RequestQuit()
{
  s_bQuitRequested = true;
}

void ezMcpEngineHost::ExecutionEventHandler(const ezGameApplicationExecutionEvent& e)
{
  if (e.m_Type == ezGameApplicationExecutionEvent::Type::BeforeWorldUpdates)
  {
    // Before the world ticks and before input is gathered, so a value written by input_set is picked up
    // by the very frame the agent asked for rather than the one after it.
    s_pServer->ProcessPendingRequests();
    return;
  }

  // Counted on present rather than at the end of the tick, because 'a frame happened' has to mean 'a
  // frame was rendered'. The editor's engine process runs its loop continuously but only renders when
  // the editor asks it to, so an app tick is not evidence of anything - and a screenshot that waits for
  // a frame would then be told frames were passing while none were.
  if (e.m_Type == ezGameApplicationExecutionEvent::Type::BeforePresent)
  {
    ++s_uiFrameCount;
    return;
  }

  if (e.m_Type == ezGameApplicationExecutionEvent::Type::EndAppTick)
  {
    if (s_bQuitRequested)
    {
      // One full frame after app_quit was answered. The response left the socket at the point above,
      // and Shutdown() joins the transport thread anyway, so nothing is lost either way.
      ezApplication::GetApplicationInstance()->QuitApplication();
    }
  }
}
