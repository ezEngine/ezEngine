#pragma once

#include <Mcp/McpTool.h>

class ezMcpJsonWriter;

/// Tools that act on the process itself, rather than on whatever it has loaded.
///
/// These exist to close the automation loop: an agent that can launch a process (with '-mcpport' to get
/// its own port), query it and then shut it down again can test a change without a human in between.
///
/// **Abstract on purpose.** Almost all of this is process level rather than editor or game level - the
/// process id, the executable path, the command line, the bound port - so the editor and a running game
/// answer the same four questions and should not make an agent learn two names for each. What differs
/// is host specific and reachable through the virtuals below. ezMcpToolRegistry only instantiates
/// providers whose RTTI can allocate, so declaring this one with ezRTTINoAllocator is what keeps the
/// base from registering its tool names alongside the concrete host's.
class EZ_MCP_DLL ezMcpAppTool : public ezMcpToolProvider
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpAppTool, ezMcpToolProvider);

public:
  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;

protected:
  /// What this process is, e.g. "editor" or "game". Used in the tool descriptions, so that an
  /// agent talking to two servers at once can tell which one it is addressing.
  virtual ezStringView GetHostNoun() const = 0;

  /// How to start another process like this one, appended to app_quit's description.
  ///
  /// Everything an agent needs has to be in the tool list - it has no repo access and no prior session -
  /// so this is where 'which executable, which arguments' gets said.
  virtual ezStringView GetRelaunchHint() const = 0;

  /// When the binary serving MCP was compiled, reported by app_info as 'buildTimestamp'.
  ///
  /// Exists because a stale binary is indistinguishable from a missing feature: an agent working from a
  /// `tools/list` produced by a build from a few days ago reports tools as absent, and nothing in the
  /// protocol reveals the mismatch. Comparing this against when the source was last changed does.
  ///
  /// The default is when the Mcp library itself was built. A host whose own plugin changes more often
  /// than the library should override this with its own __DATE__ " " __TIME__ - the tool list comes from
  /// the plugin, so that is the binary whose age actually explains a missing tool.
  virtual ezStringView GetBuildTimestamp() const;

  /// Anything host specific that app_info should report, added to the same object.
  virtual void AddHostInfo(ezMcpJsonWriter& ref_writer) {}

  /// Whether quitting is allowed right now. Return EZ_FAILURE to refuse.
  ///
  /// A refusal must never be a question to the user: a modal dialog with nobody at the keyboard never
  /// returns, and the process then hangs holding its port. Destructive choices are parameters, which is
  /// what \a bDiscardChanges is.
  virtual ezResult CanQuit(bool bDiscardChanges) { return EZ_SUCCESS; }

  /// Explains a refusal from CanQuit(). Writes into the result object, after "quitting": false.
  virtual void AddQuitRefusalInfo(ezMcpJsonWriter& ref_writer) {}

  /// Anything worth reporting about a quit that is going ahead, e.g. what got discarded.
  virtual void AddQuitInfo(ezMcpJsonWriter& ref_writer) {}

  /// Actually shuts the process down.
  ///
  /// Must defer the shutdown past the end of this call. The response has not reached the socket yet, so
  /// quitting synchronously drops it and leaves the caller waiting on a connection that closes with no
  /// answer - which is indistinguishable from a crash.
  virtual void RequestQuit(bool bDiscardChanges) = 0;

private:
  void ExecutePing(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteQuit(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteInfo(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteCommandLineOptions(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
};
