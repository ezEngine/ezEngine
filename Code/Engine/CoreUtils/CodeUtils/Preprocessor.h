#pragma once

#include <CoreUtils/Basics.h>
#include <CoreUtils/CodeUtils/Tokenizer.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Time/Timestamp.h>

/// \brief This object caches files in a tokenized state. It can be shared among ezPreprocessor instances to improve performance when
/// they access the same files.
class EZ_COREUTILS_DLL ezTokenizedFileCache
{
public:
  struct FileData
  {
    ezTokenizer m_Tokens;
    ezTimestamp m_Timestamp;
  };

  /// \brief Checks whether \a sFileName is already in the cache, returns an iterator to it. If the iterator is invalid, the file is not cached yet.
  ezMap<ezString, FileData>::ConstIterator Lookup(const ezString& sFileName) const;

  /// \brief Removes the cached content for \a sFileName from the cache. Should be used when the file content has changed and needs to be re-read.
  void Remove(const ezString& sFileName);

  /// \brief Removes all files from the cache to ensure that they will be re-read.
  void Clear();

  /// \brief Stores \a FileContent for the file \a sFileName as the new cached data.
  /// 
  //// The file content is tokenized first and all #line directives are evaluated, to update the line number and file origin for each token.
  /// Any errors are written to the given log.
  const ezTokenizer* Tokenize(const ezString& sFileName, const ezDynamicArray<ezUInt8>& FileContent, const ezTimestamp& FileTimeStamp, ezLogInterface* pLog);

private:
  void SkipWhitespace(ezDeque<ezToken>& Tokens, ezUInt32& uiCurToken);

  mutable ezMutex m_Mutex;
  ezMap<ezString, FileData> m_Cache;
};

/// \brief ezPreprocessor implements a standard C preprocessor. It can be used to preprocess files to get the output after macro expansion and #ifdef handling.
///
/// For a detailed documentation about the C preprocessor, see https://gcc.gnu.org/onlinedocs/cpp/
///
/// This class implements all standard features:
///   * object and function macros
///   * Full evaluation of #if, #ifdef etc. including mathematical operations such as #if A > 42
///   * Parameter stringification
///   * Parameter concatenation
///   * __LINE__ and __FILE__ macros
///   * Fully correct #line evaluation for error output
///   * Correct handling of __VA_ARGS__
///   * #include handling
///   * #pragma once
///   * #warning and #error for custom failure messages
class EZ_COREUTILS_DLL ezPreprocessor
{
public:

  /// \brief Describes the type of #include that was encountered during preprocessing
  enum IncludeType
  {
    MainFile,         ///< This is used for the very first access to the main source file
    RelativeInclude,  ///< An #include "file" has been encountered
    GlobalInclude     ///< An #include <file> has been encountered
  };

  /// \brief This type of callback is used to read an #include file. \a szAbsoluteFile is the path that the FileLocatorCB reported, the result needs to be stored in \a FileContent.
  typedef ezDelegate<ezResult (const char* szAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent, ezTimestamp& out_FileModification)> FileOpenCB;

  /// \brief This type of callback is used to retrieve the absolute path of the \a szIncludeFile when #included inside \a szCurAbsoluteFile.
  ///
  /// Note that you should ensure that \a out_sAbsoluteFilePath is always identical (including casing and path slashes) when it is supposed to point
  /// to the same file, as this exact name is used for file lookup (and therefore also file caching).
  /// If it is not identical, file caching will not work, and on different OSes the file may be found or not.
  typedef ezDelegate<ezResult (const char* szCurAbsoluteFile, const char* szIncludeFile, IncludeType IncType, ezString& out_sAbsoluteFilePath)> FileLocatorCB;

  /// \brief Every time an unknown command (e.g. '#version') is encountered, this callback is used to determine whether the command shall be passed through.
  ///
  /// If the callback returns false, an error is generated and parsing fails. The callback thus acts as a whitelist for all commands that shall be passed through.
  typedef ezDelegate<bool (const char* szUnknownCommand)> PassThroughUnknownCmdCB;

  typedef ezHybridArray<const ezToken*, 32> TokenStream;
  typedef ezDeque<TokenStream> MacroParameters;

  /// \brief The event data that the processor broadcasts
  ///
  /// Please note that m_pToken contains a lot of interesting information, such as
  /// the current file and line number and of course the current piece of text.
  struct ProcessingEvent
  {
    /// \brief The event types that the processor broadcasts
    enum EventType
    {
      BeginExpansion,   ///< A macro is now going to be expanded
      EndExpansion,     ///< A macro is finished being expanded
      Error,            ///< An error was encountered
      Warning,          ///< A warning has been output.
      CheckDefined,     ///< A 'defined(X)' is being evaluated
      CheckIfdef,       ///< A '#ifdef X' is being evaluated
      EvaluateUnknown,  ///< Inside an #if an unknown identifier has been encountered, it will be evaluated as zero
    };

    ProcessingEvent()
    {
      m_pToken = nullptr;
      m_szInfo = "";
    }

    EventType m_Type;

    const ezToken* m_pToken;
    const char* m_szInfo;
  };

  /// \brief Broadcasts events during the processing. This can be used to create detailed callstacks when an error is encountered.
  /// It also broadcasts errors and warnings with more detailed information than the log interface allows.
  ezEvent<const ProcessingEvent&> m_ProcessingEvents;

  ezPreprocessor();

  /// \brief All error output is sent to the given ezLogInterface.
  ///
  /// Note that when the preprocessor encounters any error, it will stop immediately and usually no output is generated.
  /// However, there are also a few cases where only a warning is generated, in this case preprocessing will continue without problems.
  ///
  /// Additionally errors and warnings are also broadcast through m_ProcessingEvents. So if you want to output more detailed information,
  /// that method should be preferred, because the events carry more information about the current file and line number etc.
  void SetLogInterface(ezLogInterface* pLog);

  /// \brief Allows to specify a custom cache object that should be used for storing the tokenized result of files.
  ///
  /// This allows to share one cache across multiple instances of ezPreprocessor and across time. E.g. it makes it possible
  /// to prevent having to read and tokenize include files that are referenced often.
  void SetCustomFileCache(ezTokenizedFileCache* pFileCache = nullptr);

  /// \brief If set to true, all #pragma commands are passed through to the output, otherwise they are removed.
  void SetPassThroughPragma(bool bPassThrough) { m_bPassThroughPragma = bPassThrough; }

  /// \brief If set to true, all #line commands are passed through to the output, otherwise they are removed.
  void SetPassThroughLine(bool bPassThrough) { m_bPassThroughLine = bPassThrough; }

  /// \brief Sets the callback that is used to determine whether an unknown command is passed through or triggers an error.
  void SetPassThroughUnknownCmdsCB(PassThroughUnknownCmdCB callback) { m_PassThroughUnknownCmdCB = callback; }

  /// \brief Sets the callback that is needed to read input data.
  ///
  /// The default file open function will just try to open files via ezFileReader.
  void SetFileOpenFunction(FileOpenCB OpenAbsFileCB);

  /// \brief Sets the callback that is needed to locate an input file
  ///
  /// The default file locator will assume that the main source file and all files #included in angle brackets can be opened without modification.
  /// Files #included in "" will be appended as relative paths to the path of the file they appeared in.
  void SetFileLocatorFunction(FileLocatorCB LocateAbsFileCB);

  /// \brief Adds a #define to the preprocessor, even before any file is processed.
  ///
  /// This allows to have global macros that are always defined for all processed files, such as the current platform etc.
  /// \a szDefinition must be in the form of the test that follows a #define statement. So to define the macro "WIN32", just
  /// pass that string. You can define any macro that could also be defined in the source files.
  ///
  /// If the definition is invalid, EZ_FAILURE is returned. Also the preprocessor might end up in an invalid state, so using it any
  /// further might fail (including crashing).
  ezResult AddCustomDefine(const char* szDefinition);

  /// \brief Processes the given file and returns the result as a stream of tokens.
  ///
  /// This function is useful when you want to further process the output afterwards and thus need it in a tokenized form anyway.
  ezResult Process(const char* szMainFile, TokenStream& TokenOutput);

  /// \brief Processes the given file and returns the result as a string.
  ///
  /// This function creates a string from the tokenized result. If \a bKeepComments is true, all block and line comments
  /// are included in the output string, otherwise they are removed.
  ezResult Process(const char* szMainFile, ezStringBuilder& sOutput, bool bKeepComments = true);


private:
  struct FileData
  {
    FileData()
    {
      m_iCurrentLine = 1;
      m_iExpandDepth = 0;
    }

    ezString m_sVirtualFileName;
    ezString m_sFileName;
    ezInt32 m_iCurrentLine;
    ezInt32 m_iExpandDepth;
  };

  enum IfDefActivity
  {
    IsActive,
    IsInactive,
    WasActive,
  };

  struct CustomDefine
  {
    ezDynamicArray<ezUInt8> m_Content;
    ezTokenizer m_Tokenized;
  };

  bool m_bPassThroughPragma;
  bool m_bPassThroughLine;
  PassThroughUnknownCmdCB m_PassThroughUnknownCmdCB;

  // this file cache is used as long as the user does not provide his own
  ezTokenizedFileCache m_InternalFileCache;

  // pointer to the file cache that is in use
  ezTokenizedFileCache* m_pUsedFileCache;

  ezDeque<FileData> m_sCurrentFileStack;

  ezLogInterface* m_pLog;

  ezDeque<CustomDefine> m_CustomDefines;

  struct IfDefState
  {
    IfDefState(IfDefActivity ActiveState = IfDefActivity::IsActive) : m_ActiveState(ActiveState), m_bIsInElseClause(false) {}

    IfDefActivity m_ActiveState;
    bool m_bIsInElseClause;
  };

  ezDeque<IfDefState> m_IfdefActiveStack;

  ezResult ProcessFile(const char* szFile, TokenStream& TokenOutput);
  ezResult ProcessCmd(const TokenStream& Tokens, TokenStream& TokenOutput);

private: // *** File Handling ***
  ezResult OpenFile(const char* szFile, const ezTokenizer** pTokenizer);
  static ezResult DefaultFileLocator(const char* szCurAbsoluteFile, const char* szIncludeFile, ezPreprocessor::IncludeType IncType, ezString& out_sAbsoluteFilePath);
  static ezResult DefaultFileOpen(const char* szAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent, ezTimestamp& out_FileModification);

  FileOpenCB m_FileOpenCallback;
  FileLocatorCB m_FileLocatorCallback;
  ezSet<ezString> m_PragmaOnce;

private: // *** Macro Definition ***

  bool RemoveDefine(const char* szName);
  ezResult HandleDefine(const TokenStream& Tokens, ezUInt32& uiCurToken);

  struct MacroDefinition
  {
    MacroDefinition();

    const ezToken* m_MacroIdentifier;
    bool m_bIsFunction;
    bool m_bCurrentlyExpanding;
    bool m_bHasVarArgs;
    ezInt32 m_iNumParameters;
    TokenStream m_Replacement;
  };

  ezResult StoreDefine(const ezToken* pMacroNameToken, const TokenStream* pReplacementTokens, ezUInt32 uiFirstReplacementToken, ezInt32 iNumParameters, bool bUsesVarArgs);
  ezResult ExtractParameterName(const TokenStream& Tokens, ezUInt32& uiCurToken, ezString& sIdentifierName);

  ezMap<ezString, MacroDefinition> m_Macros;

  static const ezInt32 s_MacroParameter0 = ezTokenType::ENUM_COUNT + 2;
  static ezString s_ParamNames[32];
  ezToken m_ParameterTokens[32];

private: // *** #if condition parsing ***

  ezResult EvaluateCondition(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseCondition(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseFactor(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseExpressionMul(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseExpressionOr(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseExpressionAnd(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseExpressionPlus(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseExpressionShift(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseExpressionBitOr(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseExpressionBitAnd(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseExpressionBitXor(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);


private: // *** Parsing ***
  static void SkipWhitespace(const TokenStream& Tokens, ezUInt32& uiCurToken);
  static void SkipWhitespaceAndNewline(const TokenStream& Tokens, ezUInt32& uiCurToken);
  static bool IsEndOfLine(const TokenStream& Tokens, ezUInt32 uiCurToken, bool bIgnoreWhitespace);
  static void CopyRelevantTokens(const TokenStream& Source, ezUInt32 uiFirstSourceToken, TokenStream& Destination, bool bPreserveNewLines);
  ezResult CopyTokensAndEvaluateDefined(const TokenStream& Source, ezUInt32 uiFirstSourceToken, TokenStream& Destination);

  bool Accept(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken, ezUInt32* pAccepted = nullptr);
  bool Accept(const TokenStream& Tokens, ezUInt32& uiCurToken, ezTokenType::Enum Type, ezUInt32* pAccepted = nullptr);
  bool Accept(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken1, const char* szToken2, ezUInt32* pAccepted = nullptr);
  bool AcceptUnless(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken1, const char* szToken2, ezUInt32* pAccepted = nullptr);

  ezResult Expect(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken, ezUInt32* pAccepted = nullptr);
  ezResult Expect(const TokenStream& Tokens, ezUInt32& uiCurToken, ezTokenType::Enum Type, ezUInt32* pAccepted = nullptr);
  ezResult Expect(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken1, const char* szToken2, ezUInt32* pAccepted = nullptr);
  ezResult ExpectEndOfLine(const TokenStream& Tokens, ezUInt32& uiCurToken);

  void CopyTokensReplaceParams(const TokenStream& Source, ezUInt32 uiFirstSourceToken, TokenStream& Destination, const ezHybridArray<ezString, 16>& parameters);
  void CombineTokensToString(const TokenStream& Tokens, ezUInt32 uiCurToken, ezStringBuilder& sResult, bool bKeepComments = true);
  void CombineRelevantTokensToString(const TokenStream& Tokens, ezUInt32 uiCurToken, ezStringBuilder& sResult);

private: // *** Macro Expansion ***
  ezResult Expand(const TokenStream& Tokens, TokenStream& Output);
  ezResult ExpandOnce(const TokenStream& Tokens, TokenStream& Output);
  ezResult ExpandObjectMacro(MacroDefinition& Macro, TokenStream& Output, const ezToken* pMacroToken);
  ezResult ExpandFunctionMacro(MacroDefinition& Macro, const MacroParameters& Parameters, TokenStream& Output, const ezToken* pMacroToken);
  ezResult ExpandMacroParam(const ezToken& MacroToken, ezUInt32 uiParam, TokenStream& Output, const MacroDefinition& Macro);
  void PassThroughFunctionMacro(MacroDefinition& Macro, const MacroParameters& Parameters, TokenStream& Output);
  ezToken* AddCustomToken(const ezToken* pPrevious, const char* szNewText);
  void OutputNotExpandableMacro(MacroDefinition& Macro, TokenStream& Output);
  ezResult ExtractAllMacroParameters(const TokenStream& Tokens, ezUInt32& uiCurToken, ezDeque< TokenStream >& AllParameters);
  ezResult ExtractParameterValue(const TokenStream& Tokens, ezUInt32& uiCurToken, TokenStream& ParamTokens);

  ezResult InsertParameters(const TokenStream& Tokens, TokenStream& Output, const MacroDefinition& Macro);

  ezResult InsertStringifiedParameters(const TokenStream& Tokens, TokenStream& Output, const MacroDefinition& Macro);
  ezResult ConcatenateParameters(const TokenStream& Tokens, TokenStream& Output, const MacroDefinition& Macro);
  void MergeTokens(const ezToken* pFirst, const ezToken* pSecond, TokenStream& Output, const MacroDefinition& Macro);

  struct CustomToken
  {
    ezToken m_Token;
    ezString m_sIdentifierString;
  };

  enum TokenFlags : ezUInt32
  {
    NoFurtherExpansion = EZ_BIT(0),
  };

  ezToken m_TokenFile;
  ezToken m_TokenLine;
  const ezToken* m_TokenOpenParenthesis;
  const ezToken* m_TokenClosedParenthesis;
  const ezToken* m_TokenComma;
  
  ezDeque<const MacroParameters*> m_MacroParamStack;
  ezDeque<const MacroParameters*> m_MacroParamStackExpanded;
  ezDeque<CustomToken> m_CustomTokens;

private: // *** Other ***
  static void StringifyTokens(const TokenStream& Tokens, ezStringBuilder& sResult, bool bSurroundWithQuotes);
  ezToken* CreateStringifiedParameter(ezUInt32 uiParam, const ezToken* pParamToken, const MacroDefinition& Macro);

  ezResult HandleErrorDirective(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken);
  ezResult HandleWarningDirective(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken);
  ezResult HandleUndef(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken);

  ezResult HandleEndif(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken);
  ezResult HandleElif(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken);
  ezResult HandleIf(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken);
  ezResult HandleElse(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken);
  ezResult HandleIfdef(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken, bool bIsIfdef);
  ezResult HandleInclude(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken, TokenStream& TokenOutput);
  ezResult HandleLine(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken, TokenStream& TokenOutput);
};

#define PP_LOG0(Type, FormatStr, ErrorToken) \
  { \
    ProcessingEvent pe; \
    pe.m_Type = ProcessingEvent::Type; \
    pe.m_pToken = ErrorToken; \
    pe.m_szInfo = FormatStr; \
    if (pe.m_pToken->m_uiLine == 0 && pe.m_pToken->m_uiColumn == 0) \
    { \
      const_cast<ezToken*>(pe.m_pToken)->m_uiLine = m_sCurrentFileStack.PeekBack().m_iCurrentLine; \
      const_cast<ezToken*>(pe.m_pToken)->m_File.Assign(m_sCurrentFileStack.PeekBack().m_sVirtualFileName.GetData()); \
    } \
    m_ProcessingEvents.Broadcast(pe); \
      ezLog::Type(m_pLog, "File '%s', Line %u (%u): " FormatStr, pe.m_pToken->m_File.GetString().GetData(), pe.m_pToken->m_uiLine, pe.m_pToken->m_uiColumn); \
  }

#define PP_LOG(Type, FormatStr, ErrorToken, ...) \
  { \
    ProcessingEvent pe; \
    pe.m_Type = ProcessingEvent::Type; \
    pe.m_pToken = ErrorToken; \
    if (pe.m_pToken->m_uiLine == 0 && pe.m_pToken->m_uiColumn == 0) \
    { \
      const_cast<ezToken*>(pe.m_pToken)->m_uiLine = m_sCurrentFileStack.PeekBack().m_iCurrentLine; \
      const_cast<ezToken*>(pe.m_pToken)->m_File.Assign(m_sCurrentFileStack.PeekBack().m_sVirtualFileName.GetData()); \
    } \
    ezStringBuilder sInfo; \
    sInfo.Format(FormatStr, __VA_ARGS__); \
    pe.m_szInfo = sInfo.GetData(); \
    m_ProcessingEvents.Broadcast(pe); \
    ezLog::Type(m_pLog, "File '%s', Line %u (%u): " FormatStr, pe.m_pToken->m_File.GetString().GetData(), pe.m_pToken->m_uiLine, pe.m_pToken->m_uiColumn, __VA_ARGS__); \
  }
