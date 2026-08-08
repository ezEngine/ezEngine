#pragma once

#include <Mcp/McpTool.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Mutex.h>

/// \brief Writing to and reading back this process's log.
///
/// Reading matters more than writing: it is the cheapest feedback loop an agent has. It can trigger
/// something and then look at what the application said about it, instead of asking the user to copy
/// the log.
///
/// Host independent, hence concrete and living in the Mcp library rather than in a plugin: a ring
/// buffer over ezGlobalLog is the same thing in the editor and in a game. Each process captures its
/// own log, which is what makes it worth running a server in both - the editor cannot see what the
/// engine process logged, and vice versa.
class ezMcpLogTool : public ezMcpToolProvider
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpLogTool, ezMcpToolProvider);

public:
  ezMcpLogTool();
  ~ezMcpLogTool();

  virtual void OnActivate() override;
  virtual void OnDeactivate() override;

  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;

private:
  struct Entry
  {
    ezUInt64 m_uiId = 0;
    ezLogMsgType::Enum m_Type = ezLogMsgType::None;
    ezString m_sText;
  };

  void LogEventHandler(const ezLoggingEventData& e);

  void ExecuteWrite(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteRead(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);

  /// How many messages are kept. The editor logs a lot, and a game logs every frame, so this is a
  /// compromise between covering a whole operation and not holding on to megabytes of text.
  static constexpr ezUInt32 s_uiMaxEntries = 2000;

  /// Log events arrive from whatever thread produced them, everything else runs on the main thread.
  mutable ezMutex m_Mutex;
  ezDeque<Entry> m_Entries;
  ezEventSubscriptionID m_LogSubscription = 0;

  /// Ids are assigned in order and never reused, even once their entry falls out of m_Entries, so a
  /// 'sinceId' from an earlier read remains meaningful no matter how much has been logged since.
  ezUInt64 m_uiNextId = 1;
};
