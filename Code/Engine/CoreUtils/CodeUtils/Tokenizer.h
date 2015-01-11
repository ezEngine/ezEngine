#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/HashedString.h>

/// \brief Describes which kind of token an ezToken is.
struct EZ_COREUTILS_DLL ezTokenType
{
  enum Enum
  {
    Unknown,        ///< for internal use
    Whitespace,     ///< The token is a space or tab
    Identifier,     ///< a series of alphanumerics or underscores
    NonIdentifier,  ///< Everything else
    Newline,        ///< Either '\n' or '\r\n'
    LineComment,    ///< A comment that starts with two slashes and ends at the next newline (or end of file)
    BlockComment,   ///< A comment that starts with a slash and a star, and ends at the next star/slash combination (or end of file)
    String1,        ///< A string enclosed in "
    String2,        ///< A string enclosed in '
    EndOfFile,      ///< End-of-file marker
    ENUM_COUNT,
  };

  static const char* EnumNames[ENUM_COUNT];
};

/// \brief Represents one piece of tokenized text in a document.
struct EZ_COREUTILS_DLL ezToken
{
  ezToken()
  {
    m_iType = ezTokenType::Unknown;
    m_uiLine = 0;
    m_uiColumn = 0;
    m_uiCustomFlags = 0;
  }

  /// Typically of type ezTokenType, but users can put anything in there, that they like
  ezInt32 m_iType;

  /// The line in which the token appeared
  ezUInt32 m_uiLine;

  /// The column in the line, at which the token string started.
  ezUInt32 m_uiColumn;

  /// The actual string data that represents the token. Note that this is a view to a substring of some larger text data.
  /// To get only the relevant piece as one zero-terminated string, assign m_DataView to an ezStringBuilder and read that instead.
  ezStringView m_DataView;

  /// For users to be able to store additional info for a token.
  ezUInt32 m_uiCustomFlags;

  /// The file in which the token appeared.
  ezHashedString m_File;
};

/// \brief Takes text and splits it up into ezToken objects. The result can be used for easier parsing.
///
/// The tokenizer is built to work on code that is similar to C. That means it will tokenize comments and
/// strings as they are defined in the C language. Also line breaks that end with a backslash are not
/// really considered as line breaks.\n
/// White space is defined as spaces and tabs.\n
/// Identifiers are names that consist of alphanumerics and underscores.\n
/// Non-Identifiers are everything else. However, they will currently never consist of more than a single character.
/// Ie. '++' will be tokenized as two consecutive non-Identifiers.\n
/// Parenthesis etc. will not be tokenized in any special way, they are all considered as non-Identifiers.
///
/// The token stream will always end with an end-of-file token.
class EZ_COREUTILS_DLL ezTokenizer
{
public:
  /// \brief Constructor.
  ezTokenizer();

  /// \brief Clears any previous result and creates a new token stream for the given array.
  void Tokenize(const ezDynamicArray<ezUInt8>& Data, ezLogInterface* pLog);

  /// \brief Gives read access to the token stream.
  const ezDeque<ezToken>& GetTokens() const { return m_Tokens; }

  /// \brief Gives read and write access to the token stream.
  ezDeque<ezToken>& GetTokens() { return m_Tokens; }

  /// \brief Returns an array of tokens that represent the next line in the file.
  ///
  /// Returns EZ_SUCCESS when there was more data to return, EZ_FAILURE if the end of the file was reached already.
  /// uiFirstToken is the index from where to start. It will be updated automatically. Consecutive calls to GetNextLine()
  /// with the same uiFirstToken variable will give one line after the other.
  ///
  /// \note This function takes care of handling the 'backslash/newline' combination, as defined in the C language.
  /// That means all such sequences will be ignored. Therefore the tokens that are returned as one line might not
  /// contain all tokens that are actually in the stream. Also the tokens might have different line numbers, when
  /// two or more lines from the file are merged into one logical line.
  ezResult GetNextLine(ezUInt32& uiFirstToken, ezHybridArray<const ezToken*, 32>& Tokens) const;

  ezResult GetNextLine(ezUInt32& uiFirstToken, ezHybridArray<ezToken*, 32>& Tokens);

private:
  void NextChar();
  void AddToken();

  void HandleUnknown();
  void HandleString1();
  void HandleString2();
  void HandleLineComment();
  void HandleBlockComment();
  void HandleWhitespace();
  void HandleIdentifier();
  void HandleNonIdentifier();

  ezLogInterface* m_pLog;
  ezTokenType::Enum m_CurMode;
  ezStringView m_Iterator;
  ezUInt32 m_uiCurLine;
  ezUInt32 m_uiCurColumn;
  ezUInt32 m_uiCurChar;
  ezUInt32 m_uiNextChar;

  ezUInt32 m_uiLastLine;
  ezUInt32 m_uiLastColumn;

  const char* m_szCurCharStart;
  const char* m_szNextCharStart;
  const char* m_szTokenStart;

  ezDeque<ezToken> m_Tokens;
  ezDynamicArray<ezUInt8> m_Data;
};


