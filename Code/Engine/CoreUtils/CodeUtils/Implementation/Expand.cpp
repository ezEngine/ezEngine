#include <CoreUtils/PCH.h>
#include <CoreUtils/CodeUtils/Preprocessor.h>

ezResult ezPreprocessor::Expand(const TokenStream& Tokens, TokenStream& Output)
{
  TokenStream Temp[2];
  ezInt32 iCur0 = 0;
  ezInt32 iCur1 = 1;

  // first expansion
  if (ExpandOnce(Tokens, Temp[iCur0]).Failed())
    return EZ_FAILURE;

  // second expansion
  if (ExpandOnce(Temp[iCur0], Temp[iCur1]).Failed())
    return EZ_FAILURE;

  ezInt32 iIterations = 2;

  // if they are not equal, at least two expansions are necessary
  while (Temp[iCur0] != Temp[iCur1])
  {
    // TODO: Generally 2 iterations should be sufficient to handle most (all?) cases
    // the limit is currently very strict to detect whether there are macros that could require more expansions
    // if we can construct a macro that needs more iterations, this limit can easily be raised
    if (iIterations >= 3)
    {
      PP_LOG(Warning, "Macro expansion reached %i iterations", Tokens[0], iIterations);

      if (iIterations > 10)
      {
        PP_LOG(Error, "Macro expansion reached %i iterations", Tokens[0], iIterations);
        return EZ_FAILURE;
      }
    }

    iIterations++;

    ezMath::Swap(iCur0, iCur1);

    // third and up
    Temp[iCur1].Clear();
    if (ExpandOnce(Temp[iCur0], Temp[iCur1]).Failed())
      return EZ_FAILURE;
  }

  Output.PushBackRange(Temp[iCur1]);

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ExpandOnce(const TokenStream& Tokens, TokenStream& Output)
{
  for (ezUInt32 uiCurToken = 0; uiCurToken < Tokens.GetCount(); )
  {
    EZ_ASSERT(Tokens[uiCurToken]->m_iType < s_MacroParameter0, "Implementation error");

    // if it is no identifier, it cannot be a macro -> just pass it through
    if (Tokens[uiCurToken]->m_iType != ezTokenType::Identifier)
    {
      Output.PushBack(Tokens[uiCurToken]);
      ++uiCurToken;
      continue;
    }

    const ezUInt32 uiIdentifierToken = uiCurToken;
    const ezString sIdentifier = Tokens[uiIdentifierToken]->m_DataView;

    auto itMacro = m_Macros.Find(sIdentifier);

    // no known macro name, or flagged as not to be expanded further -> pass through
    if (!itMacro.IsValid() || ((Tokens[uiCurToken]->m_uiCustomFlags & TokenFlags::NoFurtherExpansion) != 0))
    {
      Output.PushBack(Tokens[uiCurToken]);
      ++uiCurToken;
      continue;
    }

    // it is a valid macro name !

    if (!itMacro.Value().m_bIsFunction)
    {
      ExpandObjectMacro(itMacro.Value(), Output);
      ++uiCurToken; // move uiCurToken after the object macro token
      continue;
    }

    ++uiCurToken;

    SkipWhitespaceAndNewline(Tokens, uiCurToken);

    if (Accept(Tokens, uiCurToken, "("))
    {
      --uiCurToken;

      // we have a function macro -> extract all parameters, preexpand them, then replace the macro body parameters and expand the macro itself

      MacroParameters AllParameters;
      if (ExtractAllMacroParameters(Tokens, uiCurToken, AllParameters).Failed())
        return EZ_FAILURE;

      // uiCurToken is now after the )

      if (ExpandFunctionMacro(itMacro.Value(), AllParameters, Output).Failed())
        return EZ_FAILURE;

      continue;
    }
    else
    {
      // although the identifier is a function macro name, it is not used as a function macro -> just pass it through

      for (ezUInt32 i = uiIdentifierToken; i < uiCurToken; ++i)
        Output.PushBack(Tokens[i]);

      continue;
    }

     EZ_REPORT_FAILURE("The loop body's end should never be reached.");
  }

  return EZ_SUCCESS;
}

void ezPreprocessor::OutputNotExpandableMacro(MacroDefinition& Macro, TokenStream& Output)
{
  const ezString sMacroName = Macro.m_MacroIdentifier->m_DataView;

  EZ_ASSERT(Macro.m_bCurrentlyExpanding, "Implementation Error.");

  ezToken* pNewToken = AddCustomToken(Macro.m_MacroIdentifier, sMacroName.GetData());
  pNewToken->m_uiCustomFlags = TokenFlags::NoFurtherExpansion;

  Output.PushBack(pNewToken);
}

ezResult ezPreprocessor::ExpandObjectMacro(MacroDefinition& Macro, TokenStream& Output)
{
  // when the macro is already being expanded, just pass the macro name through, but flag it as not to be expanded further
  if (Macro.m_bCurrentlyExpanding)
  {
    OutputNotExpandableMacro(Macro, Output);
    return EZ_SUCCESS;
  }

  Macro.m_bCurrentlyExpanding = true;

  if (Expand(Macro.m_Replacement, Output).Failed())
    return EZ_FAILURE;

  Macro.m_bCurrentlyExpanding = false;

  return EZ_SUCCESS;
}

void ezPreprocessor::PassThroughFunctionMacro(MacroDefinition& Macro, const MacroParameters& Parameters, TokenStream& Output)
{
  OutputNotExpandableMacro(Macro, Output);

  Output.PushBack(m_TokenOpenParenthesis);

  for (ezUInt32 p = 0; p < Parameters.GetCount(); ++p)
  {
    // TODO: Maybe the passed through parameters need expansion

    Output.PushBackRange(Parameters[p]);

    if (p + 1 < Parameters.GetCount())
      Output.PushBack(m_TokenComma);
  }

  Output.PushBack(m_TokenClosedParenthesis);
}

ezToken* ezPreprocessor::CreateStringifiedParameter(ezUInt32 uiParam, const ezToken* pParamToken, const MacroDefinition& Macro)
{
  ezStringBuilder sStringifiedParam;

  // if there were fewer parameters passed than the macro uses, replace the non-existing parameter by an empty string
  if (uiParam >= m_MacroParamStack.PeekBack()->GetCount())
    sStringifiedParam = "\"\"";
  else
  {
    // if we want to stringify the varargs parameters
    if (Macro.m_bHasVarArgs && uiParam + 1 == Macro.m_iNumParameters)
    {
      ezStringBuilder sOneParam;

      sStringifiedParam = "\"";

      // stringifh each parameter at the end, attach it to the output
      for (ezUInt32 i = uiParam; i < m_MacroParamStack.PeekBack()->GetCount(); ++i)
      {
        StringifyTokens((*m_MacroParamStack.PeekBack())[i], sOneParam, false);

        if (i > uiParam) // second, third, etc.
          sStringifiedParam.Append(", ");

        sStringifiedParam.Append(sOneParam.GetData());
      }

      // remove redundant whitespace at end (can happen when having empty parameters
      while (sStringifiedParam.EndsWith(" "))
        sStringifiedParam.Shrink(0, 1);

      sStringifiedParam.Append("\"");
    }
    else
      StringifyTokens((*m_MacroParamStack.PeekBack())[uiParam], sStringifiedParam, true);
  }

  ezToken* pStringifiedToken = AddCustomToken(pParamToken, sStringifiedParam.GetData());
  pStringifiedToken->m_iType = ezTokenType::String1;
  return pStringifiedToken;
}

ezResult ezPreprocessor::InsertParameters(const TokenStream& Tokens, TokenStream& Output, const MacroDefinition& Macro)
{
  // todo: handle stringification and concatenation here

  ezInt32 iConsecutiveHashes = 0;
  bool bLastTokenWasHash = false;
  bool bStringifyParameter = false;

  for (ezUInt32 i = 0; i < Tokens.GetCount(); ++i)
  {
    const bool bTokenIsHash = (Tokens[i]->m_iType == ezTokenType::NonIdentifier && ezString(Tokens[i]->m_DataView) == "#");

    if (bTokenIsHash)
    {
      if (!bLastTokenWasHash)
        iConsecutiveHashes = 0;

      iConsecutiveHashes++;

      bLastTokenWasHash = true;
    }
    else
    {
      bLastTokenWasHash = false;

      // if there is an odd number of hashes, the last hash means 'stringify' the parameter
      bStringifyParameter = ezMath::IsOdd(iConsecutiveHashes);
    }

    if (bStringifyParameter &&
        Tokens[i]->m_iType < s_MacroParameter0 &&
        Tokens[i]->m_iType != ezTokenType::Whitespace)
    {
      PP_LOG0(Error, "Expected a macro parameter name", Tokens[i]);
      return EZ_FAILURE;
    }

    if (Tokens[i]->m_iType >= s_MacroParameter0)
    {
      const ezUInt32 uiParam = Tokens[i]->m_iType - s_MacroParameter0;

      if (bStringifyParameter)
      {
        bStringifyParameter = false;

        ezToken* pStringifiedToken = CreateStringifiedParameter(uiParam, Tokens[i], Macro);

        // remove all whitespaces and the last # at the end of the output
        while (true)
        {
          if (Output.PeekBack()->m_iType == ezTokenType::Whitespace)
            Output.PopBack();
          else
          {
            Output.PopBack();
            break;
          }
        }
        
        Output.PushBack(pStringifiedToken);
      }
      else
      {
        if (ExpandMacroParam(*Tokens[i], uiParam, Output, Macro).Failed())
          return EZ_FAILURE;
      }
    }
    else
    {
      // we are also appending # signs as we go, they will be removed later again, if necessary
      Output.PushBack(Tokens[i]);
    }

    if (!bTokenIsHash &&
        (Tokens[i]->m_iType != ezTokenType::BlockComment) &&
        (Tokens[i]->m_iType != ezTokenType::LineComment) &&
        (Tokens[i]->m_iType != ezTokenType::Whitespace))
        iConsecutiveHashes = 0;
  }

  // hash at  the end of a macro is already forbidden as 'invalid character at end of macro'
  // so this case does not need to be handled here

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ExpandFunctionMacro(MacroDefinition& Macro, const MacroParameters& Parameters, TokenStream& Output)
{
  // when the macro is already being expanded, just pass the macro name through, but flag it as not to be expanded further
  if (Macro.m_bCurrentlyExpanding)
  {
    // for the function macro, also output the parameter list
    PassThroughFunctionMacro(Macro, Parameters, Output);
    return EZ_SUCCESS;
  }

  MacroParameters ExpandedParameters;
  ExpandedParameters.SetCount(Parameters.GetCount());

  for (ezUInt32 i = 0; i < Parameters.GetCount(); ++i)
  {
    if (Expand(Parameters[i], ExpandedParameters[i]).Failed())
      return EZ_FAILURE;
  }

  m_MacroParamStackExpanded.PushBack(&ExpandedParameters);
  m_MacroParamStack.PushBack(&Parameters);

  Macro.m_bCurrentlyExpanding = true;

  TokenStream MacroOutput;
  if (InsertParameters(Macro.m_Replacement, MacroOutput, Macro).Failed())
    return EZ_FAILURE;

  if (Expand(MacroOutput, Output).Failed())
    return EZ_FAILURE;

  Macro.m_bCurrentlyExpanding = false;

  m_MacroParamStack.PopBack();
  m_MacroParamStackExpanded.PopBack();

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ExpandMacroParam(const ezToken& MacroToken, ezUInt32 uiParam, TokenStream& Output, const MacroDefinition& Macro)
{
  EZ_ASSERT(!m_MacroParamStack.IsEmpty(), "Implementation error.");

  const MacroParameters& ParamsExpanded = *m_MacroParamStackExpanded.PeekBack();

  if (uiParam >= ParamsExpanded.GetCount())
  {
    ezToken* pWhitespace = AddCustomToken(&MacroToken, " ");
    pWhitespace->m_iType = ezTokenType::Whitespace;

    Output.PushBack(pWhitespace);

    PP_LOG(Warning, "Trying to access parameter %u, but only %u parameters were passed along", (&MacroToken), uiParam, ParamsExpanded.GetCount());
    return EZ_SUCCESS;
  }
  else if (uiParam + 1 == Macro.m_iNumParameters && Macro.m_bHasVarArgs)
  {
    // insert all vararg parameters here

    for (ezUInt32 i = uiParam; i < ParamsExpanded.GetCount(); ++i)
    {
      Output.PushBackRange(ParamsExpanded[i]);

      if (i + 1 < ParamsExpanded.GetCount())
        Output.PushBack(m_TokenComma);
    }

    return EZ_SUCCESS;
  }

  Output.PushBackRange(ParamsExpanded[uiParam]);

  return EZ_SUCCESS;
}

