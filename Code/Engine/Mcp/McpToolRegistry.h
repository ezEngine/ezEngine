#pragma once

#include <Mcp/McpTool.h>

#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Types/Delegate.h>

/// Wraps every tool call in whatever setup and teardown the host needs.
///
/// The registry itself is host independent, but the editor has to put a call into unattended mode and
/// turn a failed assert into a tool error, and a game has no equivalent of either. So the host installs
/// this instead of the registry knowing about any of it.
///
/// \param execute Runs the tool. Call it exactly once - skipping it answers the client with an empty
///        result, calling it twice runs the tool twice.
using ezMcpExecuteWrapper = ezDelegate<void(ezStringView sToolName, ezMcpToolResult& ref_result, ezDelegate<void()> execute)>;

/// Owns one instance of every ezMcpToolProvider and routes calls to them.
///
/// The registry is filled through reflection, so a provider in another plugin needs no registration
/// call - it only has to exist.
class EZ_MCP_DLL ezMcpToolRegistry
{
public:
  /// Instantiates every provider type that isn't known yet.
  ///
  /// Idempotent, and called again when the server starts, because plugins may be loaded after the one
  /// that owns the server and would otherwise never be picked up.
  ///
  /// Provider types that cannot be allocated are skipped, which is what makes an abstract provider base
  /// usable: a tool that exists in several hosts declares the shared half with ezRTTINoAllocator and
  /// each host derives the concrete type that is actually instantiated here.
  static void UpdateProviders();

  /// Destroys all providers. Called when the plugin is unloaded.
  static void Clear();

  /// Destroys the provider of the given type, along with its tools.
  ///
  /// A provider may live in a different plugin than the registry, and plugins are not unloaded in any
  /// particular order. Once its plugin is gone, the provider's vtable and RTTI allocator point into an
  /// unmapped module, so a plugin that declares a provider has to call this from its unload function,
  /// while its own code is still mapped. Does nothing if no such provider exists, so it is safe to call
  /// when the registry was never filled. UpdateProviders() instantiates the type again if the plugin is
  /// loaded once more.
  static void RemoveProvider(const ezRTTI* pProviderType);

  /// All tools of all providers, in registration order.
  static const ezDynamicArray<ezMcpToolDesc>& GetTools() { return s_Tools; }

  /// Installs the host's wrapper around every tool call. Pass an invalid delegate to remove it.
  static void SetExecuteWrapper(ezMcpExecuteWrapper wrapper) { s_ExecuteWrapper = wrapper; }

  /// Runs a tool. Fails only if no tool of that name exists - a tool that ran but didn't like
  /// its arguments reports that through out_result instead.
  static ezResult Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result);

private:
  static ezSet<const ezRTTI*> s_KnownTypes;
  static ezDynamicArray<ezMcpToolProvider*> s_Providers;
  static ezDynamicArray<ezMcpToolDesc> s_Tools;
  static ezMap<ezString, ezMcpToolProvider*> s_ToolLookup;
  static ezMcpExecuteWrapper s_ExecuteWrapper;
};
