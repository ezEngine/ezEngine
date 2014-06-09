#include <CoreUtils/Basics.h>
#include <CoreUtils/CodeUtils/Tokenizer.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>

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

public: // *** General ***
  ezPreprocessor();

  void SetLogInterface(ezLogInterface* pLog);

  ezResult Process(const char* szMainFile, ezStringBuilder& sOutput);

  ezResult Process(const char* szMainFile, TokenStream& TokenOutput);

public: // *** File Handling ***
  void SetFileCallbacks(FileOpenCB OpenAbsFileCB, FileLocatorCB LocateAbsFileCB);

  ezResult ProcessFile(const char* szFile, TokenStream& TokenOutput);

private:

  struct FileData
  {
    FileData()
    {
      m_iCurrentLine = 1;
      m_iExpandDepth = 0;
      m_iLineOffset = 0;
    }

    ezString m_sFileName;
    ezInt32 m_iCurrentLine;
    ezInt32 m_iExpandDepth;
    ezInt32 m_iLineOffset;
  };

  ezDeque<FileData> m_sCurrentFileStack;

public: // *** Macro Definition ***

  void ClearDefines();
  bool RemoveDefine(const char* szName);
  ezResult AddDefine(const TokenStream& Tokens, ezUInt32& uiCurToken);

private: // *** General ***
  ezLogInterface* m_pLog;

  enum IfDefActivity
  {
    IsActive,
    IsInactive,
    WasActive,
  };

  ezDeque<ezInt32> m_IfdefActiveStack;

  ezResult ProcessCmd(const TokenStream& Tokens, TokenStream& TokenOutput);

private: // *** File Handling ***
  ezResult OpenFile(const char* szFile, ezTokenizer** pTokenizer);

  FileOpenCB m_FileOpenCallback;
  FileLocatorCB m_FileLocatorCallback;
  ezMap<ezString, ezTokenizer> m_FileCache;
  ezSet<ezString> m_PragmaOnce;

private: // *** Macro Definition ***

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
  void CombineTokensToString(const TokenStream& Tokens, ezUInt32 uiCurToken, ezStringBuilder& sResult);
  void CombineRelevantTokensToString(const TokenStream& Tokens, ezUInt32 uiCurToken, ezStringBuilder& sResult);

private: // *** Macro Expansion ***
  ezResult Expand(const TokenStream& Tokens, TokenStream& Output);
  ezResult ExpandOnce(const TokenStream& Tokens, TokenStream& Output);
  ezResult ExpandObjectMacro(MacroDefinition& Macro, TokenStream& Output, const ezToken* pMacroToken);
  ezResult ExpandFunctionMacro(MacroDefinition& Macro, const MacroParameters& Parameters, TokenStream& Output);
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
  ezResult HandleLine(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken);
};

#define PP_LOG0(Type, FormatStr, ErrorToken) \
  ezLog::Type(m_pLog, "Line %u (%u): " FormatStr, ErrorToken->m_uiLine, ErrorToken->m_uiColumn);

#define PP_LOG(Type, FormatStr, ErrorToken, ...) \
  ezLog::Type(m_pLog, "Line %u (%u): " FormatStr, ErrorToken->m_uiLine, ErrorToken->m_uiColumn, __VA_ARGS__);
