#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/CVar.h>
#include <CoreUtils/Console/ConsoleFunction.h>


/// \brief A Quake-style console is for in-game configuration of ezCVar and ezConsoleFunction.
///
/// The console displays the recent log activity and allows to modify cvars and call console functions.
/// It supports auto-completion of known keywords.
/// Additionally, 'keys' can be bound to arbitrary commands, such that useful commands can be executed
/// easily.
/// The default implementation uses ezConsoleInterpreter::Lua as the interpreter for commands typed into it.
/// The interpreter can be replaced with custom implementations.
class EZ_COREUTILS_DLL ezConsole
{
public:
  ezConsole();
  virtual ~ezConsole();

  /// \brief The delegate type for an interpreter of all the commands that are typed into the console.
  typedef ezDelegate<ezResult (const char*, ezConsole*)> ezCommandProcessor;

  /// \brief The data for one text entry in the console window.
  struct ConsoleString
  {
    ezString m_sText;
    ezColor m_TextColor;

    /// \brief The timestamp of when the string was added.
    ezTime m_TimeStamp;

    /// \brief If true, the application might show this string on screen when the console is not displayed.
    bool m_bShowOnScreen;

    ConsoleString() { m_TextColor = ezColor::White; m_bShowOnScreen = false; }
    bool operator< (const ConsoleString& rhs) const { return m_sText < rhs.m_sText; }
  };

  /// \name Events
  /// @{


  /// \brief The event data that is broadcast by the console
  struct ConsoleEvent
  {
    enum EventType
    {
      BeforeProcessCommand,   ///< The console is about to process a command
      ProcessCommandSuccess,  ///< A command was successfully processed
      ProcessCommandFailure,  ///< A command failed to be processed
      StringAdded,            ///< A string was added to the console
      AutoCompleteRequest,    ///< The user tries to auto-complete the input
    };

    EventType m_EventType;

    /// \brief The command the was or will be processed
    const char* m_szCommand;

    /// \brief The console string that was just added.
    const ConsoleString* m_AddedpConsoleString;

    /// \brief AutoComplete: The strings that might be used for auto-completion, the event handler may add additional suggestions to it.
    ezDeque<ezString>* m_pAutoCompleteOptions;

    /// \brief The descriptions of each auto-complete option, to be shown in the console.
    ezDeque<ConsoleString>* m_pAutoCompleteDescriptions;
  };

  /// \brief The console event variable, to attach to.
  ezEvent<ConsoleEvent&> m_Events;

  /// @}

  /// \name Configuration
  /// @{

  /// \brief Adjusts how many strings the console will keep in memory at maximum.
  void SetMaxConsoleStrings(ezUInt32 uiMax) { m_uiMaxConsoleStrings = ezMath::Clamp<ezUInt32>(uiMax, 0, 100000); }

  /// \brief Returns how many strings the console will keep in memory at maximum.
  ezUInt32 GetMaxConsoleStrings() const  { return m_uiMaxConsoleStrings; }

  /// \brief Enables or disables that the output from ezGlobalLog is displayed in the console. Enabled by default.
  virtual void EnableLogOutput(bool bEnable);

  /// \brief Writes the state of the console (history, bound keys) to the stream.
  virtual void SaveState(ezStreamWriterBase& Stream) const;

  /// \brief Reads the state of the console (history, bound keys) from the stream.
  virtual void LoadState(ezStreamReaderBase& Stream);

  /// @}

  /// \name Command Processing
  /// @{

  /// \brief Replaces the current command interpreter. This allows to attach a custom interpreter to the console.
  ///
  /// by default the Lua interpreter is used. Using a custom interpreter you can extend its functionality or allow for different syntax.
  void SetCommandInterpreter(ezCommandProcessor processor) { m_CommandProcessor = processor; }

  /// \brief Executes the given command using the current command interpreter.
  ///
  /// This will also broadcast ConsoleEvent::BeforeProcessCommand and ConsoleEvent::ProcessCommandSuccess or ConsoleEvent::ProcessCommandFailure,
  /// depending on what the interpreter returned.
  void ProcessCommand(const char* szCmd);

  /// \brief Binds \a szCommand to \a szKey. Calling ExecuteBoundKey() with this key will then run that command.
  ///
  /// A key can be any arbitrary string. However, it might make sense to either use the standard ASCII characters A-Z and a-z, which allows
  /// to trigger actions by the press of any of those buttons.
  /// You can, however, also use names for input buttons, such as 'Key_Left', but then you also need to call ExecuteBoundKey() with those
  /// names. 
  /// If you use such virtual key names, it makes also sense to listen to the auto-complete event and suggest those key names there.
  void BindKey(const char* szKey, const char* szCommand);

  /// \brief Removes the key binding.
  void UnbindKey(const char* szKey);

  /// \brief Executes the command that was bound to this key.
  void ExecuteBoundKey(const char* szKey);

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
  const char* GetInputLine() const { return m_sInputLine.GetData(); }

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

  /// \brief Tries to auto-complete the current input line.
  ///
  /// This will trigger ConsoleEvent::AutoCompleteRequest, which allows external code to suggest auto-complete options for
  /// the current word.
  /// The console will always use all CVars for auto-completion already.
  virtual void AutoCompleteInputLine();

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
  ///
  /// bShowOnScreen is a hint for the renderer to also display this string on screen, even if the console is not visible.
  void AddConsoleString(const char* szText, const ezColor& color = ezColor(1, 1, 1), bool bShowOnScreen = false);

  /// \brief Returns all current console strings. Use GetScrollPosition() to know which one should be displayed as the first one.
  const ezDeque<ConsoleString>& GetConsoleStrings() const { return m_ConsoleStrings; }

  /// \brief Deletes all console strings, making the console empty.
  void ClearConsoleStrings();

  /// \brief Returns the current input history.
  const ezStaticArray<ezString, 16>& GetInputHistory() const { return m_InputHistory; }

  /// \brief Replaces the input line by the next (or previous) history item.
  void SearchInputHistory(ezInt32 iHistoryUp);

  /// @}

  /// \name Helpers
  /// @{

  /// \brief Returns a nice string containing all the important information about the cvar.
  static ezString GetFullInfoAsString(ezCVar* pCVar);

  /// \brief Returns the value of the cvar as a string.
  static const ezString GetValueAsString(ezCVar* pCVar);

  /// @}

protected:
  /// \brief Deletes the character at the given position in the input line.
  void RemoveCharacter(ezUInt32 uiInputLinePosition);
  
  /// \brief Makes sure the caret position is clamped to the input line length.
  void ClampCaretPosition();

  /// \brief Adds an item to the input history.
  void AddToInputHistory(const char* szString);

  /// \brief Iterates over all cvars and finds all that start with the string \a szVariable.
  static void FindPossibleCVars(const char* szVariable, ezDeque<ezString>& CommonStrings, ezDeque<ConsoleString>& ConsoleStrings);

  /// \brief Iterates over all console functions and finds all that start with the string \a szVariable.
  static void FindPossibleFunctions(const char* szVariable, ezDeque<ezString>& CommonStrings, ezDeque<ConsoleString>& ConsoleStrings);

  /// \brief Returns the prefix string that is common to all strings in the \a vStrings array.
  static const ezString FindCommonString(const ezDeque<ezString>& vStrings);

  /// \brief The function that is used to read ezGlobalLog messages.
  void LogHandler(const ezLoggingEventData& data);

  ezInt32 m_iCaretPosition;
  ezStringBuilder m_sInputLine;

private:
  virtual bool ProcessInputCharacter(ezUInt32 uiChar);
  virtual bool FilterInputCharacter(ezUInt32 uiChar);

  ezCommandProcessor m_CommandProcessor;
  ezDeque<ConsoleString> m_ConsoleStrings;
  ezUInt32 m_uiMaxConsoleStrings;
  ezInt32 m_iScrollPosition;
  ezInt32 m_iCurrentInputHistoryElement;
  bool m_bLogOutputEnabled;
  bool m_bDefaultInputHandlingInitialized;

  ezStaticArray<ezString, 16> m_InputHistory;
  ezMap<ezString, ezString> m_BoundKeys;
};


