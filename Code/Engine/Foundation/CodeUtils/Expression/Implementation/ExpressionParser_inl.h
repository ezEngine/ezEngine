#include "ExpressionParser.h"

inline bool ezExpressionParser::AcceptStatementTerminator()
{
  return ezTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, ezTokenType::Newline) ||
         ezTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, ";");
}

inline ezResult ezExpressionParser::Expect(const char* szToken, const ezToken** pExpectedToken)
{
  ezUInt32 uiAcceptedToken = 0;
  if (ezTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, szToken, &uiAcceptedToken) == false)
  {
    const ezUInt32 uiErrorToken = ezMath::Min(m_TokenStream.GetCount() - 1, m_uiCurrentToken);
    auto pToken = m_TokenStream[uiErrorToken];
    ReportError(pToken, ezFmt("Syntax error, expected {} but got {}", szToken, pToken->m_DataView));
    return EZ_FAILURE;
  }

  if (pExpectedToken != nullptr)
  {
    *pExpectedToken = m_TokenStream[uiAcceptedToken];
  }

  return EZ_SUCCESS;
}

inline ezResult ezExpressionParser::Expect(ezTokenType::Enum Type, const ezToken** pExpectedToken /*= nullptr*/)
{
  ezUInt32 uiAcceptedToken = 0;
  if (ezTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, Type, &uiAcceptedToken) == false)
  {
    const ezUInt32 uiErrorToken = ezMath::Min(m_TokenStream.GetCount() - 1, m_uiCurrentToken);
    auto pToken = m_TokenStream[uiErrorToken];
    ReportError(pToken, ezFmt("Syntax error, expected token type {} but got {}", ezTokenType::EnumNames[Type], ezTokenType::EnumNames[pToken->m_iType]));
    return EZ_FAILURE;
  }

  if (pExpectedToken != nullptr)
  {
    *pExpectedToken = m_TokenStream[uiAcceptedToken];
  }

  return EZ_SUCCESS;
}

inline void ezExpressionParser::ReportError(const ezToken* pToken, const ezFormatString& message0)
{
  ezStringBuilder tmp;
  ezStringView message = message0.GetText(tmp);
  ezLog::Error("{}({},{}): {}", pToken->m_File, pToken->m_uiLine, pToken->m_uiColumn, message);
}
