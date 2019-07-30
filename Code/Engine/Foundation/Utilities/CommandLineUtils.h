#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

/// \brief This is a helper class to parse command lines.
///
/// Initialize it using SetCommandLine(). Then query for command line options using GetStringOption(), GetBoolOption(), GetIntOption()
/// or GetFloatOption()
class EZ_FOUNDATION_DLL ezCommandLineUtils
{
public:
  enum ArgMode
  {
    UseArgcArgv,  ///< Use the passed in argc/argv values as they are passed in
    PreferOsArgs, ///< On Windows, ignore argc/argv and instead query the global arguments from the OS. Necessary to properly support
                  ///< Unicode strings in arguments.
  };

  /// \brief Returns one global instance of ezCommandLineUtils.
  static ezCommandLineUtils* GetGlobalInstance();

  /// \brief Splits a string into the classic argc/argv string.
  ///
  /// Useful for platforms where command line args come in as a single string.
  /// \param addExecutableDir
  ///   Adds executable path as first parameter (just as it would normally be in 'int main(argc, argv)').
  static void SplitCommandLineString(
    const char* commandString, bool addExecutableDir, ezDynamicArray<ezString>& outArgs, ezDynamicArray<const char*>& outArgsV);

  /// \brief Initializes ezCommandLineUtils from the parameter arguments that were passed to the application.
  void SetCommandLine(ezUInt32 argc, const char** argv, ArgMode mode = UseArgcArgv); // [tested]

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  /// \brief Initializes ezCommandLineUtils by querying the command line parameters directly from the OS.
  ///
  /// This function is not available on all platforms.
  void SetCommandLine();
#endif

  /// \brief Returns the total number of command line parameters (excluding the program path, which is often passed as the first parameter).
  ezUInt32 GetParameterCount() const; // [tested]

  /// \brief Returns the n-th parameter string that was passed to the application.
  const char* GetParameter(ezUInt32 uiParam) const; // [tested]

  /// \brief Returns the index at which the given option string can be found in the parameter list.
  ///
  /// \param szOption
  ///   The name of the command line option. Must start with a hyphen (-)
  /// \param bCaseSensitive
  ///   Whether the option name szOption shall be searched case sensitive.
  ///
  /// \return
  ///  -1 When no option with the given name is found.
  ///  Otherwise the index at which the option can be found. This can be passed to GetParameter() or GetStringOptionArguments().
  ezInt32 GetOptionIndex(const char* szOption, bool bCaseSensitive = false) const; // [tested]

  /// \brief Returns how many arguments follow behind the option with the name \a szOption.
  ///
  /// Everything that does not start with a hyphen is considered to be an additional parameter for the option.
  ezUInt32 GetStringOptionArguments(const char* szOption, bool bCaseSensitive = false) const; // [tested]

  /// \brief Returns the n-th parameter to the command line option with the name \a szOption.
  ///
  /// If the option does not exist or does not have that many parameters, \a szDefault is returned.
  const char* GetStringOption(const char* szOption, ezUInt32 uiArgument = 0, const char* szDefault = "",
    bool bCaseSensitive = false) const; // [tested]

  /// \brief Returns a boolean interpretation of the option \a szOption or bDefault if it cannot be found.
  ///
  /// \param szOption
  ///   The name of the option to search for. All option-names must start with a hyphen.
  ///
  /// \param bDefault
  ///   The default value to use when no other value can be derived.
  ///
  /// \param bCaseSensitive
  ///   Whether it should be searched case-sensitive for the option with name \a szOption.
  ///
  /// \return
  ///   If an option with the name \a szOption can be found, which has no parameters, it is interpreted as 'true'.
  ///   If there is one parameter following, it is interpreted using ezConversionUtils::StringToBool().
  ///   If that conversion fails, bDefault is returned.
  bool GetBoolOption(const char* szOption, bool bDefault = false, bool bCaseSensitive = false) const; // [tested]

  /// \brief Returns an integer interpretation of the option \a szOption or iDefault if it cannot be found.
  ///
  /// \param szOption
  ///   The name of the option to search for. All option-names must start with a hyphen.
  ///
  /// \param iDefault
  ///   The default value to use when no other value can be derived.
  ///
  /// \param bCaseSensitive
  ///   Whether it should be searched case-sensitive for the option with name \a szOption.
  ///
  /// \return
  ///   If an option with the name \a szOption can be found, and there is one parameter following,
  ///   it is interpreted using ezConversionUtils::StringToInt().
  ///   If that conversion fails or there is no such option or no parameter follows it, iDefault is returned.
  ezInt32 GetIntOption(const char* szOption, ezInt32 iDefault = 0, bool bCaseSensitive = false) const; // [tested]

  /// \brief Same as GetIntOption() but assumes the value is a uint32.
  ezUInt32 GetUIntOption(const char* szOption, ezUInt32 uiDefault = 0, bool bCaseSensitive = false) const; // [tested]

  /// \brief Returns a float interpretation of the option \a szOption or fDefault if it cannot be found.
  ///
  /// \param szOption
  ///   The name of the option to search for. All option-names must start with a hyphen.
  ///
  /// \param fDefault
  ///   The default value to use when no other value can be derived.
  ///
  /// \param bCaseSensitive
  ///   Whether it should be searched case-sensitive for the option with name \a szOption.
  ///
  /// \return
  ///   If an option with the name \a szOption can be found, and there is one parameter following,
  ///   it is interpreted using ezConversionUtils::StringToFloat().
  ///   If that conversion fails or there is no such option or no parameter follows it, fDefault is returned.
  double GetFloatOption(const char* szOption, double fDefault = 0.0, bool bCaseSensitive = false) const; // [tested]

  /// \brief This allows to append an argument programmatically, that wasn't actually set through the command line.
  ///
  /// This can be useful when the command-line is a method to configure something, which might be hidden away in a plugin,
  /// and we have no other easy way to configure it.
  ///
  /// Be aware that each call to this function is like one command line argument. Therefore to add "-arg test", call it two times,
  /// once with "-arg", once with "test". To add a string with spaces, call it once, but do not wrap the string in artificial quotes.
  void InjectCustomArgument(const char* szArgument); // [tested]

private:
  ezDynamicArray<ezString> m_Commands;
};
