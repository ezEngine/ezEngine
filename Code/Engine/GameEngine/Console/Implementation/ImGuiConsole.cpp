#include <GameEngine/GameEnginePCH.h>

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <GameEngine/Console/ImGuiConsole.h>

#  include <Core/Input/DeviceTypes/MouseKeyboard.h>
#  include <Core/Input/InputManager.h>
#  include <Foundation/Configuration/CVar.h>
#  include <Foundation/IO/Stream.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Math/Math.h>
#  include <Foundation/Memory/MemoryTracker.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/Time/Clock.h>
#  include <Foundation/Time/Time.h>
#  include <GameEngine/Console/LuaInterpreter.h>
#  include <GameEngine/DearImgui/DearImgui.h>
#  include <RendererCore/Pipeline/View.h>
#  include <RendererCore/RenderWorld/RenderWorld.h>

ezImGuiConsole::ezImGuiConsole()
{
  ezMemoryUtils::ZeroFillArray(m_FrameTimeHistory);
  m_fCurrentBinMaxFrameTime = 0.0f;
  m_LastBinTime = ezTime::Now();

  ezMemoryUtils::ZeroFillArray(m_MemoryHistory);
  m_fCurrentBinMaxMemory = 0.0f;
  m_LastMemoryBinTime = ezTime::Now();

#  ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT
  SetCommandInterpreter(EZ_DEFAULT_NEW(ezCommandInterpreterLua));
#  endif

  EnableLogOutput(true);
}

ezImGuiConsole::~ezImGuiConsole()
{
  EnableLogOutput(false);
}

void ezImGuiConsole::EnableLogOutput(bool bEnable)
{
  if (m_bLogOutputEnabled == bEnable)
    return;

  m_bLogOutputEnabled = bEnable;

  if (bEnable)
  {
    ezGlobalLog::AddLogWriter(ezMakeDelegate(&ezImGuiConsole::LogHandler, this));
  }
  else
  {
    ezGlobalLog::RemoveLogWriter(ezMakeDelegate(&ezImGuiConsole::LogHandler, this));
  }
}

void ezImGuiConsole::SaveState(ezStreamWriter& inout_stream) const
{
  EZ_LOCK(m_Mutex);

  const ezUInt8 uiVersion = 100;
  inout_stream << uiVersion;

  inout_stream << m_vStatsWindowSavedSize;
}

void ezImGuiConsole::LoadState(ezStreamReader& inout_stream)
{
  EZ_LOCK(m_Mutex);

  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  // to prevent conflicts with the QuakeConsole
  if (uiVersion < 100 || uiVersion > 199)
    return;

  inout_stream >> m_vStatsWindowSavedSize;
}

void ezImGuiConsole::AddConsoleString(ezStringView sText, ezConsoleString::Type type)
{
  EZ_LOCK(GetMutex());

  // Call base class to trigger events
  ezConsole::AddConsoleString(sText, type);

  // Determine if this is command output or log message
  const bool bIsCommandOutput = (type == ezConsoleString::Type::Executed ||
                                 type == ezConsoleString::Type::VarName ||
                                 type == ezConsoleString::Type::FuncName ||
                                 type == ezConsoleString::Type::Note);

  if (m_bExecutingCommand || bIsCommandOutput)
  {
    // Store in command output for console window
    ezConsoleString& cs = m_CommandOutputStrings.ExpandAndGetRef();
    cs.m_Type = type;
    cs.m_sText = sText;

    // Limit the number of stored command output strings
    while (m_CommandOutputStrings.GetCount() > m_uiMaxCommandOutputStrings)
    {
      m_CommandOutputStrings.PopFront();
    }

    // Auto-scroll console window to bottom when new command output is added
    m_bScrollCommandsToBottom = true;
  }
}

void ezImGuiConsole::ClearLogStrings()
{
  EZ_LOCK(GetMutex());
  m_LogStrings.Clear();
}

void ezImGuiConsole::LogHandler(const ezLoggingEventData& data)
{
  ezConsoleString::Type type = ezConsoleString::Type::Default;

  switch (data.m_EventType)
  {
    case ezLogMsgType::GlobalDefault:
    case ezLogMsgType::Flush:
    case ezLogMsgType::BeginGroup:
    case ezLogMsgType::EndGroup:
    case ezLogMsgType::None:
    case ezLogMsgType::ENUM_COUNT:
    case ezLogMsgType::All:
      return;

    case ezLogMsgType::ErrorMsg:
      type = ezConsoleString::Type::Error;
      break;

    case ezLogMsgType::SeriousWarningMsg:
      type = ezConsoleString::Type::SeriousWarning;
      break;

    case ezLogMsgType::WarningMsg:
      type = ezConsoleString::Type::Warning;
      break;

    case ezLogMsgType::SuccessMsg:
      type = ezConsoleString::Type::Success;
      break;

    case ezLogMsgType::InfoMsg:
      break;

    case ezLogMsgType::DevMsg:
      type = ezConsoleString::Type::Dev;
      break;

    case ezLogMsgType::DebugMsg:
      type = ezConsoleString::Type::Debug;
      break;
  }

  ezStringBuilder sFormat;
  sFormat.SetPrintf("%*s", data.m_uiIndentation, "");
  sFormat.Append(data.m_sText);

  // Add to log messages (not command output)
  EZ_LOCK(GetMutex());
  ezConsoleString& cs = m_LogStrings.ExpandAndGetRef();
  cs.m_Type = type;
  cs.m_sText = sFormat;

  // Limit the number of stored log strings
  while (m_LogStrings.GetCount() > m_uiMaxConsoleStrings)
  {
    m_LogStrings.PopFront();
  }

  while (m_FilteredLogStrings.GetCount() > m_uiMaxConsoleStrings)
  {
    m_FilteredLogStrings.PopFront();
  }

  if (m_bFilterLog)
  {
    if (!FilterLogString(cs))
    {
      m_FilteredLogStrings.PushBack(cs);
      m_bScrollLogToBottom = true;
    }
  }
  else
  {
    m_bScrollLogToBottom = true;
  }
}

void ezImGuiConsole::RenderCommandWindow(bool bSetFocus)
{
  // Get available screen space
  ImGuiIO& io = ImGui::GetIO();
  const float screenWidth = io.DisplaySize.x;
  const float screenHeight = io.DisplaySize.y;

  // Command window: top center between stats and log, 1/5th of screen height
  const float statsWidth = screenWidth * 0.25f;                           // Stats takes 1/4 width
  const float logWidth = screenWidth * 0.33f;                             // Log takes 1/3 width
  const float commandWidth = screenWidth - statsWidth - logWidth - 20.0f; // Remaining width with some padding
  const float commandHeight = screenHeight * 0.2f;                        // 1/5th of screen height
  const float commandPosX = statsWidth + 10.0f;                           // Position after stats with padding
  const float commandPosY = 10.0f;                                        // Top of screen

  ImGui::SetNextWindowSize(ImVec2(commandWidth, commandHeight), (m_uiResetLayout > 0) ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowPos(ImVec2(commandPosX, commandPosY), (m_uiResetLayout > 0) ? ImGuiCond_Always : ImGuiCond_FirstUseEver);

  if (!ImGui::Begin("Commands", nullptr, ImGuiWindowFlags_MenuBar))
  {
    ImGui::End();
    return;
  }

  // Menu bar with Reset Layout button
  if (ImGui::BeginMenuBar())
  {
    if (ImGui::BeginMenu("Windows"))
    {
      ImGui::MenuItem("Stats", nullptr, &m_bStatsWindowOpen);
      ImGui::MenuItem("Log", nullptr, &m_bLogWindowOpen);
      ImGui::MenuItem("CVars", nullptr, &m_bCVarWindowOpen);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Layout"))
    {
      if (ImGui::MenuItem("Reset Layout"))
      {
        m_uiResetLayout = 3;
        m_bStatsWindowOpen = true;
        m_bLogWindowOpen = true;
        m_bCVarWindowOpen = true;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  // Reserve space for input at the bottom
  const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
  if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar))
  {
    // Display command output strings
    EZ_LOCK(GetMutex());

    for (const auto& consoleString : m_CommandOutputStrings)
    {
      // Set color based on message type
      ezColor color = consoleString.GetColor();
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color.r, color.g, color.b, color.a));

      ImGui::TextUnformatted(consoleString.m_sText.GetData());

      ImGui::PopStyleColor();
    }

    // Auto-scroll to bottom when new command output is added
    if (m_bScrollCommandsToBottom)
    {
      ImGui::SetScrollHereY(1.0f);
      m_bScrollCommandsToBottom = false;
    }
  }
  ImGui::EndChild();

  // Input field
  ImGui::Separator();

  if (bSetFocus)
  {
    // Auto-focus on window appearance or restore focus after ESC
    ImGui::SetKeyboardFocusHere();
  }

  char buffer[128];
  ezStringUtils::Copy(buffer, EZ_ARRAY_SIZE(buffer), m_sCommandText);

  // Make input field use full available width
  ImGui::SetNextItemWidth(-1.0f);

  if (ImGui::InputTextWithHint("##Input", "> TAB to auto-complete", buffer, EZ_ARRAY_SIZE(buffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackEdit, [](ImGuiInputTextCallbackData* data) -> int
        {
          ezImGuiConsole* console = static_cast<ezImGuiConsole*>(data->UserData);
          return console->InputTextCallback(data);
          //
        },
        this))
  {
    AddToInputHistory(buffer);
    m_bExecutingCommand = true;
    ExecuteCommand(buffer);
    m_bExecutingCommand = false;
    buffer[0] = '\0';
  }

  if (!m_sCommandText.IsEmpty() && ezStringUtils::IsNullOrEmpty(buffer))
  {
    // in case of input or ESC, make sure the focus is returned to the command input field
    m_uiForceFocus = 3;
  }

  m_sCommandText = buffer;

  // Keep focus on input field
  ImGui::SetItemDefaultFocus();

  ImGui::End();
}

void ezImGuiConsole::RenderCVarWindow()
{
  // Get available screen space
  ImGuiIO& io = ImGui::GetIO();
  const float screenWidth = io.DisplaySize.x;
  const float screenHeight = io.DisplaySize.y;

  // CVar window: bottom right, same size as log window (1/3 width, 1/2 height)
  const float cvarWidth = screenWidth * 0.33f;
  const float cvarHeight = screenHeight * 0.5f;
  const float cvarPosX = screenWidth - cvarWidth - 10.0f;   // Right side with padding
  const float cvarPosY = screenHeight - cvarHeight - 10.0f; // Bottom with padding

  ImGui::SetNextWindowSize(ImVec2(cvarWidth, cvarHeight), (m_uiResetLayout > 0) ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowPos(ImVec2(cvarPosX, cvarPosY), (m_uiResetLayout > 0) ? ImGuiCond_Always : ImGuiCond_FirstUseEver);

  if (!ImGui::Begin("CVars", &m_bCVarWindowOpen))
  {
    ImGui::End();
    return;
  }

  // Build tree structure from CVars
  CVarTreeNode rootNode;
  BuildCVarTree(rootNode);

  // Create a table with tree view in the first column
  if (ImGui::BeginTable("CVarTreeTable", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY))
  {
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 300.0f);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 150.0f);
    ImGui::TableSetupColumn("Description");
    ImGui::TableHeadersRow();

    // Render the tree
    for (auto& child : rootNode.m_Children)
    {
      RenderCVarTreeNode(child.Key(), child.Value());
    }

    ImGui::EndTable();
  }

  ImGui::End();
}

void ezImGuiConsole::RenderStatsWindow(bool bFull)
{
  // Get available screen space
  ImGuiIO& io = ImGui::GetIO();
  const float screenWidth = io.DisplaySize.x;
  const float screenHeight = io.DisplaySize.y;

  // Stats window: top left, 1/4 width, auto height
  const float statsWidth = screenWidth * 0.25f;
  const float statsPosX = 10.0f; // Left side with padding
  const float statsPosY = 10.0f; // Top with padding

  // Set initial size with width preference, allow manual resizing
  if (m_uiResetLayout > 0)
  {
    ImGui::SetNextWindowSize(ImVec2(statsWidth, -1), ImGuiCond_Always);
  }
  else
  {
    ImGui::SetNextWindowSize(ImVec2(statsWidth, -1), ImGuiCond_FirstUseEver);
  }

  ImGui::SetNextWindowPos(ImVec2(statsPosX, statsPosY), (m_uiResetLayout > 0) ? ImGuiCond_Always : ImGuiCond_FirstUseEver);

  ImGuiWindowFlags flags = 0;
  if (!bFull)
  {
    flags |= ImGuiWindowFlags_NoDecoration;
    flags |= ImGuiWindowFlags_NoInputs;
    flags |= ImGuiWindowFlags_AlwaysAutoResize; // Auto-resize when not in full mode
  }
  else
  {
    // When switching from overlay to full mode, restore saved size
    if (!m_bStatsWasInFullMode && m_vStatsWindowSavedSize.x > 0 && m_vStatsWindowSavedSize.y > 0)
    {
      ImGui::SetNextWindowSize(ImVec2(m_vStatsWindowSavedSize.x, m_vStatsWindowSavedSize.y), ImGuiCond_Always);
    }
  }

  if (!ImGui::Begin("Stats", &m_bStatsWindowOpen, flags))
  {
    ImGui::End();
    return;
  }

  // Track mode changes and save window size when in full mode
  if (bFull && !m_bStatsWasInFullMode)
  {
    // Just switched to full mode - size was already restored above if needed
  }
  else if (bFull)
  {
    // Continuously save size while in full mode
    m_vStatsWindowSavedSize.x = ImGui::GetWindowSize().x;
    m_vStatsWindowSavedSize.y = ImGui::GetWindowSize().y;
  }

  m_bStatsWasInFullMode = bFull;

  if (bFull)
  {
    ImGui::Checkbox("Pin Window", &m_bPinStatsWindow);
  }

  ezStringBuilder tmp;

  // Basic stats
  const float currentFrameTime = ezClock::GetGlobalClock()->GetTimeDiff().AsFloatInSeconds();
  ImGui::Text("FPS: %.0f (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f * currentFrameTime);

  // Memory stats
  {
    const ezUInt64 totalMemoryBytes = CalculateTotalMemoryUsage();

    tmp.SetFormat("Memory: {}", ezArgFileSize(totalMemoryBytes));
    ImGui::TextUnformatted(tmp);
  }

  // Collapsible frame time plot
  if (bFull || m_bTimeTrackingVisible)
  {
    m_bTimeTrackingVisible = ImGui::CollapsingHeader("Frame Times##group_time", ImGuiTreeNodeFlags_DefaultOpen);
    if (m_bTimeTrackingVisible)
    {
      // Calculate frame time statistics
      float maxFrameTime = m_FrameTimeHistory[0];
      for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_FrameTimeHistory); ++i)
      {
        if (m_FrameTimeHistory[i] > maxFrameTime)
          maxFrameTime = m_FrameTimeHistory[i];
      }

      maxFrameTime = ezMath::Max<float>(maxFrameTime, 1.0f / 30.0f);

      tmp.SetFormat("{} ms", ezArgF(maxFrameTime * 1000.0f, 1));

      // Determine plot color based on current frame time performance
      ImU32 plotColor;
      if (currentFrameTime > 1.0f / 30.0f)      // Worse than 30 FPS
      {
        plotColor = IM_COL32(255, 0, 0, 255);   // Red
      }
      else if (currentFrameTime > 1.0f / 55.0f) // Between 30-55 FPS
      {
        plotColor = IM_COL32(255, 255, 0, 255); // Yellow
      }
      else                                      // 60+ FPS
      {
        plotColor = IM_COL32(0, 255, 0, 255);   // Green
      }

      // Apply the color and draw the plot
      ImGui::PushStyleColor(ImGuiCol_PlotLines, plotColor);

      // Use offset to show data in chronological order (oldest to newest)
      const ezUInt32 offsetIndex = (m_uiFrameTimeHistoryIndex + 1) % EZ_ARRAY_SIZE(m_FrameTimeHistory);
      const float availableWidth = ImGui::GetContentRegionAvail().x;
      ImGui::PlotLines("##FrameTime", m_FrameTimeHistory, EZ_ARRAY_SIZE(m_FrameTimeHistory), offsetIndex, tmp, 0.0f, maxFrameTime, ImVec2(availableWidth, 100));

      ImGui::PopStyleColor(); // Restore previous color
    }
  }

  // Collapsible memory usage plot
  if (bFull || m_bMemoryTrackingVisible)
  {
    m_bMemoryTrackingVisible = ImGui::CollapsingHeader("Memory Usage##group_memory", ImGuiTreeNodeFlags_DefaultOpen);
    if (m_bMemoryTrackingVisible)
    {
      // Calculate memory usage statistics
      float maxMemory = m_MemoryHistory[0];
      for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_MemoryHistory); ++i)
      {
        if (m_MemoryHistory[i] > maxMemory)
          maxMemory = m_MemoryHistory[i];
      }

      maxMemory = ezMath::Max<float>(maxMemory, 128.0f);
      maxMemory = (float)ezMath::PowerOfTwo_Ceil(static_cast<ezUInt32>(maxMemory));

      tmp.SetFormat("{} MB", maxMemory);

      // Use offset to show data in chronological order (oldest to newest)
      const ezUInt32 memoryOffsetIndex = (m_uiMemoryHistoryIndex + 1) % EZ_ARRAY_SIZE(m_MemoryHistory);
      const float availableWidth = ImGui::GetContentRegionAvail().x;
      ImGui::PlotLines("##MemoryUsage", m_MemoryHistory, EZ_ARRAY_SIZE(m_MemoryHistory), memoryOffsetIndex, tmp, 0.0f, maxMemory, ImVec2(availableWidth, 100));
    }
  }

  // Before ending the window, ensure that if we're in overlay mode but have a saved full-mode size,
  // we temporarily set the window size to the saved size so ImGui serializes the correct size
  if (!bFull && m_vStatsWindowSavedSize.x > 0 && m_vStatsWindowSavedSize.y > 0)
  {
    ImGui::SetWindowSize(ImVec2(m_vStatsWindowSavedSize.x, m_vStatsWindowSavedSize.y));
  }

  ImGui::End();
}

void ezImGuiConsole::HandleAutoComplete()
{
  // Get current input text
  ezStringBuilder currentInput = m_sCommandText;
  currentInput.Trim();

  if (AutoComplete(currentInput))
  {
    // If auto-completion changed the text, update the input buffer
    m_sCommandText = currentInput;
  }
}

int ezImGuiConsole::InputTextCallback(ImGuiInputTextCallbackData* data)
{
  switch (data->EventFlag)
  {
    case ImGuiInputTextFlags_CallbackHistory:
    {
      // Handle command history (up/down arrows)

      ezStringBuilder sInputLine = m_sCommandText;
      if (data->EventKey == ImGuiKey_UpArrow)
      {
        RetrieveInputHistory(1, sInputLine);
      }
      else if (data->EventKey == ImGuiKey_DownArrow)
      {
        RetrieveInputHistory(-1, sInputLine);
      }

      if (sInputLine != m_sCommandText)
      {
        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, sInputLine.GetData());
      }
      break;
    }

    case ImGuiInputTextFlags_CallbackCompletion:
    {
      // Handle auto-completion (TAB key)
      ezStringBuilder currentInput = data->Buf;
      currentInput.Trim();

      // Use the base class auto-completion functionality
      // This works for both empty input (shows all commands) and partial input (completes/shows matches)
      ezStringBuilder sText = currentInput;
      if (AutoComplete(sText))
      {
        // If auto-completion changed the text, update the input buffer
        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, sText.GetData());
      }
      break;
    }
  }
  return 0;
}

void ezImGuiConsole::BuildCVarTree(CVarTreeNode& root)
{
  // Clear previous tree
  root.m_Children.Clear();

  // Iterate through all CVars and build tree structure
  for (ezCVar* pCVar = ezCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    ezStringBuilder sNameTemp;
    ezStringView sFullName = pCVar->GetName().GetData(sNameTemp);

    // Split the name by dots
    ezDeque<ezStringView> nameParts;
    sFullName.Split(false, nameParts, ".");

    // Navigate/create tree structure
    CVarTreeNode* pCurrentNode = &root;
    for (ezUInt32 i = 0; i < nameParts.GetCount(); ++i)
    {
      ezString sPartName = nameParts[i];

      if (i == nameParts.GetCount() - 1)
      {
        // Last part - this is the actual CVar
        auto& leafNode = pCurrentNode->m_Children[sPartName];
        leafNode.m_sName = sPartName;
        leafNode.m_pCVar = pCVar;
      }
      else
      {
        // Intermediate part - create parent node if needed
        auto& parentNode = pCurrentNode->m_Children[sPartName];
        parentNode.m_sName = sPartName;
        parentNode.m_pCVar = nullptr; // Parent nodes don't have CVars
        pCurrentNode = &parentNode;
      }
    }
  }
}

void ezImGuiConsole::RenderCVarTreeNode(const ezString& sNodeName, CVarTreeNode& node)
{
  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);

  if (node.m_pCVar != nullptr)
  {
    // Leaf node - render the actual CVar
    ImGui::TreeNodeEx(sNodeName.GetData(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);

    // Value column
    ImGui::TableSetColumnIndex(1);
    RenderCVarValue(node.m_pCVar);

    // Description column
    ImGui::TableSetColumnIndex(2);
    ezStringBuilder sDescTemp;
    ImGui::Text("%s", node.m_pCVar->GetDescription().GetData(sDescTemp));
  }
  else
  {
    // Parent node - render as collapsible tree node
    node.m_bExpanded = ImGui::TreeNodeEx(sNodeName.GetData(),
      node.m_bExpanded ? ImGuiTreeNodeFlags_DefaultOpen : 0);

    if (node.m_bExpanded)
    {
      // Render children
      for (auto& child : node.m_Children)
      {
        RenderCVarTreeNode(child.Key(), child.Value());
      }
      ImGui::TreePop();
    }
  }
}

ezUInt64 ezImGuiConsole::CalculateTotalMemoryUsage()
{
  ezUInt64 totalMemory = 0;

  // Iterate through all allocators and find top-level ones (those without parents)
  for (auto it = ezMemoryTracker::GetIterator(); it.IsValid(); ++it)
  {
    // Only count top-level allocators (those without a parent)
    if (it.ParentId().IsInvalidated()) // Invalid ID means no parent
    {
      const ezAllocator::Stats& stats = it.Stats();
      totalMemory += stats.m_uiAllocationSize;
    }
  }

  return totalMemory;
}

void ezImGuiConsole::UpdateFrameTimes()
{
  if (ezImgui::GetSingleton() == nullptr)
    return;

  const float currentFrameTime = ezClock::GetGlobalClock()->GetTimeDiff().AsFloatInSeconds();
  const ezTime currentTime = ezTime::Now();

  // Track the maximum frame time in the current bin
  m_fCurrentBinMaxFrameTime = ezMath::Max(m_fCurrentBinMaxFrameTime, currentFrameTime);

  // Check if we need to move to the next bin (every 1/30th of a second)
  const ezTime binDuration = ezTime::MakeFromSeconds(1.0 / 30.0);
  if (currentTime - m_LastBinTime >= binDuration)
  {
    // Move to next bin and store the maximum frame time from the previous bin
    m_uiFrameTimeHistoryIndex = (m_uiFrameTimeHistoryIndex + 1) % EZ_ARRAY_SIZE(m_FrameTimeHistory);
    m_FrameTimeHistory[m_uiFrameTimeHistoryIndex] = m_fCurrentBinMaxFrameTime;

    // Reset for next bin
    m_fCurrentBinMaxFrameTime = currentFrameTime;
    m_LastBinTime = currentTime;
  }
}

void ezImGuiConsole::UpdateMemoryUsage()
{
  const ezUInt64 currentMemoryBytes = CalculateTotalMemoryUsage();
  const float currentMemoryMB = currentMemoryBytes / (1024.0f * 1024.0f);
  const ezTime currentTime = ezTime::Now();

  // Track the maximum memory usage in the current bin
  m_fCurrentBinMaxMemory = ezMath::Max(m_fCurrentBinMaxMemory, currentMemoryMB);

  // Check if we need to move to the next bin (every 1/30th of a second)
  const ezTime binDuration = ezTime::MakeFromSeconds(1.0 / 30.0);
  if (currentTime - m_LastMemoryBinTime >= binDuration)
  {
    // Move to next bin and store the maximum memory usage from the previous bin
    m_uiMemoryHistoryIndex = (m_uiMemoryHistoryIndex + 1) % EZ_ARRAY_SIZE(m_MemoryHistory);
    m_MemoryHistory[m_uiMemoryHistoryIndex] = m_fCurrentBinMaxMemory;

    // Reset for next bin
    m_fCurrentBinMaxMemory = currentMemoryMB;
    m_LastMemoryBinTime = currentTime;
  }
}

void ezImGuiConsole::RenderCVarValue(ezCVar* pCVar)
{
  if (!pCVar)
    return;

  // Render different input widgets based on CVar type
  switch (pCVar->GetType())
  {
    case ezCVarType::Bool:
    {
      ezCVarBool* pBoolVar = static_cast<ezCVarBool*>(pCVar);
      bool value = pBoolVar->GetValue();
      ezStringBuilder sId, sNameTemp;
      sId.SetFormat("##bool_{}", pCVar->GetName().GetData(sNameTemp));
      if (ImGui::Checkbox(sId.GetData(), &value))
      {
        *pBoolVar = value;
      }
      break;
    }
    case ezCVarType::Int:
    {
      ezCVarInt* pIntVar = static_cast<ezCVarInt*>(pCVar);
      int value = pIntVar->GetValue();
      ezStringBuilder sId, sNameTemp;
      sId.SetFormat("##int_{}", pCVar->GetName().GetData(sNameTemp));
      if (ImGui::InputInt(sId.GetData(), &value))
      {
        *pIntVar = value;
      }
      break;
    }
    case ezCVarType::Float:
    {
      ezCVarFloat* pFloatVar = static_cast<ezCVarFloat*>(pCVar);
      float value = pFloatVar->GetValue();
      ezStringBuilder sId, sNameTemp;
      sId.SetFormat("##float_{}", pCVar->GetName().GetData(sNameTemp));
      if (ImGui::InputFloat(sId.GetData(), &value))
      {
        *pFloatVar = value;
      }
      break;
    }
    case ezCVarType::String:
    {
      ezCVarString* pStringVar = static_cast<ezCVarString*>(pCVar);
      ezString value = pStringVar->GetValue();
      char buffer[256];
      ezStringUtils::Copy(buffer, sizeof(buffer), value.GetData());
      ezStringBuilder sId, sNameTemp;
      sId.SetFormat("##string_{}", pCVar->GetName().GetData(sNameTemp));
      if (ImGui::InputText(sId.GetData(), buffer, sizeof(buffer)))
      {
        *pStringVar = buffer;
      }
      break;
    }

    default:
      break;
  }
}

void ezImGuiConsole::BuildFilteredLogStrings()
{
  m_bFilterLog = !m_sLogFilter.IsEmpty() || (m_LogLevel < ezLogMsgType::DebugMsg);

  if (!m_bFilterLog)
    return;

  if (m_bLogFilterChanged)
  {
    m_bLogFilterChanged = false;
    m_FilteredLogStrings.Clear();

    for (const auto& entry : m_LogStrings)
    {
      if (FilterLogString(entry))
        continue;

      m_FilteredLogStrings.PushBack(entry);
    }
  }
}

static ezLogMsgType::Enum TypeToSeverity(ezConsoleString::Type type)
{
  switch (type)
  {
    case ezConsoleString::Type::Error:
      return ezLogMsgType::ErrorMsg;

    case ezConsoleString::Type::SeriousWarning:
      return ezLogMsgType::SeriousWarningMsg;

    case ezConsoleString::Type::Warning:
      return ezLogMsgType::WarningMsg;

    case ezConsoleString::Type::Success:
      return ezLogMsgType::SuccessMsg;

    case ezConsoleString::Type::Note:
    case ezConsoleString::Type::Default:
      return ezLogMsgType::InfoMsg;

    case ezConsoleString::Type::Dev:
      return ezLogMsgType::DevMsg;

    case ezConsoleString::Type::Debug:
      return ezLogMsgType::DebugMsg;

    default:
      return ezLogMsgType::InfoMsg;
  }
}

bool ezImGuiConsole::FilterLogString(const ezConsoleString& entry) const
{
  if (TypeToSeverity(entry.m_Type) > m_LogLevel)
    return true;

  if (!m_sLogFilter.IsEmpty() && entry.m_sText.FindSubString_NoCase(m_sLogFilter) == nullptr)
    return true;

  return false;
}

void ezImGuiConsole::RenderLogWindow(bool bFull)
{
  // Get available screen space
  ImGuiIO& io = ImGui::GetIO();
  const float screenWidth = io.DisplaySize.x;
  const float screenHeight = io.DisplaySize.y;

  // Log window: top right, 1/3 width, height adjusted to avoid overlap
  const float logWidth = screenWidth * 0.33f;
  const float buttonHeight = ImGui::GetFrameHeightWithSpacing();
  const float logHeight = screenHeight * 0.5f - buttonHeight; // Reduced height to account for command window
  const float logPosX = screenWidth - logWidth - 10.0f;       // Right side with padding
  const float logPosY = 10.0f;                                // Top with padding (original position)

  ImGui::SetNextWindowSize(ImVec2(logWidth, logHeight), (m_uiResetLayout > 0) ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowPos(ImVec2(logPosX, logPosY), (m_uiResetLayout > 0) ? ImGuiCond_Always : ImGuiCond_FirstUseEver);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
  if (!bFull)
  {
    flags |= ImGuiWindowFlags_NoDecoration;
    flags |= ImGuiWindowFlags_NoInputs;
  }

  if (!ImGui::Begin("Log", &m_bLogWindowOpen, flags))
  {
    ImGui::End();
    return;
  }

  if (bFull)
  {
    ImGui::Checkbox("Pin Window", &m_bPinLogWindow);

    // Filter controls
    ImGui::Text("Filter:");
    ImGui::SameLine();

    char buffer[128];
    ezStringUtils::Copy(buffer, EZ_ARRAY_SIZE(buffer), m_sLogFilter);

    if (ImGui::InputText("##filter", buffer, EZ_ARRAY_SIZE(buffer)))
    {
      m_sLogFilter = buffer;
      m_bLogFilterChanged = true;
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(m_sLogFilter.IsEmpty());
    if (ImGui::Button("Clear Filter"))
    {
      m_sLogFilter.Clear();
      m_bLogFilterChanged = true;
    }
    ImGui::EndDisabled();

    // Message severity dropdown
    {
      ImGui::SameLine();
      const char* severityLabels[] = {"Error", "Serious Warning", "Warning", "Success", "Info", "Dev", "All"};
      const ezLogMsgType::Enum severityValues[] = {
        ezLogMsgType::ErrorMsg,
        ezLogMsgType::SeriousWarningMsg,
        ezLogMsgType::WarningMsg,
        ezLogMsgType::SuccessMsg,
        ezLogMsgType::InfoMsg,
        ezLogMsgType::DevMsg,
        ezLogMsgType::DebugMsg};

      // Find current selection index
      int currentSelection = 6; // Default to Debug (show all)
      for (int i = 0; i < 7; ++i)
      {
        if (severityValues[i] == m_LogLevel)
        {
          currentSelection = i;
          break;
        }
      }

      if (ImGui::Combo("##severity", &currentSelection, severityLabels, 7))
      {
        m_LogLevel = severityValues[currentSelection];
        m_bLogFilterChanged = true;
      }
    }

    ImGui::Separator();
  }

  BuildFilteredLogStrings();

  const auto& logStrings = m_bFilterLog ? m_FilteredLogStrings : m_LogStrings;

  // Reserve space for the Clear Log button at the bottom
  const float footer_height_to_reserve = bFull ? (ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing()) : 0;

  if (ImGui::BeginChild("LogScrolling", ImVec2(0, -footer_height_to_reserve), false, bFull ? ImGuiWindowFlags_HorizontalScrollbar : ImGuiWindowFlags_NoScrollbar))
  {
    for (const auto& consoleString : logStrings)
    {
      // Set color based on message type
      ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default white
      switch (consoleString.m_Type)
      {
        case ezConsoleString::Type::Error:
          color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); // Red
          break;
        case ezConsoleString::Type::SeriousWarning:
          color = ImVec4(1.0f, 0.6f, 0.2f, 1.0f); // Orange
          break;
        case ezConsoleString::Type::Warning:
          color = ImVec4(1.0f, 1.0f, 0.4f, 1.0f); // Yellow
          break;
        case ezConsoleString::Type::Success:
          color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); // Green
          break;
        case ezConsoleString::Type::Note:
          color = ImVec4(0.6f, 0.8f, 1.0f, 1.0f); // Light blue
          break;
        case ezConsoleString::Type::Dev:
          color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); // Gray
          break;
        case ezConsoleString::Type::Debug:
          color = ImVec4(0.4f, 0.6f, 0.8f, 1.0f); // Light blue
          break;
        default:
          break;
      }

      ImGui::PushStyleColor(ImGuiCol_Text, color);
      ImGui::TextUnformatted(consoleString.m_sText.GetData());
      ImGui::PopStyleColor();
    }

    ImGui::TextUnformatted("");

    // Auto-scroll to bottom when new messages arrive
    if (m_bScrollLogToBottom)
    {
      ImGui::SetScrollHereY(1.0f);
      m_bScrollLogToBottom = false;
    }
  }

  ImGui::EndChild();

  if (bFull)
  {
    // Clear Log button at the bottom right
    ImGui::Separator();

    // Calculate button width and position it to the right
    const char* buttonText = "Clear Log";
    const float buttonWidth = ImGui::CalcTextSize(buttonText).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - buttonWidth);

    if (ImGui::Button(buttonText))
    {
      ClearLogStrings();
    }
  }

  ImGui::End();
}


void ezImGuiConsole::RenderConsole(bool bIsOpen)
{
  UpdateFrameTimes();
  UpdateMemoryUsage();

  if (bIsOpen != m_bWasOpen)
  {
    m_bWasOpen = bIsOpen;
    m_uiForceFocus = 3;

    if (auto pInput = ezInputManager::GetInputDeviceOfType<ezInputDeviceMouseKeyboard>())
    {
      if (bIsOpen)
      {
        m_bCursorWasVisible = pInput->GetShowMouseCursor();
        m_MouseWasClipped = pInput->GetClipMouseCursor();
        pInput->SetShowMouseCursor(true);
        pInput->SetClipMouseCursor(ezMouseCursorClipMode::NoClip);
      }
      else
      {
        pInput->SetShowMouseCursor(m_bCursorWasVisible);
        pInput->SetClipMouseCursor(m_MouseWasClipped);
      }
    }
  }

  if (bIsOpen || (m_bStatsWindowOpen && m_bPinStatsWindow) || (m_bLogWindowOpen && m_bPinLogWindow))
  {
    if (ezImgui::GetSingleton() == nullptr)
    {
      EZ_DEFAULT_NEW(ezImgui);
    }

    const ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView);
    if (pView == nullptr)
      return;

    ezImgui::GetSingleton()->SetCurrentContextForView(pView->GetHandle());
  }

  if (bIsOpen)
  {
    RenderCommandWindow(m_uiForceFocus > 0);
    if (m_uiForceFocus > 0)
    {
      --m_uiForceFocus;
    }

    if (m_bCVarWindowOpen)
    {
      RenderCVarWindow();
    }
  }

  if (m_bStatsWindowOpen && (bIsOpen || m_bPinStatsWindow))
  {
    RenderStatsWindow(bIsOpen);
  }

  if (m_bLogWindowOpen && (bIsOpen || m_bPinLogWindow))
  {
    RenderLogWindow(bIsOpen);
  }

  // Reset layout flag after all windows have been rendered
  if (m_uiResetLayout > 0)
  {
    --m_uiResetLayout;
  }
}

void ezImGuiConsole::HandleInput(bool bIsOpen)
{
  if (!m_bDefaultInputHandlingInitialized)
  {
    m_bDefaultInputHandlingInitialized = true;

    ezInputActionConfig cfg;
    cfg.m_bApplyTimeScaling = false;

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyF2;
    ezInputManager::SetInputActionConfig("Console", "RepeatLast", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyF3;
    ezInputManager::SetInputActionConfig("Console", "RepeatSecondLast", cfg, true);

    return;
  }

  if (ezInputManager::GetInputActionState("Console", "RepeatLast") == ezKeyState::Pressed)
  {
    if (GetInputHistory().GetCount() >= 1)
    {
      m_bExecutingCommand = true;
      ExecuteCommand(GetInputHistory()[0]);
      m_bExecutingCommand = false;
    }
  }

  if (ezInputManager::GetInputActionState("Console", "RepeatSecondLast") == ezKeyState::Pressed)
  {
    if (GetInputHistory().GetCount() >= 2)
    {
      m_bExecutingCommand = true;
      ExecuteCommand(GetInputHistory()[1]);
      m_bExecutingCommand = false;
    }
  }
}

#endif // BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
