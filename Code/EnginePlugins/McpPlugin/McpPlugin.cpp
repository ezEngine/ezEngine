#include <McpPlugin/McpPluginPCH.h>

#include <McpPlugin/McpEngineHost.h>

#include <Mcp/McpToolRegistry.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>

EZ_PLUGIN_ON_LOADED()
{
  // Created here rather than when the server starts, so that the log tool's hook is in place before the
  // rest of engine startup runs. Otherwise log_read would come up empty for exactly the part of a
  // session that is worth reading - loading the project, the plugins and the scene.
  ezMcpToolRegistry::UpdateProviders();
}

EZ_PLUGIN_ON_UNLOADED()
{
  ezMcpToolRegistry::Clear();
}

// The registry's execute wrapper is deliberately left unset. It is what the editor uses to force a call
// into unattended mode and to turn a failed assert into a tool error; a game process has neither dialogs
// nor an assert handler that keeps running, so calls go straight through.

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Mcp, McpPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    // Not at plugin load: the server needs the frame loop to answer requests from, and
    // ezGameApplicationBase does not exist yet at that point.
    ezMcpEngineHost::Startup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezMcpEngineHost::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

EZ_STATICLINK_FILE(McpPlugin, McpPlugin_McpPlugin);
