#pragma once

#include <Foundation/Basics.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Memory/CommonAllocators.h>

/// \brief This object caches files in a tokenized state. It can be shared among ezPreprocessor instances to improve performance when
/// they access the same files.
class EZ_FOUNDATION_DLL ezTokenizedFileCache
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
  const ezTokenizer* Tokenize(const ezString& sFileName, ezArrayPtr<const ezUInt8> FileContent, const ezTimestamp& FileTimeStamp, ezLogInterface* pLog);

private:
  void SkipWhitespace(ezDeque<ezToken>& Tokens, ezUInt32& uiCurToken);

  mutable ezMutex m_Mutex;
  ezMap<ezString, FileData> m_Cache;
};

/// \brief ezPreprocessor implements a standard C preprocessor. It can be used to pre-process files to get the output after macro expansion and #ifdef handling.
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
class EZ_FOUNDATION_DLL ezPreprocessor
{
public:
  /// \brief Describes the type of #include that was encountered during preprocessing
  enum IncludeType
  {
    MainFile,        ///< This is used for the very first access to the main source file
    RelativeInclude, ///< An #include "file" has been encountered
    GlobalInclude    ///< An #include <file> has been encountered
  };

  /// \brief This type of callback is used to read an #include file. \a szAbsoluteFile is the path that the FileLocatorCB reported, the result needs to be stored in \a FileContent.
  typedef ezDelegate<ezResult(const char* szAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent, ezTimestamp& out_FileModification)> FileOpenCB;

  /// \brief This type of callback is used to retrieve the absolute path of the \a szIncludeFile when #included inside \a szCurAbsoluteFile.
  ///
  /// Note that you should ensure that \a out_sAbsoluteFilePath is always identical (including casing and path slashes) when it is supposed to point
  /// to the same file, as this exact name is used for file lookup (and therefore also file caching).
  /// If it is not identical, file caching will not work, and on different OSes the file may be found or not.
  typedef ezDelegate<ezResult(const char* szCurAbsoluteFile, const char* szIncludeFile, IncludeType IncType, ezStringBuilder& out_sAbsoluteFilePath)> FileLocatorCB;

  /// \brief Every time an unknown command (e.g. '#version') is encountered, this callback is used to determine whether the command shall be passed through.
  ///
  /// If the callback returns false, an error is generated and parsing fails. The callback thus acts as a whitelist for all commands that shall be passed through.
  typedef ezDelegate<bool(const char* szUnknownCommand)> PassThroughUnknownCmdCB;

  typedef ezDeque<ezTokenParseUtils::TokenStream> MacroParameters;

  /// \brief The event data that the processor broadcasts
  ///
  /// Please note that m_pToken contains a lot of interesting information, such as
  /// the current file and line number and of course the current piece of text.
  struct ProcessingEvent
  {
    /// \brief The event types that the processor broadcasts
    enum EventType
    {
      BeginExpansion,  ///< A macro is now going to be expanded
      EndExpansion,    ///< A macro is finished being expanded
      Error,           ///< An error was encountered
      Warning,         ///< A warning has been output.
      CheckDefined,    ///< A 'defined(X)' is being evaluated
      CheckIfdef,      ///< A '#ifdef X' is being evaluated
      CheckIfndef,     ///< A '#ifndef X' is being evaluated
      EvaluateUnknown, ///< Inside an #if an unknown identifier has been encountered, it will be evaluated as zero
      Define,          ///< A #define X has been stored
      Redefine,        ///< A #define for an already existing macro name (also logged as a warning)
    };

    ProcessingEvent()
    {
      m_Type = Error;
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
  ezResult Process(const char* szMainFile, ezTokenParseUtils::TokenStream& TokenOutput);

  /// \brief Processes the given file and returns the result as a string.
  ///
  /// This function creates a string from the tokenized result. If \a bKeepComments is true, all block and line comments
  /// are included in the output string, otherwise they are removed.
  ezResult Process(const char* szMainFile, ezStringBuilder& sOutput, bool bKeepComments = true, bool bRemoveRedundantWhitespace = false, bool bInsertLine = false);


private:
  struct FileData
  {
    FileData()
    {
      m_iCurrentLine = 1;
      m_iExpandDepth = 0;
    }

    ezHashedString m_sVirtualFileName;
    ezHashedString m_sFileName;
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
    ezHybridArray<ezUInt8, 64> m_Content;
    ezTokenizer m_Tokenized;
  };

  // This class-local allocator is used to get rid of some of the memory allocation
  // tracking that would otherwise occur for allocations made by the preprocessor.
  // If changing its position in the class, make sure it always comes before all
  // other members that depend on it to ensure deallocations in those members
  // happen before the allocator get destroyed.
  ezAllocator<ezMemoryPolicies::ezHeapAllocation, ezMemoryTrackingFlags::None> m_ClassAllocator;

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

  ezResult ProcessFile(const char* szFile, ezTokenParseUtils::TokenStream& TokenOutput);
  ezResult ProcessCmd(const ezTokenParseUtils::TokenStream& Tokens, ezTokenParseUtils::TokenStream& TokenOutput);

private: // *** File Handling ***
  ezResult OpenFile(const char* szFile, const ezTokenizer** pTokenizer);
  static ezResult DefaultFileLocator(const char* szCurAbsoluteFile, const char* szIncludeFile, ezPreprocessor::IncludeType IncType, ezStringBuilder& out_sAbsoluteFilePath);
  static ezResult DefaultFileOpen(const char* szAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent, ezTimestamp& out_FileModification);

  FileOpenCB m_FileOpenCallback;
  FileLocatorCB m_FileLocatorCallback;
  ezSet<ezTempHashedString> m_PragmaOnce;

private: // *** Macro Definition ***
  bool RemoveDefine(const char* szName);
  ezResult HandleDefine(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken);

  struct MacroDefinition
  {
    MacroDefinition();

    const ezToken* m_MacroIdentifier;
    bool m_bIsFunction;
    bool m_bCurrentlyExpanding;
    bool m_bHasVarArgs;
    ezInt32 m_iNumParameters;
    ezTokenParseUtils::TokenStream m_Replacement;
  };

  ezResult StoreDefine(const ezToken* pMacroNameToken, const ezTokenParseUtils::TokenStream* pReplacementTokens, ezUInt32 uiFirstReplacementToken, ezInt32 iNumParameters, bool bUsesVarArgs);
  ezResult ExtractParameterName(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, ezString& sIdentifierName);

  ezMap<ezString256, MacroDefinition> m_Macros;

  static const ezInt32 s_MacroParameter0 = ezTokenType::ENUM_COUNT + 2;
  static ezString s_ParamNames[32];
  ezToken m_ParameterTokens[32];

private: // *** #if condition parsing ***
  ezResult EvaluateCondition(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseCondition(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseFactor(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseExpressionMul(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseExpressionOr(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseExpressionAnd(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseExpressionPlus(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseExpressionShift(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseExpressionBitOr(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseExpressionBitAnd(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);
  ezResult ParseExpressionBitXor(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult);


private: // *** Parsing ***
  ezResult CopyTokensAndEvaluateDefined(const ezTokenParseUtils::TokenStream& Source, ezUInt32 uiFirstSourceToken, ezTokenParseUtils::TokenStream& Destination);
  void CopyTokensReplaceParams(const ezTokenParseUtils::TokenStream& Source, ezUInt32 uiFirstSourceToken, ezTokenParseUtils::TokenStream& Destination, const ezHybridArray<ezString, 16>& parameters);

  ezResult Expect(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken, ezUInt32* pAccepted = nullptr);
  ezResult Expect(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, ezTokenType::Enum Type, ezUInt32* pAccepted = nullptr);
  ezResult Expect(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken1, const char* szToken2, ezUInt32* pAccepted = nullptr);
  ezResult ExpectEndOfLine(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken);

private: // *** Macro Expansion ***
  ezResult Expand(const ezTokenParseUtils::TokenStream& Tokens, ezTokenParseUtils::TokenStream& Output);
  ezResult ExpandOnce(const ezTokenParseUtils::TokenStream& Tokens, ezTokenParseUtils::TokenStream& Output);
  ezResult ExpandObjectMacro(MacroDefinition& Macro, ezTokenParseUtils::TokenStream& Output, const ezToken* pMacroToken);
  ezResult ExpandFunctionMacro(MacroDefinition& Macro, const MacroParameters& Parameters, ezTokenParseUtils::TokenStream& Output, const ezToken* pMacroToken);
  ezResult ExpandMacroParam(const ezToken& MacroToken, ezUInt32 uiParam, ezTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);
  void PassThroughFunctionMacro(MacroDefinition& Macro, const MacroParameters& Parameters, ezTokenParseUtils::TokenStream& Output);
  ezToken* AddCustomToken(const ezToken* pPrevious, const ezStringView& sNewText);
  void OutputNotExpandableMacro(MacroDefinition& Macro, ezTokenParseUtils::TokenStream& Output);
  ezResult ExtractAllMacroParameters(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, ezDeque<ezTokenParseUtils::TokenStream>& AllParameters);
  ezResult ExtractParameterValue(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32& uiCurToken, ezTokenParseUtils::TokenStream& ParamTokens);

  ezResult InsertParameters(const ezTokenParseUtils::TokenStream& Tokens, ezTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);

  ezResult InsertStringifiedParameters(const ezTokenParseUtils::TokenStream& Tokens, ezTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);
  ezResult ConcatenateParameters(const ezTokenParseUtils::TokenStream& Tokens, ezTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);
  void MergeTokens(const ezToken* pFirst, const ezToken* pSecond, ezTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);

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
  static void StringifyTokens(const ezTokenParseUtils::TokenStream& Tokens, ezStringBuilder& sResult, bool bSurroundWithQuotes);
  ezToken* CreateStringifiedParameter(ezUInt32 uiParam, const ezToken* pParamToken, const MacroDefinition& Macro);

  ezResult HandleErrorDirective(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken);
  ezResult HandleWarningDirective(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken);
  ezResult HandleUndef(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken);

  ezResult HandleEndif(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken);
  ezResult HandleElif(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken);
  ezResult HandleIf(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken);
  ezResult HandleElse(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken);
  ezResult HandleIfdef(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken, bool bIsIfdef);
  ezResult HandleInclude(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken, ezTokenParseUtils::TokenStream& TokenOutput);
  ezResult HandleLine(const ezTokenParseUtils::TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken, ezTokenParseUtils::TokenStream& TokenOutput);
};

#define PP_LOG0(Type, FormatStr, ErrorToken)                                                                                                        \
  {                                                                                                                                                 \
    ProcessingEvent pe;                                                                                                                             \
    pe.m_Type = ProcessingEvent::Type;                                                                                                              \
    pe.m_pToken = ErrorToken;                                                                                                                       \
    pe.m_szInfo = FormatStr;                                                                                                                        \
    if (pe.m_pToken->m_uiLine == 0 && pe.m_pToken->m_uiColumn == 0)                                                                                 \
    {                                                                                                                                               \
      const_cast<ezToken*>(pe.m_pToken)->m_uiLine = m_sCurrentFileStack.PeekBack().m_iCurrentLine;                                                  \
      const_cast<ezToken*>(pe.m_pToken)->m_File.Assign(m_sCurrentFileStack.PeekBack().m_sVirtualFileName.GetData());                                \
    }                                                                                                                                               \
    m_ProcessingEvents.Broadcast(pe);                                                                                                               \
    ezLog::Type(m_pLog, "File '{0}', Line {1} ({2}): " FormatStr, pe.m_pToken->m_File.GetString(), pe.m_pToken->m_uiLine, pe.m_pToken->m_uiColumn); \
  }

#define PP_LOG(Type, FormatStr, ErrorToken, ...)                                                                                                    \
  {                                                                                                                                                 \
    ProcessingEvent _pe;                                                                                                                             \
    _pe.m_Type = ProcessingEvent::Type;                                                                                                             \
    _pe.m_pToken = ErrorToken;                                                                                                                       \
    if (_pe.m_pToken->m_uiLine == 0 && _pe.m_pToken->m_uiColumn == 0)                                                                                 \
    {                                                                                                                                               \
      const_cast<ezToken*>(_pe.m_pToken)->m_uiLine = m_sCurrentFileStack.PeekBack().m_iCurrentLine;                                                  \
      const_cast<ezToken*>(_pe.m_pToken)->m_File.Assign(m_sCurrentFileStack.PeekBack().m_sVirtualFileName.GetData());                                \
    }                                                                                                                                               \
    ezStringBuilder sInfo;                                                                                                                          \
    sInfo.Format(FormatStr, ##__VA_ARGS__);                                                                                                         \
    _pe.m_szInfo = sInfo.GetData();                                                                                                                  \
    m_ProcessingEvents.Broadcast(_pe);                                                                                                               \
    ezLog::Type(m_pLog, "File '{0}', Line {1} ({2}): {3}", _pe.m_pToken->m_File.GetString(), _pe.m_pToken->m_uiLine, _pe.m_pToken->m_uiColumn, sInfo); \
  }

