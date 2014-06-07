#include <CoreUtils/PCH.h>
#include <CoreUtils/CodeUtils/Preprocessor.h>

void ezPreprocessor::ClearDefines()
{
  m_Macros.Clear();
}

bool ezPreprocessor::RemoveDefine(const char* szName)
{
  auto it = m_Macros.Find(szName);

  if (it.IsValid())
  {
    m_Macros.Erase(it);
    return true;
  }

  return false;
}


ezResult ezPreprocessor::StoreDefine(const ezToken* pMacroNameToken, const TokenStream* pReplacementTokens, ezUInt32 uiFirstReplacementToken, ezInt32 iNumParameters, bool bUsesVarArgs)
{
  const ezString sMacroName = pMacroNameToken->m_DataView;

  if ((sMacroName == "defined") || (sMacroName == "__FILE__") || (sMacroName == "__LINE__"))
  {
    PP_LOG(Error, "Macro name '%s' is reserved, '#define' ignored", pMacroNameToken, sMacroName.GetData());
    return EZ_FAILURE;
  }

  MacroDefinition md;
  md.m_MacroIdentifier = pMacroNameToken;
  md.m_bIsFunction = iNumParameters >= 0;
  md.m_iNumParameters = ezMath::Max(0, iNumParameters);
  md.m_bHasVarArgs = bUsesVarArgs;

  // removes whitespace at start and end, skips comments, newlines, etc.
  if (pReplacementTokens)
    CopyRelevantTokens(*pReplacementTokens, uiFirstReplacementToken, md.m_Replacement);

  if (!md.m_Replacement.IsEmpty() && md.m_Replacement.PeekBack()->m_DataView == "#")
  {
    PP_LOG(Error, "Macro '%s' ends with invalid character '#'", md.m_Replacement.PeekBack(), sMacroName.GetData());
    return EZ_FAILURE;
  }

  bool bExisted = false;
  auto it = m_Macros.FindOrAdd(sMacroName, &bExisted);

  if (bExisted)
  {
    PP_LOG(Error, "Redefinition of macro '%s'", pMacroNameToken, sMacroName.GetData());
    return EZ_FAILURE;
  }

  it.Value() = md;
  return EZ_SUCCESS;
}

ezResult ezPreprocessor::AddDefine(const TokenStream& Tokens, ezUInt32& uiCurToken)
{
  SkipWhitespace(Tokens, uiCurToken);

  ezUInt32 uiNameToken = uiCurToken;

  if (Expect(Tokens, uiCurToken, ezTokenType::Identifier, &uiNameToken).Failed())
    return EZ_FAILURE;

  // check if we got an empty macro definition
  if (IsEndOfLine(Tokens, uiCurToken, true))
  {
    ezStringBuilder sDefine = Tokens[uiNameToken]->m_DataView;

    PP_LOG(Dev, "Empty Macro definition: '%s'", Tokens[uiNameToken], sDefine.GetData());
    StoreDefine(Tokens[uiNameToken], nullptr, 0, -1, false);
    return EZ_SUCCESS;
  }

  // first determine whether this is a function macro (before skipping whitespace)
  if (Tokens[uiCurToken]->m_DataView != "(")
  {
    // add the rest of the macro definition as the replacement
    return StoreDefine(Tokens[uiNameToken], &Tokens, uiCurToken, -1, false);
  }

  // extract parameter names
  {
    bool bVarArgsFounds = false;

    // skip the opening parenthesis (
    if (Expect(Tokens, uiCurToken, "(").Failed())
      return EZ_FAILURE;

    ezHybridArray<ezString, 16> parameters;

    while (!Accept(Tokens, uiCurToken, ")"))
    {
      if (uiCurToken >= Tokens.GetCount())
      {
        PP_LOG(Error, "Could not extract macro parameter %i, reached end of token stream first", Tokens[Tokens.GetCount() - 1], parameters.GetCount());
        return EZ_FAILURE;
      }

      const ezUInt32 uiCurParamToken = uiCurToken;

      ezString sParam;
      if (ExtractParameterName(Tokens, uiCurToken, sParam) == EZ_FAILURE)
      {
        PP_LOG(Error, "Could not extract macro parameter %i", Tokens[uiCurParamToken], parameters.GetCount());
        return EZ_FAILURE;
      }

      if (bVarArgsFounds)
      {
        PP_LOG(Error, "No additional parameters are allowed after '...'", Tokens[uiCurParamToken]);
        return EZ_FAILURE;
      }

      /// \todo Make sure the same parameter name is not used twice

      if (sParam == "...")
      {
        bVarArgsFounds = true;
        sParam = "__VA_ARGS__";

        PP_LOG(Dev, "Macro parameter %u is varargs", Tokens[uiCurParamToken], parameters.GetCount());
      }

      parameters.PushBack(sParam);
    }

    TokenStream ReplacementTokens;
    CopyTokensReplaceParams(Tokens, uiCurToken, ReplacementTokens, parameters);

    StoreDefine(Tokens[uiNameToken], &ReplacementTokens, 0, parameters.GetCount(), bVarArgsFounds);
  }

  return EZ_SUCCESS;
}

