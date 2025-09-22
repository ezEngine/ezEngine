#pragma once

#include <Core/Console/Console.h>
#include <GameEngine/GameEngineDLL.h>

struct ezLoggingEventData;

/// \brief A Quake-style console for in-game configuration of ezCVar and ezConsoleFunction.
///
/// The console displays the recent log activity and allows to modify cvars and call console functions.
/// It supports auto-completion of known keywords.
/// Additionally, 'keys' can be bound to arbitrary commands, such that useful commands can be executed
/// easily.
/// The default implementation uses ezConsoleInterpreter::Lua as the interpreter for commands typed into it.
/// The interpreter can be replaced with custom implementations.
class EZ_GAMEENGINE_DLL ezQuakeConsole final : public ezConsole
{
public:
  ezQuakeConsole();
  virtual ~ezQuakeConsole();



  /// \name Configuration
  /// @{

  /// \brief Adjusts how many strings the console will keep in memory at maximum.
  void SetMaxConsoleStrings(ezUInt32 uiMax) { m_uiMaxConsoleStrings = ezMath::Clamp<ezUInt32>(uiMax, 0, 100000); }

  /// \brief Returns how many strings the console will keep in memory at maximum.
  ezUInt32 GetMaxConsoleStrings() const { return m_uiMaxConsoleStrings; }

  /// \brief Enables or disables that the output from ezGlobalLog is displayed in the console. Enabled by default.
  virtual void EnableLogOutput(bool bEnable);

  /// \brief Writes the state of the console (history, bound keys) to the stream.
  virtual void SaveState(ezStreamWriter& inout_stream) const;

  /// \brief Reads the state of the console (history, bound keys) from the stream.
  virtual void LoadState(ezStreamReader& inout_stream);

  /// @}

  /// \name Command Processing
  /// @{



  /// \brief Executes the given command using the current command interpreter.
  virtual void ExecuteCommand(ezStringView sInput) override;

  /// \brief Binds \a szCommand to \a szKey. Calling ExecuteBoundKey() with this key will then run that command.
  ///
  /// A key can be any arbitrary string. However, it might make sense to either use the standard ASCII characters A-Z and a-z, which allows
  /// to trigger actions by the press of any of those buttons.
  /// You can, however, also use names for input buttons, such as 'Key_Left', but then you also need to call ExecuteBoundKey() with those
  /// names.
  /// If you use such virtual key names, it makes also sense to listen to the auto-complete event and suggest those key names there.
  void BindKey(ezStringView sKey, ezStringView sCommand);

  /// \brief Removes the key binding.
  void UnbindKey(ezStringView sKey);

  /// \brief Executes the command that was bound to this key.
  void ExecuteBoundKey(ezStringView sKey);

  /// @}

  /// \name Input Handling
  /// @{

  /// \brief Inserts one character at the caret position into the console input line.
  ///
  /// This function also calls ProcessInputCharacter and FilterInputCharacter. By default this already reacts on Tab, Enter and ESC
  /// and filters out all non ASCII characters.
  void AddInputCharacter(ezUInt32 uiChar);

  /// \brief Clears the input line of the console.
  void ClearInputLine();

  /// \brief Returns the current content of the input line.
  ezStringView GetInputLine() const { return m_sInputLine; }

  /// \brief Returns the position (in characters) of the caret.
  ezInt32 GetCaretPosition() const { return m_iCaretPosition; }

  /// \brief Moves the caret in the text. Its position will be clamped to the length of the current input line text.
  void MoveCaret(ezInt32 iMoveOffset);

  /// \brief Deletes the character following the caret position.
  void DeleteNextCharacter();

  /// \brief Scrolls the contents of the console up or down. Will be clamped to the available range.
  void Scroll(ezInt32 iLines);

  /// \brief Returns the current scroll position. This must be used during rendering to start with the proper line.
  ezUInt32 GetScrollPosition() const { return m_iScrollPosition; }

  /// \brief This function implements input handling (via ezInputManager) for the console.
  ///
  /// If the console is 'open' (ie. has full focus), it will handle more input for caret movement etc.
  /// However, in the 'closed' state, it will still execute bound keys and commands from the history.
  /// It is not required to call this function, you can implement input handling entirely outside the console.
  ///
  /// If this function is used, it should be called once per frame and if the console is considered 'open',
  /// no further keyboard input should be processed, as that might lead to confusing behavior when the user types
  /// text into the console.
  ///
  /// The state whether the console is considered open has to be managed by the application.
  virtual void DoDefaultInputHandling(bool bConsoleOpen);

  /// @}

  /// \name Console Content
  /// @{

  /// \brief Adds a string to the console.
  virtual void AddConsoleString(ezStringView sText, ezConsoleString::Type type = ezConsoleString::Type::Default) override;

  /// \brief Returns all current console strings. Use GetScrollPosition() to know which one should be displayed as the first one.
  const ezDeque<ezConsoleString>& GetConsoleStrings() const;

  /// \brief Deletes all console strings, making the console empty.
  void ClearConsoleStrings();

  /// \brief Display the console state.
  virtual void RenderConsole(bool bIsOpen) override;

  /// \brief Update the console with the latest input.
  virtual void HandleInput(bool bIsOpen) override;

  /// @}

protected:
  /// \brief Deletes the character at the given position in the input line.
  void RemoveCharacter(ezUInt32 uiInputLinePosition);

  /// \brief Makes sure the caret position is clamped to the input line length.
  void ClampCaretPosition();


  /// \brief The function that is used to read ezGlobalLog messages.
  void LogHandler(const ezLoggingEventData& data);

  ezInt32 m_iCaretPosition;
  ezStringBuilder m_sInputLine;

  virtual bool ProcessInputCharacter(ezUInt32 uiChar);
  virtual bool FilterInputCharacter(ezUInt32 uiChar);
  virtual void InputStringChanged();

  ezDeque<ezConsoleString> m_ConsoleStrings;
  bool m_bUseFilteredStrings = false;
  ezDeque<ezConsoleString> m_FilteredConsoleStrings;
  ezUInt32 m_uiMaxConsoleStrings;
  ezInt32 m_iScrollPosition;
  bool m_bLogOutputEnabled;
  bool m_bDefaultInputHandlingInitialized;


  ezMap<ezString, ezString> m_BoundKeys;
};
