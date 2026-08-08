#pragma once

#include <McpPlugin/McpPluginDLL.h>

#include <Foundation/Time/Time.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Variant.h>
#include <Mcp/McpTool.h>

class ezMcpInputDevice;

/// \brief Pressing keys and moving the mouse, as the game sees it.
///
/// Owns the synthetic ezInputDevice that does the actual writing - see ezMcpInputDevice for why it is a
/// device rather than injected values. The provider's lifetime is the plugin's, so the device exists for
/// as long as there is a server to ask for input.
///
/// Deliberately shaped to grow: an agent driving a game will eventually want recorded sequences and
/// timed scripts, and those are new tool names on this provider writing to the same device, not a
/// different mechanism.
class ezMcpInputTool : public ezMcpToolProvider
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpInputTool, ezMcpToolProvider);

public:
  ezMcpInputTool();
  ~ezMcpInputTool();

  virtual void OnActivate() override;
  virtual void OnDeactivate() override;

  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;

private:
  void ExecuteSlots(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteSet(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteSequence(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);

  /// \brief Writes every slot in \a slots via m_pDevice, the same way ExecuteSet() does. Shared with
  /// ExecuteSequence()'s 'set' steps. Returns false (and has already set out_result) on the first
  /// value that is not a number.
  bool ApplySlots(const ezVariantDictionary& slots, ezUInt32 uiFrames, ezMcpToolResult& out_result);

  ezUniquePtr<ezMcpInputDevice> m_pDevice;

  /// Same reason as everywhere else: a process registers hundreds of slots and an unfiltered dump of
  /// them buries whatever the caller was looking for.
  static constexpr ezUInt32 s_uiMaxResults = 200;

  /// input_sequence cannot loop across a 'wait' step - like game_wait, it defers instead
  /// (ezMcpToolResult::m_bNotFinished) and is re-entered once per frame until the wait is over. This is
  /// what survives across those re-entries: which step is in flight, and until when.
  bool m_bSequenceActive = false;
  bool m_bSequenceWaiting = false;
  ezUInt32 m_uiSequenceStep = 0;
  ezUInt64 m_uiSequenceWaitUntilFrame = 0;
  ezTime m_SequenceWaitStarted;
  ezTime m_SequenceWaitTimeout;

  /// A caller that asks for a sequence of a million steps has made a mistake that would otherwise look
  /// exactly like a hang - same reasoning as game_wait's s_uiMaxFrames.
  static constexpr ezUInt32 s_uiMaxSteps = 200;
  static constexpr ezUInt32 s_uiMaxWaitFrames = 10000;
  static constexpr ezTime s_DefaultWaitTimeout = ezTime::MakeFromSeconds(30);
};
