#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>

ezString ezPreprocessor::s_ParamNames[32];

using namespace ezTokenParseUtils;

ezPreprocessor::ezPreprocessor()
  : m_ClassAllocator("ezPreprocessor", ezFoundation::GetDefaultAllocator())
  , m_CurrentFileStack(&m_ClassAllocator)
  , m_CustomDefines(&m_ClassAllocator)
  , m_IfdefActiveStack(&m_ClassAllocator)
  , m_Macros(ezCompareHelper<ezString256>(), &m_ClassAllocator)
  , m_MacroParamStack(&m_ClassAllocator)
  , m_MacroParamStackExpanded(&m_ClassAllocator)
  , m_CustomTokens(&m_ClassAllocator)
{
  SetCustomFileCache();
  m_pLog = nullptr;

  m_FileLocatorCallback = DefaultFileLocator;
  m_FileOpenCallback = DefaultFileOpen;

  ezStringBuilder s;
  for (ezUInt32 i = 0; i < 32; ++i)
  {
    s.SetFormat("__Param{0}__", i);
    s_ParamNames[i] = s;

    m_ParameterTokens[i].m_iType = s_iMacroParameter0 + i;
    m_ParameterTokens[i].m_DataView = s_ParamNames[i].GetView();
  }

  ezToken dummy;
  dummy.m_iType = ezTokenType::NonIdentifier;

  m_pTokenOpenParenthesis = AddCustomToken(&dummy, "(");
  m_pTokenClosedParenthesis = AddCustomToken(&dummy, ")");
  m_pTokenComma = AddCustomToken(&dummy, ",");
}

void ezPreprocessor::SetCustomFileCache(ezTokenizedFileCache* pFileCache)
{
  m_pUsedFileCache = &m_InternalFileCache;

  if (pFileCache != nullptr)
    m_pUsedFileCache = pFileCache;
}

ezToken* ezPreprocessor::AddCustomToken(const ezToken* pPrevious, const ezStringView& sNewText)
{
  CustomToken* pToken = &m_CustomTokens.ExpandAndGetRef();

  pToken->m_sIdentifierString = sNewText;
  pToken->m_Token = *pPrevious;
  pToken->m_Token.m_DataView = pToken->m_sIdentifierString;

  return &pToken->m_Token;
}

ezResult ezPreprocessor::ProcessFile(ezStringView sFile, TokenStream& TokenOutput, const ezToken* pCurParentToken)
{
  const ezTokenizer* pTokenizer = nullptr;

  if (OpenFile(sFile, &pTokenizer).Failed())
  {
    if (pCurParentToken)
    {
      PP_LOG(Error, "Invalid #include '{}'", pCurParentToken, sFile);
    }

    return EZ_FAILURE;
  }

  FileData fd;
  fd.m_sFileName.Assign(sFile);
  fd.m_sVirtualFileName = fd.m_sFileName;

  m_CurrentFileStack.PushBack(fd);

  ezUInt32 uiNextToken = 0;
  TokenStream TokensLine(&m_ClassAllocator);
  TokenStream TokensCode(&m_ClassAllocator);

  while (pTokenizer->GetNextLine(uiNextToken, TokensLine).Succeeded())
  {
    ezUInt32 uiCurToken = 0;

    // if the line starts with a # it is a preprocessor command
    if (Accept(TokensLine, uiCurToken, "#"))
    {
      // code that was not yet expanded before the command -> expand now
      if (!TokensCode.IsEmpty())
      {
        if (Expand(TokensCode, TokenOutput).Failed())
          return EZ_FAILURE;

        TokensCode.Clear();
      }

      // process the command
      if (ProcessCmd(TokensLine, TokenOutput).Failed())
        return EZ_FAILURE;
    }
    else
    {
      // we are currently inside an inactive text block
      if (m_IfdefActiveStack.PeekBack().m_ActiveState != IfDefActivity::IsActive)
        continue;

      // store for later expansion
      TokensCode.PushBackRange(TokensLine);
    }
  }

  // some remaining code at the end -> expand
  if (!TokensCode.IsEmpty())
  {
    if (Expand(TokensCode, TokenOutput).Failed())
      return EZ_FAILURE;

    TokensCode.Clear();
  }

  m_CurrentFileStack.PopBack();

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::Process(ezStringView sMainFile, TokenStream& ref_tokenOutput)
{
  EZ_ASSERT_DEV(m_FileLocatorCallback.IsValid(), "No file locator callback has been set.");

  ref_tokenOutput.Clear();

  // Add a custom define for the __FILE__ macro
  {
    m_TokenFile.m_DataView = ezStringView("__FILE__");
    m_TokenFile.m_iType = ezTokenType::Identifier;

    MacroDefinition md;
    md.m_MacroIdentifier = &m_TokenFile;
    md.m_bIsFunction = false;
    md.m_uiNumParameters = 0;
    md.m_bHasVarArgs = false;

    m_Macros.Insert("__FILE__", md);
  }

  // Add a custom define for the __LINE__ macro
  {
    m_TokenLine.m_DataView = ezStringView("__LINE__");
    m_TokenLine.m_iType = ezTokenType::Identifier;

    MacroDefinition md;
    md.m_MacroIdentifier = &m_TokenLine;
    md.m_bIsFunction = false;
    md.m_uiNumParameters = 0;
    md.m_bHasVarArgs = false;

    m_Macros.Insert("__LINE__", md);
  }

  m_IfdefActiveStack.Clear();
  m_IfdefActiveStack.PushBack(IfDefActivity::IsActive);

  ezStringBuilder sFileToOpen;
  if (m_FileLocatorCallback("", sMainFile, IncludeType::MainFile, sFileToOpen).Failed())
  {
    ezLog::Error(m_pLog, "Could not locate file '{0}'", sMainFile);
    return EZ_FAILURE;
  }

  if (ProcessFile(sFileToOpen, ref_tokenOutput, nullptr).Failed())
    return EZ_FAILURE;

  m_IfdefActiveStack.PopBack();

  if (!m_IfdefActiveStack.IsEmpty())
  {
    ezLog::Error(m_pLog, "Incomplete nesting of #if / #else / #endif");
    return EZ_FAILURE;
  }

  if (!m_CurrentFileStack.IsEmpty())
  {
    ezLog::Error(m_pLog, "Internal error, file stack is not empty after processing. {0} elements, top stack item: '{1}'", m_CurrentFileStack.GetCount(), m_CurrentFileStack.PeekBack().m_sFileName);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::Process(ezStringView sMainFile, ezStringBuilder& ref_sOutput, bool bKeepComments, bool bRemoveRedundantWhitespace, bool bInsertLine)
{
  ref_sOutput.Clear();

  TokenStream TokenOutput;
  if (Process(sMainFile, TokenOutput).Failed())
    return EZ_FAILURE;

  // generate the final text output
  CombineTokensToString(TokenOutput, 0, ref_sOutput, bKeepComments, bRemoveRedundantWhitespace, bInsertLine);

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ProcessCmd(const TokenStream& Tokens, TokenStream& TokenOutput)
{
  ezUInt32 uiCurToken = 0;

  ezUInt32 uiHashToken = 0;

  if (Expect(Tokens, uiCurToken, "#", &uiHashToken).Failed())
    return EZ_FAILURE;

  // just a single hash sign is a valid preprocessor line
  if (IsEndOfLine(Tokens, uiCurToken, true))
    return EZ_SUCCESS;

  ezUInt32 uiAccepted = uiCurToken;

  // if there is a #pragma once anywhere in the file (not only the active part), it will be flagged to not be included again
  // this is actually more efficient than include guards, because the file is never even looked at again, thus macro expansion
  // does not take place each and every time (which is unfortunately necessary with include guards)
  {
    ezUInt32 uiTempPos = uiCurToken;
    if (Accept(Tokens, uiTempPos, "pragma") && Accept(Tokens, uiTempPos, "once"))
    {
      uiCurToken = uiTempPos;
      m_PragmaOnce.Insert(m_CurrentFileStack.PeekBack().m_sFileName);

      // rather pointless to pass this through, as the output ends up as one big file
      // if (m_bPassThroughPragma)
      //  CopyRelevantTokens(Tokens, uiHashToken, TokenOutput);

      return ExpectEndOfLine(Tokens, uiCurToken);
    }
  }

  if (Accept(Tokens, uiCurToken, "ifdef", &uiAccepted))
    return HandleIfdef(Tokens, uiCurToken, uiAccepted, true);

  if (Accept(Tokens, uiCurToken, "ifndef", &uiAccepted))
    return HandleIfdef(Tokens, uiCurToken, uiAccepted, false);

  if (Accept(Tokens, uiCurToken, "else", &uiAccepted))
    return HandleElse(Tokens, uiCurToken, uiAccepted);

  if (Accept(Tokens, uiCurToken, "if", &uiAccepted))
    return HandleIf(Tokens, uiCurToken, uiAccepted);

  if (Accept(Tokens, uiCurToken, "elif", &uiAccepted))
    return HandleElif(Tokens, uiCurToken, uiAccepted);

  if (Accept(Tokens, uiCurToken, "endif", &uiAccepted))
    return HandleEndif(Tokens, uiCurToken, uiAccepted);

  // we are currently inside an inactive text block, so skip all the following commands
  if (m_IfdefActiveStack.PeekBack().m_ActiveState != IfDefActivity::IsActive)
  {
    // check that the following command is valid, even if it is ignored
    if (Accept(Tokens, uiCurToken, "line", &uiAccepted) || Accept(Tokens, uiCurToken, "include", &uiAccepted) || Accept(Tokens, uiCurToken, "define") || Accept(Tokens, uiCurToken, "undef", &uiAccepted) || Accept(Tokens, uiCurToken, "error", &uiAccepted) ||
        Accept(Tokens, uiCurToken, "warning", &uiAccepted) || Accept(Tokens, uiCurToken, "pragma"))
      return EZ_SUCCESS;

    if (m_PassThroughUnknownCmdCB.IsValid())
    {
      ezString sCmd = Tokens[uiCurToken]->m_DataView;

      if (m_PassThroughUnknownCmdCB(sCmd))
        return EZ_SUCCESS;
    }

    PP_LOG0(Error, "Expected a preprocessor command", Tokens[0]);
    return EZ_FAILURE;
  }

  if (Accept(Tokens, uiCurToken, "line", &uiAccepted))
    return HandleLine(Tokens, uiCurToken, uiHashToken, TokenOutput);

  if (Accept(Tokens, uiCurToken, "include", &uiAccepted))
    return HandleInclude(Tokens, uiCurToken, uiAccepted, TokenOutput);

  if (Accept(Tokens, uiCurToken, "define"))
    return HandleDefine(Tokens, uiCurToken);

  if (Accept(Tokens, uiCurToken, "undef", &uiAccepted))
    return HandleUndef(Tokens, uiCurToken, uiAccepted);

  if (Accept(Tokens, uiCurToken, "error", &uiAccepted))
    return HandleErrorDirective(Tokens, uiCurToken, uiAccepted);

  if (Accept(Tokens, uiCurToken, "warning", &uiAccepted))
    return HandleWarningDirective(Tokens, uiCurToken, uiAccepted);

  // Pass #line and #pragma commands through unmodified, the user expects them to arrive in the final output properly
  if (Accept(Tokens, uiCurToken, "pragma"))
  {
    if (m_bPassThroughPragma)
      CopyRelevantTokens(Tokens, uiHashToken, TokenOutput, true);

    return EZ_SUCCESS;
  }

  if (m_PassThroughUnknownCmdCB.IsValid())
  {
    ezString sCmd = Tokens[uiCurToken]->m_DataView;

    if (m_PassThroughUnknownCmdCB(sCmd))
    {
      TokenOutput.PushBackRange(Tokens);
      return EZ_SUCCESS;
    }
  }

  PP_LOG0(Error, "Expected a preprocessor command", Tokens[0]);
  return EZ_FAILURE;
}

ezResult ezPreprocessor::HandleLine(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiHashToken, TokenStream& TokenOutput)
{
  // #line directives are just passed through, the actual #line detection is already done by the tokenizer
  // however we check them for validity here

  if (m_bPassThroughLine)
    CopyRelevantTokens(Tokens, uiHashToken, TokenOutput, true);

  ezUInt32 uiNumberToken = 0;
  if (Expect(Tokens, uiCurToken, ezTokenType::Integer, &uiNumberToken).Failed())
    return EZ_FAILURE;

  ezInt32 iNextLine = 0;

  const ezString sNumber = Tokens[uiNumberToken]->m_DataView;
  if (ezConversionUtils::StringToInt(sNumber, iNextLine).Failed())
  {
    PP_LOG(Error, "Could not parse '{0}' as a line number", Tokens[uiNumberToken], sNumber);
    return EZ_FAILURE;
  }

  ezUInt32 uiFileNameToken = 0;
  if (Accept(Tokens, uiCurToken, ezTokenType::String1, &uiFileNameToken))
  {
    // ezStringBuilder sFileName = Tokens[uiFileNameToken]->m_DataView;
    // sFileName.Shrink(1, 1); // remove surrounding "
    // m_CurrentFileStack.PeekBack().m_sVirtualFileName = sFileName;
  }
  else
  {
    if (ExpectEndOfLine(Tokens, uiCurToken).Failed())
      return EZ_FAILURE;
  }

  // there is one case that is not handled here:
  // when the #line directive appears other than '#line number [file]', then the other parameters should be expanded
  // and then checked again for the above form
  // since this is probably not in common use, we ignore this case

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::HandleIfdef(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken, bool bIsIfdef)
{
  EZ_IGNORE_UNUSED(uiDirectiveToken);

  if (m_IfdefActiveStack.PeekBack().m_ActiveState != IfDefActivity::IsActive)
  {
    m_IfdefActiveStack.PushBack(IfDefActivity::IsInactive);
    return EZ_SUCCESS;
  }

  ezUInt32 uiIdentifier = uiCurToken;
  if (Expect(Tokens, uiCurToken, ezTokenType::Identifier, &uiIdentifier).Failed())
    return EZ_FAILURE;

  const bool bDefined = m_Macros.Find(Tokens[uiIdentifier]->m_DataView).IsValid();

  // broadcast that '#ifdef' is being evaluated
  {
    ProcessingEvent pe;
    pe.m_pToken = Tokens[uiIdentifier];
    pe.m_Type = bIsIfdef ? ProcessingEvent::CheckIfdef : ProcessingEvent::CheckIfndef;
    pe.m_sInfo = bDefined ? "defined" : "undefined";
    m_ProcessingEvents.Broadcast(pe);
  }

  m_IfdefActiveStack.PushBack(bIsIfdef == bDefined ? IfDefActivity::IsActive : IfDefActivity::IsInactive);

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::HandleElse(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken)
{
  EZ_IGNORE_UNUSED(uiCurToken);

  const IfDefActivity bCur = m_IfdefActiveStack.PeekBack().m_ActiveState;
  m_IfdefActiveStack.PopBack();

  if (m_IfdefActiveStack.IsEmpty())
  {
    PP_LOG0(Error, "Unexpected '#else'", Tokens[uiDirectiveToken]);
    return EZ_FAILURE;
  }

  if (m_IfdefActiveStack.PeekBack().m_bIsInElseClause)
  {
    PP_LOG0(Error, "Unexpected '#else'", Tokens[uiDirectiveToken]);
    return EZ_FAILURE;
  }

  m_IfdefActiveStack.PeekBack().m_bIsInElseClause = true;

  if (m_IfdefActiveStack.PeekBack().m_ActiveState != IfDefActivity::IsActive)
  {
    m_IfdefActiveStack.PushBack(IfDefActivity::IsInactive);
    return EZ_SUCCESS;
  }

  if (bCur == IfDefActivity::WasActive || bCur == IfDefActivity::IsActive)
    m_IfdefActiveStack.PushBack(IfDefActivity::WasActive);
  else
    m_IfdefActiveStack.PushBack(IfDefActivity::IsActive);

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::HandleIf(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken)
{
  EZ_IGNORE_UNUSED(uiDirectiveToken);

  if (m_IfdefActiveStack.PeekBack().m_ActiveState != IfDefActivity::IsActive)
  {
    m_IfdefActiveStack.PushBack(IfDefActivity::IsInactive);
    return EZ_SUCCESS;
  }

  ezInt64 iResult = 0;

  if (EvaluateCondition(Tokens, uiCurToken, iResult).Failed())
    return EZ_FAILURE;

  m_IfdefActiveStack.PushBack(iResult != 0 ? IfDefActivity::IsActive : IfDefActivity::IsInactive);
  return EZ_SUCCESS;
}

ezResult ezPreprocessor::HandleElif(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken)
{
  const IfDefActivity Cur = m_IfdefActiveStack.PeekBack().m_ActiveState;
  m_IfdefActiveStack.PopBack();

  if (m_IfdefActiveStack.IsEmpty())
  {
    PP_LOG0(Error, "Unexpected '#elif'", Tokens[uiDirectiveToken]);
    return EZ_FAILURE;
  }

  if (m_IfdefActiveStack.PeekBack().m_bIsInElseClause)
  {
    PP_LOG0(Error, "Unexpected '#elif'", Tokens[uiDirectiveToken]);
    return EZ_FAILURE;
  }

  if (m_IfdefActiveStack.PeekBack().m_ActiveState != IfDefActivity::IsActive)
  {
    m_IfdefActiveStack.PushBack(IfDefActivity::IsInactive);
    return EZ_SUCCESS;
  }

  ezInt64 iResult = 0;
  if (EvaluateCondition(Tokens, uiCurToken, iResult).Failed())
    return EZ_FAILURE;

  if (Cur != IfDefActivity::IsInactive)
  {
    m_IfdefActiveStack.PushBack(IfDefActivity::WasActive);
    return EZ_SUCCESS;
  }

  m_IfdefActiveStack.PushBack(iResult != 0 ? IfDefActivity::IsActive : IfDefActivity::IsInactive);
  return EZ_SUCCESS;
}

ezResult ezPreprocessor::HandleEndif(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken)
{
  SkipWhitespace(Tokens, uiCurToken);

  EZ_SUCCEED_OR_RETURN(ExpectEndOfLine(Tokens, uiCurToken));

  m_IfdefActiveStack.PopBack();

  if (m_IfdefActiveStack.IsEmpty())
  {
    PP_LOG0(Error, "Unexpected '#endif'", Tokens[uiDirectiveToken]);
    return EZ_FAILURE;
  }
  else
  {
    m_IfdefActiveStack.PeekBack().m_bIsInElseClause = false;
  }

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::HandleUndef(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken)
{
  EZ_IGNORE_UNUSED(uiDirectiveToken);

  ezUInt32 uiIdentifierToken = uiCurToken;

  if (Expect(Tokens, uiCurToken, ezTokenType::Identifier, &uiIdentifierToken).Failed())
    return EZ_FAILURE;

  const ezString sUndef = Tokens[uiIdentifierToken]->m_DataView;
  if (!RemoveDefine(sUndef))
  {
    PP_LOG(Warning, "'#undef' of undefined macro '{0}'", Tokens[uiIdentifierToken], sUndef);
    return EZ_SUCCESS;
  }

  // this is an error, but not one that will cause it to fail
  ExpectEndOfLine(Tokens, uiCurToken).IgnoreResult();

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::HandleErrorDirective(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken)
{
  SkipWhitespace(Tokens, uiCurToken);

  ezStringBuilder sTemp;
  CombineTokensToString(Tokens, uiCurToken, sTemp);

  while (sTemp.EndsWith("\n") || sTemp.EndsWith("\r"))
    sTemp.Shrink(0, 1);

  PP_LOG(Error, "#error '{0}'", Tokens[uiDirectiveToken], sTemp);

  return EZ_FAILURE;
}

ezResult ezPreprocessor::HandleWarningDirective(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken)
{
  SkipWhitespace(Tokens, uiCurToken);

  ezStringBuilder sTemp;
  CombineTokensToString(Tokens, uiCurToken, sTemp);

  while (sTemp.EndsWith("\n") || sTemp.EndsWith("\r"))
    sTemp.Shrink(0, 1);

  PP_LOG(Warning, "#warning '{0}'", Tokens[uiDirectiveToken], sTemp);

  return EZ_SUCCESS;
}
