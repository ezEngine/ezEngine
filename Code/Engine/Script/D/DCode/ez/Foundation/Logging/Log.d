module ez.Foundation.Logging.Log;

extern(C++) struct ezLoggingEventData {}

/// \brief Base class for all logging classes.
///
/// You can derive from this class to create your own logging system,
/// which you can pass to the functions in ezLog.
export extern(C++) class ezLogInterface
{
public:
  /// \brief Override this function to handle logging events.
  abstract void HandleLogMessage(const ref ezLoggingEventData le);
};

/// \brief Static class that allows to write out logging information.
///
/// This class takes logging information, prepares it and then broadcasts it to all interested code
/// via the event interface. It does not write anything on disk or somewhere else, itself. Instead it
/// allows to register custom log writers that can then write it to disk, to console, send it over a
/// network or pop up a message box. Whatever suits the current situation.
/// Since event handlers can be registered only temporarily, it is also possible to just gather all
/// errors that occur during some operation and then unregister the event handler again.
export extern(C++) class ezLog
{
public:
  /// \brief Allows to change which logging system is used by default. If nothing is set, ezGlobalLog is used.
  static void SetDefaultLogSystem(ezLogInterface pInterface);

  /// \brief Returns the currently set default logging system.
  static ezLogInterface GetDefaultLogSystem();

  /// \brief An error that needs to be fixed as soon as possible.
  static void Error(const(char)* szFormat, ...);

  /// \brief Overload of Error() to output messages to a specific log.
  static void Error(ezLogInterface pInterface, const(char)* szFormat, ...);

  /// \brief Not an error, but definitely a big problem, that should be looked into very soon.
  static void SeriousWarning(const(char)* szFormat, ...);

  /// \brief Overload of SeriousWarning() to output messages to a specific log.
  static void SeriousWarning(ezLogInterface pInterface, const(char)* szFormat, ...);

  /// \brief A potential problem or a performance warning. Might be possible to ignore it.
  static void Warning(const(char)* szFormat, ...);

  /// \brief Overload of Warning() to output messages to a specific log.
  static void Warning(ezLogInterface pInterface, const(char)* szFormat, ...);

  /// \brief Status information that something was completed successfully.
  static void Success(const(char)* szFormat, ...);

  /// \brief Overload of Success() to output messages to a specific log.
  static void Success(ezLogInterface pInterface, const(char)* szFormat, ...);

  /// \brief Status information that is important.
  static void Info(const(char)* szFormat, ...);

  /// \brief Overload of Info() to output messages to a specific log.
  static void Info(ezLogInterface pInterface, const(char)* szFormat, ...);

  /// \brief Status information that is nice to have during development.
  ///
  /// This function is compiled out in non-development builds.
  static void Dev(const(char)* szFormat, ...);

  /// \brief Overload of Dev() to output messages to a specific log.
  static void Dev(ezLogInterface pInterface, const(char)* szFormat, ...);

  /// \brief Status information during debugging. Very verbose. Usually only temporarily added to the code.
  ///
  /// This function is compiled out in non-debug builds.
  static void Debug(const(char)* szFormat, ...);

  /// \brief Overload of Debug() to output messages to a specific log.
  static void Debug(ezLogInterface pInterface, const(char)* szFormat, ...);

  /// \brief This is a placeholder function for verbose logging during debugging some piece of code.
  ///
  /// By default this function does not do anything (it has an empty body). It is meant to be redirected to one
  /// of the other logging methods (e.g. Debug() ), using a #define.
  /// This allows to only enable it in selected cpp files. E.g. you can use it in hundreds of source files, but
  /// the function will only have an effect in those few files where a\n
  /// \code{.cpp} #define VerboseDebugMessage Debug \endcode \n
  /// is inserted, which will actually redirect all logging to another function.
  /// You can commit your code that calls this logging function, but it should always be 'deactivated',
  /// e.g. the line \code{.cpp} #define VerboseDebugMessage Debug \endcode should always be commented out.
  static void VerboseDebugMessage(const(char)* szFormat, ...);

  /// \brief Overload of VerboseDebugMessage() to output messages to a specific log.
  static void VerboseDebugMessage(ezLogInterface pInterface, const(char)* szFormat, ...);
};