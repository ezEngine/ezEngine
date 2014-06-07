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

        if (m_Macros.Find(sIdentifier).IsValid())
          pReplacement = AddCustomToken(Source[uiIdentifier], "1");
        else
          pReplacement = AddCustomToken(Source[uiIdentifier], "0");

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

ezResult ezPreprocessor::EvaluateCondition(const TokenStream& Tokens, ezUInt32& uiCurToken, ezInt32& iResult)
{
  iResult = 0;

  TokenStream Copied;
  if (CopyTokensAndEvaluateDefined(Tokens, uiCurToken, Copied).Failed())
    return EZ_FAILURE;

  TokenStream Expanded;

  if (Expand(Copied, Expanded).Failed())
    return EZ_FAILURE;

  ezUInt32 uiCurToken2 = 0;
  return ParseExpressionOr(Expanded, uiCurToken2, iResult);
}

ezResult ezPreprocessor::ParseFactor(const ezHybridArray<const ezToken*, 32>& Tokens, ezUInt32& uiCurToken, ezInt32& iResult)
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

  //if (Accept(Tokens, uiCurToken, "defined"))
  //{
  //  ezUInt32 uiIdentifier = uiCurToken;
  //  if (Expect(Tokens, uiCurToken, ezTokenType::Identifier, &uiIdentifier).Failed())
  //    return EZ_FAILURE;

  //  iResult = m_Macros.Find(Tokens[uiIdentifier]->m_DataView).IsValid() ? 1 : 0;

  //  return EZ_SUCCESS;
  //}
  //else
  if (Accept(Tokens, uiCurToken, ezTokenType::Identifier))
  {
    const ezString sVal = Tokens[uiCurToken - 1]->m_DataView;

    if (ezConversionUtils::StringToInt(sVal.GetData(), iResult).Failed())
    {
      PP_LOG(Error, "Identifier '%s' could not be converted to an integer value", Tokens[uiCurToken], sVal.GetData());
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }
  else if (Accept(Tokens, uiCurToken, "("))
  {
    if (ParseExpressionOr(Tokens, uiCurToken, iResult).Failed())
      return EZ_FAILURE;
    
    return Expect(Tokens, uiCurToken, ")");
  }

  PP_LOG(Error, "Syntax error, expected identifier, number or '('", Tokens[uiCurToken]);

  return EZ_FAILURE;
}

ezResult ezPreprocessor::ParseExpressionPlus(const ezHybridArray<const ezToken*, 32>& Tokens, ezUInt32& uiCurToken, ezInt32& iResult)
{
  if (ParseExpressionMul(Tokens, uiCurToken, iResult).Failed())
    return EZ_FAILURE;

  while (true)
  {
    if (Accept(Tokens, uiCurToken, "+"))
    {
      ezInt32 iNextValue = 0;
      if (ParseExpressionMul(Tokens, uiCurToken, iNextValue).Failed())
        return EZ_FAILURE;

      iResult += iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "-"))
    {
      ezInt32 iNextValue = 0;
      if (ParseExpressionMul(Tokens, uiCurToken, iNextValue).Failed())
        return EZ_FAILURE;

      iResult -= iNextValue;
    }
    else
      break;
  }

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ParseExpressionShift(const ezHybridArray<const ezToken*, 32>& Tokens, ezUInt32& uiCurToken, ezInt32& iResult)
{
  if (ParseExpressionPlus(Tokens, uiCurToken, iResult).Failed())
    return EZ_FAILURE;

  while (true)
  {
    if (Accept(Tokens, uiCurToken, ">", ">"))
    {
      ezInt32 iNextValue = 0;
      if (ParseExpressionPlus(Tokens, uiCurToken, iNextValue).Failed())
        return EZ_FAILURE;

      iResult >>= iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "<", "<"))
    {
      ezInt32 iNextValue = 0;
      if (ParseExpressionPlus(Tokens, uiCurToken, iNextValue).Failed())
        return EZ_FAILURE;

      iResult <<= iNextValue;
    }
    else
      break;
  }

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ParseExpressionOr(const ezHybridArray<const ezToken*, 32>& Tokens, ezUInt32& uiCurToken, ezInt32& iResult)
{
  if (ParseExpressionAnd(Tokens, uiCurToken, iResult).Failed())
    return EZ_FAILURE;

  while (Accept(Tokens, uiCurToken, "|", "|"))
  {
    ezInt32 iNextValue = 0;
    if (ParseExpressionAnd(Tokens, uiCurToken, iNextValue).Failed())
      return EZ_FAILURE;

    iResult = (iResult != 0 || iNextValue != 0) ? 1 : 0;
  }

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ParseExpressionAnd(const ezHybridArray<const ezToken*, 32>& Tokens, ezUInt32& uiCurToken, ezInt32& iResult)
{
  if (ParseCondition(Tokens, uiCurToken, iResult).Failed())
    return EZ_FAILURE;

  while (Accept(Tokens, uiCurToken, "&", "&"))
  {
    ezInt32 iNextValue = 0;
    if (ParseCondition(Tokens, uiCurToken, iNextValue).Failed())
      return EZ_FAILURE;

    iResult = (iResult != 0 && iNextValue != 0) ? 1 : 0;
  }

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ParseExpressionMul(const ezHybridArray<const ezToken*, 32>& Tokens, ezUInt32& uiCurToken, ezInt32& iResult)
{
  if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
    return EZ_FAILURE;

  while (true)
  {
    if (Accept(Tokens, uiCurToken, "*"))
    {
      ezInt32 iNextValue = 0;
      if (ParseFactor(Tokens, uiCurToken, iNextValue).Failed())
        return EZ_FAILURE;

      iResult *= iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "/"))
    {
      ezInt32 iNextValue = 0;
      if (ParseFactor(Tokens, uiCurToken, iNextValue).Failed())
        return EZ_FAILURE;

      iResult /= iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "%"))
    {
      ezInt32 iNextValue = 0;
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

ezResult ezPreprocessor::ParseCondition(const ezHybridArray<const ezToken*, 32>& Tokens, ezUInt32& uiCurToken, ezInt32& iResult)
{
  ezInt32 iResult1 = 0;
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

  ezInt32 iResult2 = 0;
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
  }

  return EZ_FAILURE;
}

