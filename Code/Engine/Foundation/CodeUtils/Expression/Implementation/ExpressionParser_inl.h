
inline ezResult ezExpressionParser::Expect(const char* szToken)
{
  if (ezTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, szToken) == false)
  {
    auto pCurToken = m_TokenStream[m_uiCurrentToken];
    ReportError(pCurToken, ezFmt("Syntax error, expected {} but got {}", szToken, pCurToken->m_DataView));
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}


inline void ezExpressionParser::ReportError(const ezToken* pToken, const ezFormatString& message0)
{
  ezStringBuilder tmp;
  ezStringView message = message0.GetText(tmp);
  ezLog::Error("{}({},{}): {}", pToken->m_File, pToken->m_uiLine, pToken->m_uiColumn, message);
}
