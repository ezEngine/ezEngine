#include <EditorPluginMcp/EditorPluginMcpPCH.h>

#include <Mcp/McpServer.h>
#include <Mcp/McpToolRegistry.h>

#include <Foundation/Utilities/CommandLineOptions.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

/// The port to listen on.
///
/// Exists so that several editors can run at the same time, each answering on its own port: with a
/// single fixed port the second editor just fails to bind. That matters for automated testing, where
/// one editor is launched per project and has to be addressable independently of any other.
///
/// Spelled '-editor-mcpport' rather than '-mcpport' because the editor forwards its whole command line
/// to the engine process it starts, which serves MCP as well. One name for both would have meant both
/// processes reading the same number and fighting over the port; two names mean the engine process can
/// derive its own from this one without anything being passed explicitly.
///
/// The default is what a client connects to when nothing else was agreed on, so changing it means
/// changing the documented URL as well (see the 'ez-mcp' skill). Ports below 1024 are excluded
/// because they need elevation on most systems, and 0 because 'any free port' cannot be put into a
/// client's URL.
static ezCommandLineOptionInt s_opt_McpPort("_Mcp", "-editor-mcpport",
  "The port that the editor's MCP server listens on, on 127.0.0.1.\n"
  "Pass a distinct port per editor to run several at the same time.\n"
  "The engine process that runs the game serves MCP on this port + 1, unless it is given '-mcpport'.",
  7391, 1024, 0xFFFF);

static ezMcpServer* s_pServer = nullptr;
static ezEventSubscriptionID s_TickSubscription = 0;

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);
static void TickEventHandler(const ezQtUiServices::TickEvent& e);

/// \brief Runs one tool call inside the editor's unattended mode, and turns a failed assert into an error.
///
/// This is the editor's half of what ezMcpToolRegistry used to do itself. It stays here, in the plugin,
/// because none of it means anything in a game process.
static void ExecuteWrapper(ezStringView sToolName, ezMcpToolResult& ref_result, ezDelegate<void()> execute)
{
  // Only for the duration of the call: the user typically *is* sitting in front of this editor, so their
  // own menu clicks must keep opening dialogs. It is the agent's call that must not block on one - see
  // ezQtDialog. What got suppressed is reported afterwards, otherwise a suppressed dialog is
  // indistinguishable from the operation having done nothing.
  ezQtScopedUnattended unattended;
  ezQtUiServices::ClearSuppressedDialogs();
  ezQtUiServices::ClearFailedAsserts();

  execute();

  // An assert that fires during a tool call no longer stops the editor - the handler installed by
  // ezQtUiServices records it and lets execution continue - but whatever the tool produced afterwards
  // was produced by code that had already declared its own assumptions broken. Reporting it as an error
  // is the only honest answer, and it replaces what used to be a hung editor and a lost session.
  const ezArrayPtr<const ezString> asserts = ezQtUiServices::GetFailedAsserts();

  if (!asserts.IsEmpty())
  {
    ezStringBuilder sError;
    sError.SetFormat("Tool '{}' triggered {} failed assert(s). Its result is not trustworthy and the editor may now be in a broken "
                     "state - restart it before relying on anything further. This is a bug in the editor or in the tool, not "
                     "something the arguments can be changed to avoid; report it with the text below.\n",
      sToolName, asserts.GetCount());

    for (const ezString& sAssert : asserts)
    {
      sError.AppendFormat("\n{}", sAssert);
    }

    ref_result.SetError(sError);
  }
}

void OnLoadPlugin()
{
  // s_Events is static, so this works before any project has been opened
  ezToolsProject::s_Events.AddEventHandler(ToolsProjectEventHandler);

  ezMcpToolRegistry::SetExecuteWrapper(&ExecuteWrapper);

  // create the tool providers already now, rather than when the server starts, so that the log tool
  // sees everything that happens during editor startup
  ezMcpToolRegistry::UpdateProviders();
}

void OnUnloadPlugin()
{
  ezToolsProject::s_Events.RemoveEventHandler(ToolsProjectEventHandler);

  if (s_TickSubscription != 0)
  {
    ezQtUiServices::s_TickEvent.RemoveEventHandler(s_TickSubscription);
    s_TickSubscription = 0;
  }

  EZ_DEFAULT_DELETE(s_pServer);

  ezMcpToolRegistry::SetExecuteWrapper({});
  ezMcpToolRegistry::Clear();
}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

static void TickEventHandler(const ezQtUiServices::TickEvent& e)
{
  // BeforeFrame, not StartFrame: StartFrame is only broadcast when some handler asked for a frame to be
  // drawn, so an editor with nothing to redraw would never reach it and every tool call would hang.
  // BeforeFrame is driven by a plain timer and always fires. It is also outside the frame proper, which
  // is closer to where the Qt socket callback used to run a tool call than the middle of a redraw is.
  if (e.m_Type != ezQtUiServices::TickEvent::Type::BeforeFrame)
    return;

  // The transport reads requests on its own thread but never answers one there: a tool call may touch
  // any part of the editor, none of which is thread safe. This is where the answering happens, so if
  // this stops being called, clients simply wait.
  if (s_pServer != nullptr)
  {
    s_pServer->ProcessPendingRequests();
  }
}

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  // the server is tied to the open project, because everything it will eventually expose is
  // project specific

  if (e.m_Type == ezToolsProjectEvent::Type::ProjectOpened)
  {
    if (s_pServer == nullptr)
    {
      s_pServer = EZ_DEFAULT_NEW(ezMcpServer, "ezEditor");
    }

    const ezUInt16 uiPort = static_cast<ezUInt16>(s_opt_McpPort.GetOptionValue(ezCommandLineOption::LogMode::FirstTimeIfSpecified));

    if (s_pServer->Start(uiPort).Succeeded())
    {
      if (s_TickSubscription == 0)
      {
        s_TickSubscription = ezQtUiServices::s_TickEvent.AddEventHandler(&TickEventHandler);
      }
    }
    else
    {
      // Said out loud because there is nothing else to notice it by: without a server there is no
      // log_read either, so an agent that cannot connect has no way to find out why. The usual cause is
      // another editor already holding the port.
      ezLog::Error("MCP: The server could not listen on port {}. This editor is not reachable through MCP. Another editor may already "
                   "be using that port - pass a different '-editor-mcpport'.",
        uiPort);
    }
  }

  if (e.m_Type == ezToolsProjectEvent::Type::ProjectClosing)
  {
    if (s_pServer != nullptr)
    {
      s_pServer->Stop();
    }
  }
}
