#include <CoreUtils/PCH.h>
#include <CoreUtils/CodeUtils/Preprocessor.h>
#include <Foundation/Utilities/ConversionUtils.h>

ezResult ezPreprocessor::CopyTokensAndEvaluateDefined(const TokenStream& Source, ezUInt32 uiFirstSourceToken, TokenStream& Destination)
{
  Destination.Clear();
  Destination.Reserve(Source.GetCount() - uiFirstSourceToken);

  {
    // skip all whitespace at the start of the replacement string
    ezUInt32 uiCurToken = uiFirstSourceToken;
    SkipWhitespace(Source, uiCurToken);

    // add all the relevant tokens to the definition
    while (uiCurToken < Source.GetCount())
    {
      if (Source[uiCurToken]->m_iType == ezTokenType::BlockComment ||
          Source[uiCurToken]->m_iType == ezTokenType::LineComment ||
          Source[uiCurToken]->m_iType == ezTokenType::EndOfFile ||
          Source[uiCurToken]->m_iType == ezTokenType::Newline)
      {
          ++uiCurToken;
          continue;
      }

      if (ezString(Source[uiCurToken]->m_DataView) == "defined")
      {
        ++uiCurToken;

        const bool bParenthesis = Accept(Source, uiCurToken, "(");

        ezUInt32 uiIdentifier = uiCurToken;
        if (Expect(Source, uiCurToken, ezTokenType::Identifier, &uiIdentifier).Failed())
          return EZ_FAILURE;

        ezToken* pReplacement = nullptr;

        const ezString sIdentifier = Source[uiIdentifier]->m_DataView;

        const bool bDefined = m_Macros.Find(sIdentifier).IsValid();

        // broadcast that 'defined' is being evaluated
        {
          ProcessingEvent pe;
          pe.m_pToken = Source[uiIdentifier];
          pe.m_Type = ProcessingEvent::CheckDefined;
          pe.m_szInfo = bDefined ? "defined" : "undefined";
          m_ProcessingEvents.Broadcast(pe);
        }

        pReplacement = AddCustomToken(Source[uiIdentifier], bDefined ? "1" : "0");

        Destination.PushBack(pReplacement);

        if (bParenthesis)
        {
          if (Expect(Source, uiCurToken, ")").Failed())
            return EZ_FAILURE;
        }
      }
      else
      {
        Destination.PushBack(Source[uiCurToken]);
        ++uiCurToken;
      }
    }
  }

  // remove whitespace at end of macro
  while (!Destination.IsEmpty() && Destination.PeekBack()->m_iType == ezTokenType::Whitespace)
    Destination.PopBack();

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::EvaluateCondition(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult)
{
  iResult = 0;

  TokenStream Copied;
  if (CopyTokensAndEvaluateDefined(Tokens, uiCurToken, Copied).Failed())
    return EZ_FAILURE;

  TokenStream Expanded;

  if (Expand(Copied, Expanded).Failed())
    return EZ_FAILURE;

  if (Expanded.IsEmpty())
  {
    PP_LOG0(Error, "After expansion the condition is empty", Tokens[uiCurToken]);
    return EZ_FAILURE;
  }

  ezUInt32 uiCurToken2 = 0;
  if (ParseExpressionOr(Expanded, uiCurToken2, iResult).Failed())
    return EZ_FAILURE;

  return ExpectEndOfLine(Expanded, uiCurToken2);
}

ezResult ezPreprocessor::ParseFactor(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult)
{
  while (Accept(Tokens, uiCurToken, "+"))
  {
  }

  if (Accept(Tokens, uiCurToken, "-"))
  {
    if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
      return EZ_FAILURE;

    iResult = -iResult;
    return EZ_SUCCESS;
  }

  if (Accept(Tokens, uiCurToken, "~"))
  {
    if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
      return EZ_FAILURE;

    iResult = ~iResult;
    return EZ_SUCCESS;
  }

  if (Accept(Tokens, uiCurToken, "!"))
  {
    if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
      return EZ_FAILURE;

    iResult = (iResult != 0) ? 0 : 1;
    return EZ_SUCCESS;
  }

  ezUInt32 uiValueToken = uiCurToken;
  if (Accept(Tokens, uiCurToken, ezTokenType::Identifier, &uiValueToken))
  {
    const ezString sVal = Tokens[uiValueToken]->m_DataView;

    ezInt32 iResult32 = 0;

    if (ezConversionUtils::StringToInt(sVal.GetData(), iResult32).Failed())
    {
      // this is not an error, all unknown identifiers are assumed to be zero

      // broadcast that we encountered this unknown identifier
      ProcessingEvent pe;
      pe.m_pToken = Tokens[uiValueToken];
      pe.m_Type = ProcessingEvent::EvaluateUnknown;
      m_ProcessingEvents.Broadcast(pe);
    }

    iResult = (ezInt64) iResult32;

    return EZ_SUCCESS;
  }
  else if (Accept(Tokens, uiCurToken, "("))
  {
    if (ParseExpressionOr(Tokens, uiCurToken, iResult).Failed())
      return EZ_FAILURE;
    
    return Expect(Tokens, uiCurToken, ")");
  }

  uiCurToken = ezMath::Min(uiCurToken, Tokens.GetCount() - 1);
  PP_LOG0(Error, "Syntax error, expected identifier, number or '('", Tokens[uiCurToken]);

  return EZ_FAILURE;
}

ezResult ezPreprocessor::ParseExpressionPlus(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult)
{
  if (ParseExpressionMul(Tokens, uiCurToken, iResult).Failed())
    return EZ_FAILURE;

  while (true)
  {
    if (Accept(Tokens, uiCurToken, "+"))
    {
      ezInt64 iNextValue = 0;
      if (ParseExpressionMul(Tokens, uiCurToken, iNextValue).Failed())
        return EZ_FAILURE;

      iResult += iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "-"))
    {
      ezInt64 iNextValue = 0;
      if (ParseExpressionMul(Tokens, uiCurToken, iNextValue).Failed())
        return EZ_FAILURE;

      iResult -= iNextValue;
    }
    else
      break;
  }

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ParseExpressionShift(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult)
{
  if (ParseExpressionPlus(Tokens, uiCurToken, iResult).Failed())
    return EZ_FAILURE;

  while (true)
  {
    if (Accept(Tokens, uiCurToken, ">", ">"))
    {
      ezInt64 iNextValue = 0;
      if (ParseExpressionPlus(Tokens, uiCurToken, iNextValue).Failed())
        return EZ_FAILURE;

      iResult >>= iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "<", "<"))
    {
      ezInt64 iNextValue = 0;
      if (ParseExpressionPlus(Tokens, uiCurToken, iNextValue).Failed())
        return EZ_FAILURE;

      iResult <<= iNextValue;
    }
    else
      break;
  }

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ParseExpressionOr(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult)
{
  if (ParseExpressionAnd(Tokens, uiCurToken, iResult).Failed())
    return EZ_FAILURE;

  while (Accept(Tokens, uiCurToken, "|", "|"))
  {
    ezInt64 iNextValue = 0;
    if (ParseExpressionAnd(Tokens, uiCurToken, iNextValue).Failed())
      return EZ_FAILURE;

    iResult = (iResult != 0 || iNextValue != 0) ? 1 : 0;
  }

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ParseExpressionAnd(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult)
{
  if (ParseExpressionBitOr(Tokens, uiCurToken, iResult).Failed())
    return EZ_FAILURE;

  while (Accept(Tokens, uiCurToken, "&", "&"))
  {
    ezInt64 iNextValue = 0;
    if (ParseExpressionBitOr(Tokens, uiCurToken, iNextValue).Failed())
      return EZ_FAILURE;

    iResult = (iResult != 0 && iNextValue != 0) ? 1 : 0;
  }

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ParseExpressionBitOr(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult)
{
  if (ParseExpressionBitXor(Tokens, uiCurToken, iResult).Failed())
    return EZ_FAILURE;

  while (AcceptUnless(Tokens, uiCurToken, "|", "|"))
  {
    ezInt64 iNextValue = 0;
    if (ParseExpressionBitXor(Tokens, uiCurToken, iNextValue).Failed())
      return EZ_FAILURE;

    iResult |= iNextValue;
  }

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ParseExpressionBitAnd(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult)
{
  if (ParseCondition(Tokens, uiCurToken, iResult).Failed())
    return EZ_FAILURE;

  while (AcceptUnless(Tokens, uiCurToken, "&", "&"))
  {
    ezInt64 iNextValue = 0;
    if (ParseCondition(Tokens, uiCurToken, iNextValue).Failed())
      return EZ_FAILURE;

    iResult &= iNextValue;
  }

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ParseExpressionBitXor(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult)
{
  if (ParseExpressionBitAnd(Tokens, uiCurToken, iResult).Failed())
    return EZ_FAILURE;

  while (Accept(Tokens, uiCurToken, "^"))
  {
    ezInt64 iNextValue = 0;
    if (ParseExpressionBitAnd(Tokens, uiCurToken, iNextValue).Failed())
      return EZ_FAILURE;

    iResult ^= iNextValue;
  }

  return EZ_SUCCESS;
}
ezResult ezPreprocessor::ParseExpressionMul(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult)
{
  if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
    return EZ_FAILURE;

  while (true)
  {
    if (Accept(Tokens, uiCurToken, "*"))
    {
      ezInt64 iNextValue = 0;
      if (ParseFactor(Tokens, uiCurToken, iNextValue).Failed())
        return EZ_FAILURE;

      iResult *= iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "/"))
    {
      ezInt64 iNextValue = 0;
      if (ParseFactor(Tokens, uiCurToken, iNextValue).Failed())
        return EZ_FAILURE;

      iResult /= iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "%"))
    {
      ezInt64 iNextValue = 0;
      if (ParseFactor(Tokens, uiCurToken, iNextValue).Failed())
        return EZ_FAILURE;

      iResult %= iNextValue;
    }
    else
      break;
  }

  return EZ_SUCCESS;
}

enum class Comparison
{
  None,
  Equal,
  Unequal,
  LessThan,
  GreaterThan,
  LessThanEqual,
  GreaterThanEqual
};

ezResult ezPreprocessor::ParseCondition(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt64& iResult)
{
  ezInt64 iResult1 = 0;
  if (ParseExpressionShift(Tokens, uiCurToken, iResult1).Failed())
    return EZ_FAILURE;

  Comparison Operator = Comparison::None;

  if (Accept(Tokens, uiCurToken, "=", "="))
    Operator = Comparison::Equal;
  else if (Accept(Tokens, uiCurToken, "!", "="))
    Operator = Comparison::Unequal;
  else if (Accept(Tokens, uiCurToken, ">", "="))
    Operator = Comparison::GreaterThanEqual;
  else if (Accept(Tokens, uiCurToken, "<", "="))
    Operator = Comparison::LessThanEqual;
  else if (AcceptUnless(Tokens, uiCurToken, ">", ">"))
    Operator = Comparison::GreaterThan;
  else if (AcceptUnless(Tokens, uiCurToken, "<", "<"))
    Operator = Comparison::LessThan;
  else
  {
    iResult = iResult1;
    return EZ_SUCCESS;
  }

  ezInt64 iResult2 = 0;
  if (ParseExpressionShift(Tokens, uiCurToken, iResult2).Failed())
    return EZ_FAILURE;

  switch (Operator)
  {
  case Comparison::Equal:
    iResult = (iResult1 == iResult2) ? 1 : 0;
    return EZ_SUCCESS;
  case Comparison::GreaterThan:
    iResult = (iResult1 > iResult2) ? 1 : 0;
    return EZ_SUCCESS;
  case Comparison::GreaterThanEqual:
    iResult = (iResult1 >= iResult2) ? 1 : 0;
    return EZ_SUCCESS;
  case Comparison::LessThan:
    iResult = (iResult1 < iResult2) ? 1 : 0;
    return EZ_SUCCESS;
  case Comparison::LessThanEqual:
    iResult = (iResult1 <= iResult2) ? 1 : 0;
    return EZ_SUCCESS;
  case Comparison::Unequal:
    iResult = (iResult1 != iResult2) ? 1 : 0;
    return EZ_SUCCESS;
  case Comparison::None:
    ezLog::Error(m_pLog, "Unknown operator");
    return EZ_FAILURE;
  }

  return EZ_FAILURE;
}



EZ_STATICLINK_FILE(CoreUtils, CoreUtils_CodeUtils_Implementation_Conditions);

