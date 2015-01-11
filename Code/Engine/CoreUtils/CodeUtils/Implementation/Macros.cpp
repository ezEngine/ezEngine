#include <CoreUtils/PCH.h>
#include <CoreUtils/CodeUtils/Preprocessor.h>

ezPreprocessor::MacroDefinition::MacroDefinition()
{
  m_MacroIdentifier = nullptr;
  m_bIsFunction = false;
  m_bCurrentlyExpanding = false;
  m_iNumParameters = -1;
  m_bHasVarArgs = false;
}

void ezPreprocessor::CopyTokensReplaceParams(const TokenStream& Source, ezUInt32 uiFirstSourceToken, TokenStream& Destination, const ezHybridArray<ezString, 16>& parameters)
{
  Destination.Clear();
  Destination.Reserve(Source.GetCount() - uiFirstSourceToken);

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
          Source[i]->m_iType == ezTokenType::Newline)
          continue;

      if (Source[i]->m_iType == ezTokenType::Identifier)
      {
        for (ezUInt32 p = 0; p < parameters.GetCount(); ++p)
        {
          if (Source[i]->m_DataView == parameters[p])
          {
            // create a custom token for the parameter, for better error messages
            ezToken* pParamToken = AddCustomToken(Source[i], parameters[p].GetData());
            pParamToken->m_iType = s_MacroParameter0 + p;

            Destination.PushBack(pParamToken);
            goto tokenfound;
          }
        }
      }

      Destination.PushBack(Source[i]);

    tokenfound:
      continue;
    }
  }

  // remove whitespace at end of macro
  while (!Destination.IsEmpty() && Destination.PeekBack()->m_iType == ezTokenType::Whitespace)
    Destination.PopBack();
}

ezResult ezPreprocessor::ExtractParameterName(const TokenStream& Tokens, ezUInt32& uiCurToken, ezString& sIdentifierName)
{
  SkipWhitespace(Tokens, uiCurToken);

  if (uiCurToken + 2 < Tokens.GetCount() && 
      Tokens[uiCurToken + 0]->m_DataView == "." &&
      Tokens[uiCurToken + 1]->m_DataView == "." &&
      Tokens[uiCurToken + 2]->m_DataView == ".")
  {
    sIdentifierName = "...";
    uiCurToken += 3;
  }
  else
  {
    ezUInt32 uiParamToken = uiCurToken;

    if (Expect(Tokens, uiCurToken, ezTokenType::Identifier, &uiParamToken).Failed())
      return EZ_FAILURE;

    sIdentifierName = Tokens[uiParamToken]->m_DataView;
  }

  // skip a trailing comma
  if (Accept(Tokens, uiCurToken, ","))
    SkipWhitespace(Tokens, uiCurToken);

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ExtractAllMacroParameters(const TokenStream& Tokens, ezUInt32& uiCurToken, ezDeque< TokenStream >& AllParameters)
{
  if (Expect(Tokens, uiCurToken, "(").Failed())
    return EZ_FAILURE;

  do
  {
    // add one parameter
    // note: we always add one extra parameter value, because MACRO() is actually a macro call with one empty parameter
    // the same for MACRO(a,) is a macro with two parameters, the second being empty
    AllParameters.SetCount(AllParameters.GetCount() + 1);

    if (ExtractParameterValue(Tokens, uiCurToken, AllParameters.PeekBack()).Failed())
      return EZ_FAILURE;

    // reached the end of the parameter list
    if (Accept(Tokens, uiCurToken, ")"))
      return EZ_SUCCESS;
  }
  while (Accept(Tokens, uiCurToken, ",")); // continue with the next parameter

  ezString s = Tokens[uiCurToken]->m_DataView;
  PP_LOG(Error, "',' or ')' expected, got '%s' instead", Tokens[uiCurToken], s.GetData());

  return EZ_FAILURE;
}

ezResult ezPreprocessor::ExtractParameterValue(const TokenStream& Tokens, ezUInt32& uiCurToken, TokenStream& ParamTokens)
{
  SkipWhitespaceAndNewline(Tokens, uiCurToken);
  const ezUInt32 uiFirstToken = ezMath::Min(uiCurToken, Tokens.GetCount() - 1);

  ezInt32 iParenthesis = 0;

  // get all tokens up until a comma or the last closing parenthesis
  // ignore commas etc. as long as they are surrounded with parenthesis
  for ( ; uiCurToken < Tokens.GetCount(); ++uiCurToken)
  {
    if (Tokens[uiCurToken]->m_iType == ezTokenType::BlockComment ||
        Tokens[uiCurToken]->m_iType == ezTokenType::LineComment ||
        Tokens[uiCurToken]->m_iType == ezTokenType::Newline)
        continue;

    if (Tokens[uiCurToken]->m_iType == ezTokenType::EndOfFile)
      break; // outputs an error

    if (iParenthesis == 0)
    {
      if (Tokens[uiCurToken]->m_DataView == "," || Tokens[uiCurToken]->m_DataView == ")")
      {
        if (!ParamTokens.IsEmpty() && ParamTokens.PeekBack()->m_iType == ezTokenType::Whitespace)
        {
          ParamTokens.PopBack();
        }
        return EZ_SUCCESS;
      }
    }

    if (Tokens[uiCurToken]->m_DataView == "(")
      ++iParenthesis;
    else
    if (Tokens[uiCurToken]->m_DataView == ")")
      --iParenthesis;

    ParamTokens.PushBack(Tokens[uiCurToken]);
  }
  
  // reached the end of the stream without encountering the closing parenthesis first
  PP_LOG0(Error, "Unexpected end of file during macro parameter extraction", Tokens[uiFirstToken]);
  return EZ_FAILURE;
}

void ezPreprocessor::StringifyTokens(const TokenStream& Tokens, ezStringBuilder& sResult, bool bSurroundWithQuotes)
{
  ezUInt32 uiCurToken = 0;

  sResult.Clear();

  if (bSurroundWithQuotes)
    sResult = "\"";

  ezStringBuilder sTemp;

  SkipWhitespace(Tokens, uiCurToken);

  ezUInt32 uiLastNonWhitespace = Tokens.GetCount();

  while (uiLastNonWhitespace > 0)
  {
    if (Tokens[uiLastNonWhitespace - 1]->m_iType != ezTokenType::Whitespace &&
        Tokens[uiLastNonWhitespace - 1]->m_iType != ezTokenType::Newline &&
        Tokens[uiLastNonWhitespace - 1]->m_iType != ezTokenType::BlockComment &&
        Tokens[uiLastNonWhitespace - 1]->m_iType != ezTokenType::LineComment)
        break;

    --uiLastNonWhitespace;
  }

  for (ezUInt32 t = uiCurToken; t < uiLastNonWhitespace; ++t)
  {
    // comments, newlines etc. are stripped out
    if ((Tokens[t]->m_iType == ezTokenType::LineComment) ||
        (Tokens[t]->m_iType == ezTokenType::BlockComment) ||
        (Tokens[t]->m_iType == ezTokenType::Newline) ||
        (Tokens[t]->m_iType == ezTokenType::EndOfFile))
        continue;

    sTemp = Tokens[t]->m_DataView;

    // all whitespace becomes a single white space
    if (Tokens[t]->m_iType == ezTokenType::Whitespace)
      sTemp = " ";

    // inside strings, all backslashes and double quotes are escaped
    if ((Tokens[t]->m_iType == ezTokenType::String1) ||
        (Tokens[t]->m_iType == ezTokenType::String2))
    {
      sTemp.ReplaceAll("\\", "\\\\");
      sTemp.ReplaceAll("\"", "\\\"");
    }

    sResult.Append(sTemp.GetData());
  }

  if (bSurroundWithQuotes)
    sResult.Append("\"");
}



EZ_STATICLINK_FILE(CoreUtils, CoreUtils_CodeUtils_Implementation_Macros);

