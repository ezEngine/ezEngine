#pragma once

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <Core/Console/Console.h>
#  include <Core/Input/DeviceTypes/MouseKeyboard.h>
#  include <GameEngine/GameEngineDLL.h>

struct ezLoggingEventData;

/// \brief An ImGui-based console for in-game display of logs, configuration of ezCVars, and more.
///
/// The console displays the recent log activity, allows to modify CVars and call console functions.
/// It supports auto-completion and provides a GUI using ImGui.
/// The default implementation uses ezConsoleInterpreter::Lua as the interpreter for commands typed into it.
/// The interpreter can be replaced with custom implementations.
class EZ_GAMEENGINE_DLL ezImGuiConsole final : public ezConsole
{
public:
  ezImGuiConsole();
  ~ezImGuiConsole();

  /// \name Configuration
  /// @{

  /// \brief Adjusts how many strings the console will keep in memory at maximum.
  void SetMaxConsoleStrings(ezUInt32 uiMax) { m_uiMaxConsoleStrings = ezMath::Clamp<ezUInt32>(uiMax, 0, 100000); }

  /// \brief Returns how many strings the console will keep in memory at maximum.
  ezUInt32 GetMaxConsoleStrings() const { return m_uiMaxConsoleStrings; }

  /// \brief Enables or disables that the output from ezGlobalLog is displayed in the console. Enabled by default.
  void EnableLogOutput(bool bEnable);

  /// \brief Writes the state of the console (history, bound keys) to the stream.
  virtual void SaveState(ezStreamWriter& inout_stream) const;

  /// \brief Reads the state of the console (history, bound keys) from the stream.
  virtual void LoadState(ezStreamReader& inout_stream);

  /// @}
  /// \name Console Content
  /// @{

  /// \brief Adds a string to the console.
  virtual void AddConsoleString(ezStringView sText, ezConsoleString::Type type = ezConsoleString::Type::Default) override;

  /// @}
  /// \name Updates
  /// @{

  /// \brief Display the console state.
  virtual void RenderConsole(bool bIsOpen) override;

  /// \brief Update the console with the latest input.
  virtual void HandleInput(bool bIsOpen) override;

  /// @}

protected:
  struct CVarTreeNode
  {
    ezString m_sName;
    ezCVar* m_pCVar = nullptr; // nullptr for parent nodes, valid for leaf nodes
    ezMap<ezString, CVarTreeNode> m_Children;
    bool m_bExpanded = false;
  };

  void LogHandler(const ezLoggingEventData& data);
  void ClearLogStrings();
  void RenderCommandWindow(bool bSetFocus);
  void RenderCVarWindow();
  void RenderStatsWindow(bool bFull);
  void RenderLogWindow(bool bFull);
  void HandleAutoComplete();
  int InputTextCallback(struct ImGuiInputTextCallbackData* data);
  void BuildCVarTree(CVarTreeNode& root);
  void RenderCVarTreeNode(const ezString& sNodeName, CVarTreeNode& node);
  void RenderCVarValue(ezCVar* pCVar);
  void BuildFilteredLogStrings();
  bool FilterLogString(const ezConsoleString& entry) const;
  ezUInt64 CalculateTotalMemoryUsage();
  void UpdateFrameTimes();
  void UpdateMemoryUsage();

  ezDeque<ezConsoleString> m_LogStrings;           // For log messages (used by log window)
  ezDeque<ezConsoleString> m_CommandOutputStrings; // For command input/output (used by console window)
  ezDeque<ezConsoleString> m_FilteredLogStrings;
  ezUInt32 m_uiMaxConsoleStrings = 1000;
  ezUInt32 m_uiMaxCommandOutputStrings = 1000;
  bool m_bLogOutputEnabled = false;
  bool m_bScrollLogToBottom = false;
  bool m_bScrollCommandsToBottom = false; // For console window auto-scroll

  bool m_bPinStatsWindow = false;
  bool m_bPinLogWindow = false;
  bool m_bStatsWindowOpen = true;
  bool m_bLogWindowOpen = true;
  bool m_bCVarWindowOpen = true;

  // Log filtering
  ezStringBuilder m_sLogFilter;
  ezStringBuilder m_sCommandText;
  bool m_bFilterLog = false;
  bool m_bLogFilterChanged = false;
  ezLogMsgType::Enum m_LogLevel = ezLogMsgType::DebugMsg;

  // Input handling
  ezUInt8 m_uiForceFocus = 3;
  bool m_bWasOpen = false;
  bool m_bCursorWasVisible = true;
  bool m_bDefaultInputHandlingInitialized = false;
  bool m_bExecutingCommand = false;
  ezUInt8 m_uiResetLayout = 0;
  ezMouseCursorClipMode::Enum m_MouseWasClipped;

  // Frame time tracking for statistics
  float m_FrameTimeHistory[30 * 10];
  ezUInt32 m_uiFrameTimeHistoryIndex = 0;
  float m_fCurrentBinMaxFrameTime = 0.0f;
  ezTime m_LastBinTime;
  bool m_bTimeTrackingVisible = true;

  // Memory tracking for statistics
  float m_MemoryHistory[30 * 10]; // Memory usage in MB
  ezUInt32 m_uiMemoryHistoryIndex = 0;
  float m_fCurrentBinMaxMemory = 0.0f;
  ezTime m_LastMemoryBinTime;
  bool m_bMemoryTrackingVisible = true;

  // Stats window size tracking for overlay/full mode switching
  ezVec2 m_vStatsWindowSavedSize = ezVec2(0, 0);
  bool m_bStatsWasInFullMode = true;
};

#endif // BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
