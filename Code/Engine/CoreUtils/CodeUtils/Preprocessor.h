#include <CoreUtils/Basics.h>
#include <CoreUtils/CodeUtils/Tokenizer.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>

class EZ_COREUTILS_DLL ezTokenizedFileCache
{
public:
  ezMap<ezString, ezTokenizer>::ConstIterator Lookup(const ezString& sFileName) const;

  void Remove(const ezString& sFileName);

  void Clear();

  const ezTokenizer* Tokenize(const ezString& sFileName, const ezDynamicArray<ezUInt8>& FileContent, ezLogInterface* pLog);

private:
  mutable ezMutex m_Mutex;
  ezMap<ezString, ezTokenizer> m_Cache;
};

class EZ_COREUTILS_DLL ezPreprocessor
{
public:
  enum IncludeType
  {
    MainFile,
    RelativeInclude,
    GlobalInclude
  };

  typedef ezResult (*FileOpenCB)(const char* szAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent);
  typedef ezResult (*FileLocatorCB)(const char* szCurAbsoluteFile, const char* szIncludeFile, IncludeType IncType, ezString& out_sAbsoluteFilePath);
  typedef ezHybridArray<const ezToken*, 32> TokenStream;
  typedef ezDeque<TokenStream> MacroParameters;

  struct ProcessingEvent
  {
    enum EventType
    {
      BeginExpansion,
      EndExpansion,
      Error,
      Warning,
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

  ezEvent<const ProcessingEvent&> m_ProcessingEvents;

  ezPreprocessor();

  /// \brief All error output is sent to the given ezLogInterface.
  ///
  /// Note that when the preprocessor encounters any error, it will stop immediately and usually no output is generated.
  /// However, there are also a few cases where only a warning is generated, in this case preprocessing will continue without problems.
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

  ezResult Process(const char* szMainFile, TokenStream& TokenOutput);

  ezResult Process(const char* szMainFile, ezStringBuilder& sOutput, bool bKeepComments = true);

  ezResult AddCustomDefine(const char* szDefinition);

  void SetFileCallbacks(FileOpenCB OpenAbsFileCB, FileLocatorCB LocateAbsFileCB);


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

  // this file cache is used as long as the user does not provide his own
  ezTokenizedFileCache m_InternalFileCache;

  // pointer to the file cache that is in use
  ezTokenizedFileCache* m_pUsedFileCache;

  ezDeque<FileData> m_sCurrentFileStack;

  ezLogInterface* m_pLog;

  ezDeque<CustomDefine> m_CustomDefines;
  ezDeque<ezInt32> m_IfdefActiveStack;

  ezResult ProcessFile(const char* szFile, TokenStream& TokenOutput);
  ezResult ProcessCmd(const TokenStream& Tokens, TokenStream& TokenOutput);

private: // *** File Handling ***
  ezResult OpenFile(const char* szFile, const ezTokenizer** pTokenizer);

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
  static void CopyRelevantTokens(const TokenStream& Source, ezUInt32 uiFirstSourceToken, TokenStream& Destination);
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
  //ezResult ValidCodeCheck(const TokenStream& Tokens);
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
  ProcessingEvent pe; \
  pe.m_Type = ProcessingEvent::Type; \
  pe.m_pToken = ErrorToken; \
  pe.m_szInfo = FormatStr; \
  m_ProcessingEvents.Broadcast(pe); \
    ezLog::Type(m_pLog, "File '%s', Line %u (%u): " FormatStr, ErrorToken->m_File.GetString().GetData(), ErrorToken->m_uiLine, ErrorToken->m_uiColumn);


#define PP_LOG(Type, FormatStr, ErrorToken, ...) \
  { \
    ProcessingEvent pe; \
    pe.m_Type = ProcessingEvent::Type; \
    pe.m_pToken = ErrorToken; \
    ezStringBuilder sInfo; \
    sInfo.Format(FormatStr, __VA_ARGS__); \
    pe.m_szInfo = sInfo.GetData(); \
    m_ProcessingEvents.Broadcast(pe); \
    ezLog::Type(m_pLog, "File '%s', Line %u (%u): " FormatStr, ErrorToken->m_File.GetString().GetData(), ErrorToken->m_uiLine, ErrorToken->m_uiColumn, __VA_ARGS__); \
  }
