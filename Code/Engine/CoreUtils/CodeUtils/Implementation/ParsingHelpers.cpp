#include <CoreUtils/PCH.h>
#include <CoreUtils/CodeUtils/Preprocessor.h>

void ezPreprocessor::SkipWhitespace(const TokenStream& Tokens, ezUInt32& uiCurToken)
{
  while (uiCurToken < Tokens.GetCount() &&
         ((Tokens[uiCurToken]->m_iType == ezTokenType::Whitespace) ||
          (Tokens[uiCurToken]->m_iType == ezTokenType::BlockComment) ||
          (Tokens[uiCurToken]->m_iType == ezTokenType::LineComment)))
    ++uiCurToken;
}

void ezPreprocessor::SkipWhitespaceAndNewline(const TokenStream& Tokens, ezUInt32& uiCurToken)
{
  while (uiCurToken < Tokens.GetCount() &&
         ((Tokens[uiCurToken]->m_iType == ezTokenType::Whitespace) ||
          (Tokens[uiCurToken]->m_iType == ezTokenType::BlockComment) ||
          (Tokens[uiCurToken]->m_iType == ezTokenType::Newline) ||
          (Tokens[uiCurToken]->m_iType == ezTokenType::LineComment)))
    ++uiCurToken;
}

bool ezPreprocessor::IsEndOfLine(const TokenStream& Tokens, ezUInt32 uiCurToken, bool bIgnoreWhitespace)
{
  if (bIgnoreWhitespace)
    SkipWhitespace(Tokens, uiCurToken);

  if (uiCurToken >= Tokens.GetCount())
    return true;

  return Tokens[uiCurToken]->m_iType == ezTokenType::Newline || Tokens[uiCurToken]->m_iType == ezTokenType::EndOfFile;
}

void ezPreprocessor::CopyRelevantTokens(const TokenStream& Source, ezUInt32 uiFirstSourceToken, TokenStream& Destination, bool bPreserveNewLines)
{
  Destination.Reserve(Destination.GetCount() + Source.GetCount() - uiFirstSourceToken);

  {
    // skip all whitespace at the start of the replacement string
    ezUInt32 i = uiFirstSourceToken;
    SkipWhitespace(Source, i);

    // add all the relevant tokens to the definition
    for ( ; i < Source.GetCount(); ++i)
    {
      if (Source[i]->m_iType == ezTokenType::BlockComment ||
          Source[i]->m_iType == ezTokenType::LineComment ||
          Source[i]->m_iType == ezTokenType::EndOfFile ||
          (!bPreserveNewLines && Source[i]->m_iType == ezTokenType::Newline))
          continue;

      Destination.PushBack(Source[i]);
    }
  }

  // remove whitespace at end of macro
  while (!Destination.IsEmpty() && Destination.PeekBack()->m_iType == ezTokenType::Whitespace)
    Destination.PopBack();
}

bool ezPreprocessor::Accept(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken, ezUInt32* pAccepted)
{
  SkipWhitespace(Tokens, uiCurToken);

  if (uiCurToken >= Tokens.GetCount())
    return false;

  if (Tokens[uiCurToken]->m_DataView == szToken)
  {
    if (pAccepted)
      *pAccepted = uiCurToken;

    uiCurToken++;
    return true;
  }

  return false;
}

bool ezPreprocessor::Accept(const TokenStream& Tokens, ezUInt32& uiCurToken, ezTokenType::Enum Type, ezUInt32* pAccepted)
{
  SkipWhitespace(Tokens, uiCurToken);

  if (uiCurToken >= Tokens.GetCount())
    return false;

  if (Tokens[uiCurToken]->m_iType == Type)
  {
    if (pAccepted)
      *pAccepted = uiCurToken;

    uiCurToken++;
    return true;
  }

  return false;
}

bool ezPreprocessor::Accept(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken1, const char* szToken2, ezUInt32* pAccepted)
{
  SkipWhitespace(Tokens, uiCurToken);

  if (uiCurToken + 1 >= Tokens.GetCount())
    return false;

  if (Tokens[uiCurToken]->m_DataView == szToken1 && Tokens[uiCurToken + 1]->m_DataView == szToken2)
  {
    if (pAccepted)
      *pAccepted = uiCurToken;

    uiCurToken += 2;
    return true;
  }

  return false;
}

bool ezPreprocessor::AcceptUnless(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken1, const char* szToken2, ezUInt32* pAccepted)
{
  SkipWhitespace(Tokens, uiCurToken);

  if (uiCurToken + 1 >= Tokens.GetCount())
    return false;

  if (Tokens[uiCurToken]->m_DataView == szToken1 && Tokens[uiCurToken + 1]->m_DataView != szToken2)
  {
    if (pAccepted)
      *pAccepted = uiCurToken;

    uiCurToken += 1;
    return true;
  }

  return false;
}


ezResult ezPreprocessor::Expect(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken, ezUInt32* pAccepted)
{
  if (Tokens.GetCount() < 1)
  {
    ezLog::Error(m_pLog, "Expected token '%s', got empty token stream", szToken);
    return EZ_FAILURE;
  }

  if (Accept(Tokens, uiCurToken, szToken, pAccepted))
    return EZ_SUCCESS;

  const ezUInt32 uiErrorToken = ezMath::Min(Tokens.GetCount() - 1, uiCurToken);
  ezString sErrorToken = Tokens[uiErrorToken]->m_DataView;
  PP_LOG(Error, "Expected token '%s' got '%s'", Tokens[uiErrorToken], szToken, sErrorToken.GetData());

  return EZ_FAILURE;
}

ezResult ezPreprocessor::Expect(const TokenStream& Tokens, ezUInt32& uiCurToken, ezTokenType::Enum Type, ezUInt32* pAccepted)
{
  if (Tokens.GetCount() < 1)
  {
    ezLog::Error(m_pLog, "Expected token of type '%s', got empty token stream", ezTokenType::EnumNames[Type]);
    return EZ_FAILURE;
  }

  if (Accept(Tokens, uiCurToken, Type, pAccepted))
    return EZ_SUCCESS;

  const ezUInt32 uiErrorToken = ezMath::Min(Tokens.GetCount() - 1, uiCurToken);
  PP_LOG(Error, "Expected token of type '%s' got type '%s' instead", Tokens[uiErrorToken], ezTokenType::EnumNames[Type], ezTokenType::EnumNames[Tokens[uiErrorToken]->m_iType]);

  return EZ_FAILURE;
}

ezResult ezPreprocessor::Expect(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken1, const char* szToken2, ezUInt32* pAccepted)
{
  if (Tokens.GetCount() < 2)
  {
    ezLog::Error(m_pLog, "Expected tokens '%s%s', got empty token stream", szToken1, szToken2);
    return EZ_FAILURE;
  }

  if (Accept(Tokens, uiCurToken, szToken1, szToken2, pAccepted))
    return EZ_SUCCESS;

  const ezUInt32 uiErrorToken = ezMath::Min(Tokens.GetCount() - 2, uiCurToken);
  ezString sErrorToken1 = Tokens[uiErrorToken]->m_DataView;
  ezString sErrorToken2 = Tokens[uiErrorToken + 1]->m_DataView;
  PP_LOG(Error, "Expected tokens '%s%s', got '%s%s'", Tokens[uiErrorToken], szToken1, szToken2, sErrorToken1.GetData(), sErrorToken2.GetData());

  return EZ_FAILURE;
}

ezResult ezPreprocessor::ExpectEndOfLine(const TokenStream& Tokens, ezUInt32& uiCurToken)
{
  if (!IsEndOfLine(Tokens, uiCurToken, true))
  {
    ezString sToken = Tokens[uiCurToken]->m_DataView;
    PP_LOG(Warning, "Expected end-of-line, found token '%s'", Tokens[uiCurToken], sToken.GetData());
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezPreprocessor::CombineRelevantTokensToString(const TokenStream& Tokens, ezUInt32 uiCurToken, ezStringBuilder& sResult)
{
  sResult.Clear();
  ezStringBuilder sTemp;

  for (ezUInt32 t = uiCurToken; t < Tokens.GetCount(); ++t)
  {
    if ((Tokens[t]->m_iType == ezTokenType::LineComment) ||
        (Tokens[t]->m_iType == ezTokenType::BlockComment) ||
        (Tokens[t]->m_iType == ezTokenType::Newline) ||
        (Tokens[t]->m_iType == ezTokenType::EndOfFile))
        continue;

    sTemp = Tokens[t]->m_DataView;
    sResult.Append(sTemp.GetData());
  }
}


void ezPreprocessor::CreateCleanTokenStream(const TokenStream& Tokens, ezUInt32 uiCurToken, TokenStream& Destination, bool bKeepComments)
{
  SkipWhitespace(Tokens, uiCurToken);

  for (ezUInt32 t = uiCurToken; t < Tokens.GetCount(); ++t)
  {
    if (Tokens[t]->m_iType == ezTokenType::Newline)
    {
      // remove all whitespace before a newline
      while (!Destination.IsEmpty() && Destination.PeekBack()->m_iType == ezTokenType::Whitespace)
        Destination.PopBack();

      // if there is already a newline stored, discard the new one
      if (!Destination.IsEmpty() && Destination.PeekBack()->m_iType == ezTokenType::Newline)
        continue;
    }

    Destination.PushBack(Tokens[t]);
  }
}

void ezPreprocessor::CombineTokensToString(const TokenStream& Tokens0, ezUInt32 uiCurToken, ezStringBuilder& sResult, bool bKeepComments, bool bRemoveRedundantWhitespace, bool bInsertLine)
{
  TokenStream Tokens;

  if (bRemoveRedundantWhitespace)
  {
    CreateCleanTokenStream(Tokens0, uiCurToken, Tokens, bKeepComments);
    uiCurToken = 0;
  }
  else
    Tokens = Tokens0;

  sResult.Clear();
  ezStringBuilder sTemp;

  ezUInt32 uiCurLine = 0xFFFFFFFF;
  ezHashedString sCurFile;

  for (ezUInt32 t = uiCurToken; t < Tokens.GetCount(); ++t)
  {
    // skip all comments, if not desired
    if ((Tokens[t]->m_iType == ezTokenType::BlockComment ||
      Tokens[t]->m_iType == ezTokenType::LineComment) &&
      !bKeepComments)
      continue;

    if (Tokens[t]->m_iType == ezTokenType::EndOfFile)
      return;

    if (bInsertLine)
    {
      if (sResult.IsEmpty())
      {
        sResult.AppendFormat("#line %u \"%s\"\n", Tokens[t]->m_uiLine, Tokens[t]->m_File.GetData());
        uiCurLine = Tokens[t]->m_uiLine;
        sCurFile = Tokens[t]->m_File;
      }

      if (Tokens[t]->m_iType == ezTokenType::Newline)
      {
        ++uiCurLine;
      }

      if (t > 0 && Tokens[t - 1]->m_iType == ezTokenType::Newline)
      {
        if (Tokens[t]->m_uiLine != uiCurLine ||
            Tokens[t]->m_File != sCurFile)
        {
          sResult.AppendFormat("\n#line %u \"%s\"\n", Tokens[t]->m_uiLine, Tokens[t]->m_File.GetData());
          uiCurLine = Tokens[t]->m_uiLine;
          sCurFile = Tokens[t]->m_File;
        }
      }
    }

    sTemp = Tokens[t]->m_DataView;
    sResult.Append(sTemp.GetData());
  }
}



EZ_STATICLINK_FILE(CoreUtils, CoreUtils_CodeUtils_Implementation_ParsingHelpers);

