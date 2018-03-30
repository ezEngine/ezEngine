#include <PCH.h>
#include <Foundation/CodeUtils/Preprocessor.h>

using namespace ezTokenParseUtils;

ezResult ezPreprocessor::Expect(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken, ezUInt32* pAccepted)
{
  if (Tokens.GetCount() < 1)
  {
    ezLog::Error(m_pLog, "Expected token '{0}', got empty token stream", szToken);
    return EZ_FAILURE;
  }

  if (Accept(Tokens, uiCurToken, szToken, pAccepted))
    return EZ_SUCCESS;

  const ezUInt32 uiErrorToken = ezMath::Min(Tokens.GetCount() - 1, uiCurToken);
  ezString sErrorToken = Tokens[uiErrorToken]->m_DataView;
  PP_LOG(Error, "Expected token '{0}' got '{1}'", Tokens[uiErrorToken], szToken, sErrorToken);

  return EZ_FAILURE;
}

ezResult ezPreprocessor::Expect(const TokenStream& Tokens, ezUInt32& uiCurToken, ezTokenType::Enum Type, ezUInt32* pAccepted)
{
  if (Tokens.GetCount() < 1)
  {
    ezLog::Error(m_pLog, "Expected token of type '{0}', got empty token stream", ezTokenType::EnumNames[Type]);
    return EZ_FAILURE;
  }

  if (Accept(Tokens, uiCurToken, Type, pAccepted))
    return EZ_SUCCESS;

  const ezUInt32 uiErrorToken = ezMath::Min(Tokens.GetCount() - 1, uiCurToken);
  PP_LOG(Error, "Expected token of type '{0}' got type '{1}' instead", Tokens[uiErrorToken], ezTokenType::EnumNames[Type], ezTokenType::EnumNames[Tokens[uiErrorToken]->m_iType]);

  return EZ_FAILURE;
}

ezResult ezPreprocessor::Expect(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken1, const char* szToken2, ezUInt32* pAccepted)
{
  if (Tokens.GetCount() < 2)
  {
    ezLog::Error(m_pLog, "Expected tokens '{0}{1}', got empty token stream", szToken1, szToken2);
    return EZ_FAILURE;
  }

  if (Accept(Tokens, uiCurToken, szToken1, szToken2, pAccepted))
    return EZ_SUCCESS;

  const ezUInt32 uiErrorToken = ezMath::Min(Tokens.GetCount() - 2, uiCurToken);
  ezString sErrorToken1 = Tokens[uiErrorToken]->m_DataView;
  ezString sErrorToken2 = Tokens[uiErrorToken + 1]->m_DataView;
  PP_LOG(Error, "Expected tokens '{0}{1}', got '{2}{3}'", Tokens[uiErrorToken], szToken1, szToken2, sErrorToken1, sErrorToken2);

  return EZ_FAILURE;
}

ezResult ezPreprocessor::ExpectEndOfLine(const TokenStream& Tokens, ezUInt32& uiCurToken)
{
  if (!IsEndOfLine(Tokens, uiCurToken, true))
  {
    ezString sToken = Tokens[uiCurToken]->m_DataView;
    PP_LOG(Warning, "Expected end-of-line, found token '{0}'", Tokens[uiCurToken], sToken);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_PreprocessorParseHelper);

