#include <CoreUtils/PCH.h>
#include <CoreUtils/CodeUtils/Preprocessor.h>

// TODO:
// #include
// stringification
// concatenation
// varargs
// #pragma once
// __LINE__
// __FILE__

ezString ezPreprocessor::s_ParamNames[32];

ezPreprocessor::ezPreprocessor()
{
  m_FileOpenCallback = nullptr;
  m_pLog = nullptr;

  ezStringBuilder s;
  for (ezUInt32 i = 0; i < 32; ++i)
  {
    s.Format("__Param%u__", i);
    s_ParamNames[i] = s;

    m_ParameterTokens[i].m_iType = s_MacroParameter0 + i;
    m_ParameterTokens[i].m_DataView = ezStringIterator(s_ParamNames[i].GetData());
  }

  ezToken dummy;
  dummy.m_iType = ezTokenType::NonIdentifier;

  m_TokenOpenParenthesis    = AddCustomToken(&dummy, "(");
  m_TokenClosedParenthesis  = AddCustomToken(&dummy, ")");
  m_TokenComma              = AddCustomToken(&dummy, ",");

}

ezToken* ezPreprocessor::AddCustomToken(const ezToken* pPrevious, const char* szNewText)
{
  m_CustomTokens.SetCount(m_CustomTokens.GetCount() + 1);
  CustomToken* pToken = &m_CustomTokens.PeekBack();

  pToken->m_sIdentifierString = szNewText;
  pToken->m_Token = *pPrevious;
  pToken->m_Token.m_DataView = pToken->m_sIdentifierString.GetIteratorFront();

  return &pToken->m_Token;
}

ezResult ezPreprocessor::ProcessFile(const char* szFile, ezStringBuilder& sOutput)
{
  sOutput.Clear();

  ezTokenizer* pTokenizer = nullptr;

  m_IfdefActiveStack.Clear();
  m_IfdefActiveStack.PushBack(IfDefActivity::IsActive);

  if (OpenFile(szFile, &pTokenizer).Failed())
    return EZ_FAILURE;

  ezUInt32 uiNextToken = 0;
  ezHybridArray<const ezToken*, 32> TokensLine;
  ezHybridArray<const ezToken*, 32> TokensCode;
  ezHybridArray<const ezToken*, 32> TokenOutput;

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
      if (ProcessCmd(TokensLine, sOutput).Failed())
        return EZ_FAILURE;
    }
    else
    {
      // we are currently inside an inactive text block
      if (m_IfdefActiveStack.PeekBack() != IfDefActivity::IsActive)
        continue;

      if (ValidCodeCheck(TokensLine).Failed())
        return EZ_FAILURE;

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

  // generate the final text output
  CombineTokensToString(TokenOutput, 0, sOutput);

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ProcessCmd(const ezHybridArray<const ezToken*, 32>& Tokens, ezStringBuilder& sOutput)
{
  ezUInt32 uiCurToken = 0;

  if (Expect(Tokens, uiCurToken, "#").Failed())
    return EZ_FAILURE;

  // just a single hash sign is a valid preprocessor line
  if (IsEndOfLine(Tokens, uiCurToken, true))
    return EZ_SUCCESS;

  ezUInt32 uiAccepted = uiCurToken;
  if (Accept(Tokens, uiCurToken,  "ifdef", &uiAccepted) || Accept(Tokens, uiCurToken,  "ifndef", &uiAccepted))
  {
    if (m_IfdefActiveStack.PeekBack() != IfDefActivity::IsActive)
    {
      m_IfdefActiveStack.PushBack(IfDefActivity::IsInactive);
      return EZ_SUCCESS;
    }

    ezUInt32 uiIdentifier = uiCurToken;
    if (Expect(Tokens, uiCurToken, ezTokenType::Identifier, &uiIdentifier).Failed())
      return EZ_FAILURE;

    const ezString sIdentifier = Tokens[uiIdentifier]->m_DataView;

    const bool bDefined = m_Macros.Find(sIdentifier).IsValid();
    const bool bIsIfdef = Tokens[uiAccepted]->m_DataView == "ifdef";

    m_IfdefActiveStack.PushBack(bIsIfdef == bDefined ? IfDefActivity::IsActive : IfDefActivity::IsInactive);

    return EZ_SUCCESS;
  }

  if (Accept(Tokens, uiCurToken, "else"))
  {
    if (m_IfdefActiveStack.IsEmpty())
    {
      PP_LOG0(Error, "Wrong usage of #else", Tokens[uiCurToken - 1]);
      return EZ_FAILURE;
    }

    const IfDefActivity bCur = (IfDefActivity) m_IfdefActiveStack.PeekBack();
    m_IfdefActiveStack.PopBack();

    if (m_IfdefActiveStack.PeekBack() != IfDefActivity::IsActive)
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

  if (Accept(Tokens, uiCurToken, "if"))
  {
    if (m_IfdefActiveStack.PeekBack() != IfDefActivity::IsActive)
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

  if (Accept(Tokens, uiCurToken, "elif"))
  {
    const IfDefActivity Cur = (IfDefActivity) m_IfdefActiveStack.PeekBack();
    m_IfdefActiveStack.PopBack();

    if (m_IfdefActiveStack.PeekBack() != IfDefActivity::IsActive)
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

  if (Accept(Tokens, uiCurToken, "endif"))
  {
    SkipWhitespace(Tokens, uiCurToken);

    ExpectEndOfLine(Tokens, uiCurToken);

    m_IfdefActiveStack.PopBack();

    if (m_IfdefActiveStack.IsEmpty())
    {
      PP_LOG0(Error, "Unexpected '#endif'", Tokens[0]);
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  // we are currently inside an inactive text block, so skip all the following commands
  if (m_IfdefActiveStack.PeekBack() != IfDefActivity::IsActive)
    return EZ_SUCCESS;


  if (Accept(Tokens, uiCurToken, "define"))
    return AddDefine(Tokens, uiCurToken);

  if (Accept(Tokens, uiCurToken, "undef"))
  {
    if (Expect(Tokens, uiCurToken, ezTokenType::Identifier).Failed())
      return EZ_FAILURE;

    const ezString sUndef = Tokens[uiCurToken - 1]->m_DataView;
    RemoveDefine(sUndef.GetData());

    // this is an error, but not one that will cause it to fail
    ExpectEndOfLine(Tokens, uiCurToken);

    return EZ_SUCCESS;
  }

  if (Accept(Tokens, uiCurToken, "error", &uiAccepted) || Accept(Tokens, uiCurToken, "warning", &uiAccepted))
  {
    SkipWhitespace(Tokens, uiCurToken);

    ezStringBuilder sTemp;
    CombineTokensToString(Tokens, uiCurToken, sTemp);

    if (Tokens[uiAccepted]->m_DataView == "error")
    {
      PP_LOG(Error, "#error '%s'", Tokens[uiAccepted], sTemp.GetData());
      return EZ_FAILURE;
    }
    else
      PP_LOG(Warning, "#warning '%s'", Tokens[uiAccepted], sTemp.GetData());

    return EZ_SUCCESS;
  }

  // Pass #line and #pragma commands through unmodified, the user expects them to arrive in the final output properly
  if (Accept(Tokens, uiCurToken, "line") || Accept(Tokens, uiCurToken, "pragma"))
  {
    ezStringBuilder sTemp;
    CombineTokensToString(Tokens, 0, sTemp);

    // TODO: Append at token output instead
    sOutput.Append(sTemp.GetData());
    return EZ_SUCCESS;
  }

  PP_LOG0(Error, "Expected a preprocessor command", Tokens[0]);
  return EZ_FAILURE;
}


