#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Time/Clock.h>
#include <GameEngine/Console/LuaInterpreter.h>
#include <GameEngine/Console/QuakeConsole.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezQuakeConsole::ezQuakeConsole()
{
  ClearInputLine();

  m_bLogOutputEnabled = false;
  m_bDefaultInputHandlingInitialized = false;
  m_uiMaxConsoleStrings = 1000;

  EnableLogOutput(true);

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT
  SetCommandInterpreter(EZ_DEFAULT_NEW(ezCommandInterpreterLua));
#endif
}

ezQuakeConsole::~ezQuakeConsole()
{
  EnableLogOutput(false);
}

void ezQuakeConsole::AddConsoleString(ezStringView sText, ezConsoleString::Type type)
{
  EZ_LOCK(m_Mutex);

  m_ConsoleStrings.PushFront();

  ezConsoleString& cs = m_ConsoleStrings.PeekFront();
  cs.m_sText = sText;
  cs.m_Type = type;

  if (m_ConsoleStrings.GetCount() > m_uiMaxConsoleStrings)
    m_ConsoleStrings.PopBack(m_ConsoleStrings.GetCount() - m_uiMaxConsoleStrings);

  ezConsole::AddConsoleString(sText, type);
}

const ezDeque<ezConsoleString>& ezQuakeConsole::GetConsoleStrings() const
{
  if (m_bUseFilteredStrings)
  {
    return m_FilteredConsoleStrings;
  }

  return m_ConsoleStrings;
}

void ezQuakeConsole::LogHandler(const ezLoggingEventData& data)
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

  AddConsoleString(sFormat.GetData(), type);
}

void ezQuakeConsole::InputStringChanged()
{
  m_bUseFilteredStrings = false;
  m_FilteredConsoleStrings.Clear();

  if (m_sInputLine.StartsWith("*"))
  {
    ezStringBuilder input = m_sInputLine;

    input.Shrink(1, 0);
    input.Trim(" ");

    if (input.IsEmpty())
      return;

    m_FilteredConsoleStrings.Clear();
    m_bUseFilteredStrings = true;

    for (const auto& e : m_ConsoleStrings)
    {
      if (e.m_sText.FindSubString_NoCase(input))
      {
        m_FilteredConsoleStrings.PushBack(e);
      }
    }

    Scroll(0); // clamp scroll position
  }
}

void ezQuakeConsole::EnableLogOutput(bool bEnable)
{
  if (m_bLogOutputEnabled == bEnable)
    return;

  m_bLogOutputEnabled = bEnable;

  if (bEnable)
  {
    ezGlobalLog::AddLogWriter(ezMakeDelegate(&ezQuakeConsole::LogHandler, this));
  }
  else
  {
    ezGlobalLog::RemoveLogWriter(ezMakeDelegate(&ezQuakeConsole::LogHandler, this));
  }
}

void ezQuakeConsole::SaveState(ezStreamWriter& inout_stream) const
{
  EZ_LOCK(m_Mutex);

  const ezUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_InputHistory.GetCount();
  for (ezUInt32 i = 0; i < m_InputHistory.GetCount(); ++i)
  {
    inout_stream << m_InputHistory[i];
  }

  inout_stream << m_BoundKeys.GetCount();
  for (auto it = m_BoundKeys.GetIterator(); it.IsValid(); ++it)
  {
    inout_stream << it.Key();
    inout_stream << it.Value();
  }
}

void ezQuakeConsole::LoadState(ezStreamReader& inout_stream)
{
  EZ_LOCK(m_Mutex);

  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  if (uiVersion == 1)
  {
    ezUInt32 count = 0;
    inout_stream >> count;
    m_InputHistory.SetCount(count);

    for (ezUInt32 i = 0; i < m_InputHistory.GetCount(); ++i)
    {
      inout_stream >> m_InputHistory[i];
    }

    inout_stream >> count;

    ezString sKey;
    ezString sValue;

    for (ezUInt32 i = 0; i < count; ++i)
    {
      inout_stream >> sKey;
      inout_stream >> sValue;

      m_BoundKeys[sKey] = sValue;
    }
  }
}

void ezQuakeConsole::ExecuteCommand(ezStringView sInput)
{
  const bool bBind = sInput.StartsWith_NoCase("bind ");
  const bool bUnbind = sInput.StartsWith_NoCase("unbind ");

  if (bBind || bUnbind)
  {
    ezStringBuilder tmp;
    const char* szAfterCmd = ezStringUtils::FindWordEnd(sInput.GetData(tmp), ezStringUtils::IsWhiteSpace);              // skip the word 'bind' or 'unbind'

    const char* szKeyNameStart = ezStringUtils::SkipCharacters(szAfterCmd, ezStringUtils::IsWhiteSpace);                // go to the next word
    const char* szKeyNameEnd = ezStringUtils::FindWordEnd(szKeyNameStart, ezStringUtils::IsIdentifierDelimiter_C_Code); // find its end

    ezStringView sKey(szKeyNameStart, szKeyNameEnd);
    tmp = sKey;                                                                                                         // copy the word into a zero terminated string

    const char* szCommandToBind = ezStringUtils::SkipCharacters(szKeyNameEnd, ezStringUtils::IsWhiteSpace);

    if (bUnbind || ezStringUtils::IsNullOrEmpty(szCommandToBind))
    {
      UnbindKey(tmp);
      return;
    }

    BindKey(tmp, szCommandToBind);
    return;
  }

  ezConsole::ExecuteCommand(sInput);
}

void ezQuakeConsole::BindKey(ezStringView sKey, ezStringView sCommand)
{
  ezStringBuilder s;
  s.SetFormat("Binding key '{0}' to command '{1}'", sKey, sCommand);
  AddConsoleString(s, ezConsoleString::Type::Success);

  m_BoundKeys[sKey] = sCommand;
}

void ezQuakeConsole::UnbindKey(ezStringView sKey)
{
  ezStringBuilder s;
  s.SetFormat("Unbinding key '{0}'", sKey);
  AddConsoleString(s, ezConsoleString::Type::Success);

  m_BoundKeys.Remove(sKey);
}

void ezQuakeConsole::ExecuteBoundKey(ezStringView sKey)
{
  auto it = m_BoundKeys.Find(sKey);

  if (it.IsValid())
  {
    ExecuteCommand(it.Value());
  }
}

bool ezQuakeConsole::ProcessInputCharacter(ezUInt32 uiChar)
{
  switch (uiChar)
  {
    case 27: // Escape
      ClearInputLine();
      return false;

    case '\b': // backspace
    {
      if (!m_sInputLine.IsEmpty() && m_iCaretPosition > 0)
      {
        RemoveCharacter(m_iCaretPosition - 1);
        MoveCaret(-1);
      }
    }
      return false;

    case '\t':
      if (AutoComplete(m_sInputLine))
      {
        MoveCaret(500);
      }
      return false;

    case 13: // Enter
      AddToInputHistory(m_sInputLine);
      ExecuteCommand(m_sInputLine);
      ClearInputLine();
      return false;
  }

  return true;
}

bool ezQuakeConsole::FilterInputCharacter(ezUInt32 uiChar)
{
  // filter out not only all non-ASCII characters, but also all the non-printable ASCII characters
  // if you want to support full Unicode characters in the console, override this function and change this restriction
  if (uiChar < 32 || uiChar > 126)
    return false;

  return true;
}

void ezQuakeConsole::ClampCaretPosition()
{
  m_iCaretPosition = ezMath::Clamp<ezInt32>(m_iCaretPosition, 0, m_sInputLine.GetCharacterCount());
}

void ezQuakeConsole::MoveCaret(ezInt32 iMoveOffset)
{
  m_iCaretPosition += iMoveOffset;

  ClampCaretPosition();
}

void ezQuakeConsole::Scroll(ezInt32 iLines)
{
  if (m_bUseFilteredStrings)
    m_iScrollPosition = ezMath::Clamp<ezInt32>(m_iScrollPosition + iLines, 0, ezMath::Max<ezInt32>(m_FilteredConsoleStrings.GetCount() - 10, 0));
  else
    m_iScrollPosition = ezMath::Clamp<ezInt32>(m_iScrollPosition + iLines, 0, ezMath::Max<ezInt32>(m_ConsoleStrings.GetCount() - 10, 0));
}

void ezQuakeConsole::ClearInputLine()
{
  m_sInputLine.Clear();
  m_iCaretPosition = 0;
  m_iScrollPosition = 0;
  m_iCurrentInputHistoryElement = -1;

  m_FilteredConsoleStrings.Clear();
  m_bUseFilteredStrings = false;

  InputStringChanged();
}

void ezQuakeConsole::ClearConsoleStrings()
{
  m_ConsoleStrings.Clear();
  m_FilteredConsoleStrings.Clear();
  m_bUseFilteredStrings = false;
  m_iScrollPosition = 0;
}

void ezQuakeConsole::DeleteNextCharacter()
{
  RemoveCharacter(m_iCaretPosition);
}

void ezQuakeConsole::RemoveCharacter(ezUInt32 uiInputLinePosition)
{
  if (uiInputLinePosition >= m_sInputLine.GetCharacterCount())
    return;

  auto it = m_sInputLine.GetIteratorFront();
  it += uiInputLinePosition;

  auto itNext = it;
  ++itNext;

  m_sInputLine.Remove(it.GetData(), itNext.GetData());

  InputStringChanged();
}

void ezQuakeConsole::AddInputCharacter(ezUInt32 uiChar)
{
  if (uiChar == '\0')
    return;

  if (!ProcessInputCharacter(uiChar))
    return;

  if (!FilterInputCharacter(uiChar))
    return;

  ClampCaretPosition();

  auto it = m_sInputLine.GetIteratorFront();
  it += m_iCaretPosition;

  ezUInt32 uiString[2] = {uiChar, 0};

  m_sInputLine.Insert(it.GetData(), ezStringUtf8(uiString).GetData());

  MoveCaret(1);

  InputStringChanged();
}

void ezQuakeConsole::DoDefaultInputHandling(bool bConsoleOpen)
{
  if (!m_bDefaultInputHandlingInitialized)
  {
    m_bDefaultInputHandlingInitialized = true;

    ezInputActionConfig cfg;
    cfg.m_bApplyTimeScaling = true;

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyLeft;
    ezInputManager::SetInputActionConfig("Console", "MoveCaretLeft", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyRight;
    ezInputManager::SetInputActionConfig("Console", "MoveCaretRight", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyHome;
    ezInputManager::SetInputActionConfig("Console", "MoveCaretStart", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyEnd;
    ezInputManager::SetInputActionConfig("Console", "MoveCaretEnd", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyDelete;
    ezInputManager::SetInputActionConfig("Console", "DeleteCharacter", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyPageUp;
    ezInputManager::SetInputActionConfig("Console", "ScrollUp", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyPageDown;
    ezInputManager::SetInputActionConfig("Console", "ScrollDown", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyUp;
    ezInputManager::SetInputActionConfig("Console", "HistoryUp", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyDown;
    ezInputManager::SetInputActionConfig("Console", "HistoryDown", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyF2;
    ezInputManager::SetInputActionConfig("Console", "RepeatLast", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyF3;
    ezInputManager::SetInputActionConfig("Console", "RepeatSecondLast", cfg, true);

    return;
  }

  if (bConsoleOpen)
  {
    if (ezInputManager::GetInputActionState("Console", "MoveCaretLeft") == ezKeyState::Pressed)
      MoveCaret(-1);
    if (ezInputManager::GetInputActionState("Console", "MoveCaretRight") == ezKeyState::Pressed)
      MoveCaret(1);
    if (ezInputManager::GetInputActionState("Console", "MoveCaretStart") == ezKeyState::Pressed)
      MoveCaret(-1000);
    if (ezInputManager::GetInputActionState("Console", "MoveCaretEnd") == ezKeyState::Pressed)
      MoveCaret(1000);
    if (ezInputManager::GetInputActionState("Console", "DeleteCharacter") == ezKeyState::Pressed)
      DeleteNextCharacter();
    if (ezInputManager::GetInputActionState("Console", "ScrollUp") == ezKeyState::Pressed)
      Scroll(10);
    if (ezInputManager::GetInputActionState("Console", "ScrollDown") == ezKeyState::Pressed)
      Scroll(-10);
    if (ezInputManager::GetInputActionState("Console", "HistoryUp") == ezKeyState::Pressed)
    {
      RetrieveInputHistory(1, m_sInputLine);
      m_iCaretPosition = m_sInputLine.GetCharacterCount();
    }
    if (ezInputManager::GetInputActionState("Console", "HistoryDown") == ezKeyState::Pressed)
    {
      RetrieveInputHistory(-1, m_sInputLine);
      m_iCaretPosition = m_sInputLine.GetCharacterCount();
    }

    const ezUInt32 uiChar = ezInputManager::RetrieveLastCharacter();

    if (uiChar != '\0')
      AddInputCharacter(uiChar);
  }
  else
  {
    const ezUInt32 uiChar = ezInputManager::RetrieveLastCharacter(false);

    char szCmd[16] = "";
    char* szIterator = szCmd;
    ezUnicodeUtils::EncodeUtf32ToUtf8(uiChar, szIterator);
    *szIterator = '\0';
    ExecuteBoundKey(szCmd);
  }

  if (ezInputManager::GetInputActionState("Console", "RepeatLast") == ezKeyState::Pressed)
  {
    if (GetInputHistory().GetCount() >= 1)
      ExecuteCommand(GetInputHistory()[0]);
  }

  if (ezInputManager::GetInputActionState("Console", "RepeatSecondLast") == ezKeyState::Pressed)
  {
    if (GetInputHistory().GetCount() >= 2)
      ExecuteCommand(GetInputHistory()[1]);
  }
}

void ezQuakeConsole::RenderConsole(bool bIsOpen)
{
  if (!bIsOpen)
    return;

  const ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView);
  if (pView == nullptr)
    return;

  ezViewHandle hView = pView->GetHandle();
  const float fViewWidth = pView->GetViewport().width;
  const float fViewHeight = pView->GetViewport().height;
  const float fGlyphWidth = ezDebugRenderer::GetTextGlyphWidth();
  const float fLineHeight = ezDebugRenderer::GetTextLineHeight();

  const float fConsoleHeight = fViewHeight * 0.4f;
  const float fBorderWidth = 2.0f;
  const float fConsoleTextAreaHeight = fConsoleHeight - (2.0f * fBorderWidth) - fLineHeight;

  const ezInt32 iTextHeight = (ezInt32)fLineHeight;
  const ezInt32 iTextLeft = (ezInt32)(fBorderWidth);

  // Draw console background
  {
    ezColor backgroundColor(0.3f, 0.3f, 0.3f, 0.7f);
    ezDebugRenderer::Draw2DRectangle(hView, ezRectFloat(0.0f, 0.0f, fViewWidth, fConsoleHeight), 0.0f, backgroundColor);
    ezColor foregroundColor(0.0f, 0.0f, 0.0f, 0.8f);
    ezDebugRenderer::Draw2DRectangle(hView, ezRectFloat(fBorderWidth, 0.0f, fViewWidth - (2.0f * fBorderWidth), fConsoleTextAreaHeight), 0.0f, foregroundColor);
    ezDebugRenderer::Draw2DRectangle(hView, ezRectFloat(fBorderWidth, fConsoleTextAreaHeight + fBorderWidth, fViewWidth - (2.0f * fBorderWidth), fLineHeight), 0.0f, foregroundColor);
  }

  // Draw console text
  {
    EZ_LOCK(GetMutex());
    auto& consoleStrings = GetConsoleStrings();
    ezUInt32 uiNumConsoleLines = (ezUInt32)(ezMath::Ceil(fConsoleTextAreaHeight / fLineHeight));
    ezInt32 uiFirstLine = GetScrollPosition() + uiNumConsoleLines - 1;
    ezInt32 iFirstLinePos = (ezInt32)(fBorderWidth);

    ezInt32 uiSkippedLines = ezMath::Max(uiFirstLine - (ezInt32)consoleStrings.GetCount() + 1, 0);
    for (ezUInt32 i = uiSkippedLines; i < uiNumConsoleLines; ++i)
    {
      auto& consoleString = consoleStrings[uiFirstLine - i];
      ezDebugRenderer::Draw2DText(hView, consoleString.m_sText.GetData(), ezVec2I32(iTextLeft, iFirstLinePos + i * iTextHeight), consoleString.GetColor());
    }

    // Draw input line
    ezDebugRenderer::Draw2DText(hView, GetInputLine(), ezVec2I32(iTextLeft, (ezInt32)(fConsoleTextAreaHeight + fBorderWidth + (fLineHeight * 0.5f))), ezColor::White, 16, ezDebugTextHAlign::Default, ezDebugTextVAlign::Center);

    // Draw caret
    if (ezMath::Fraction(ezClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds()) > 0.5)
    {
      const float fCaretPosition = (float)GetCaretPosition();
      const float fCaretX = fBorderWidth + (fCaretPosition + 0.5f) * fGlyphWidth;
      const float fCaretY = fConsoleTextAreaHeight + fBorderWidth + 1.0f;
      ezColor caretColor(1.0f, 1.0f, 1.0f, 0.5f);
      ezDebugRenderer::Draw2DRectangle(hView, ezRectFloat(fCaretX, fCaretY, 2.0f, fLineHeight - 2.0f), 0.0f, caretColor);
    }
  }
}

void ezQuakeConsole::HandleInput(bool bIsOpen)
{
  DoDefaultInputHandling(bIsOpen);
}
