#include <CoreUtils/PCH.h>
#include <CoreUtils/CodeUtils/Preprocessor.h>

ezResult ezPreprocessor::Expand(const TokenStream& Tokens, TokenStream& Output)
{
  for (ezUInt32 uiCurToken = 0; uiCurToken < Tokens.GetCount(); )
  {
    if (Tokens[uiCurToken]->m_iType >= s_MacroParameter0)
    {
      if (ExpandMacroParam(*Tokens[uiCurToken], Tokens[uiCurToken]->m_iType - s_MacroParameter0, Output).Failed())
        return EZ_FAILURE;

      ++uiCurToken;
      continue;
    }

    // if it is no identifier, it cannot be a macro -> just pass it through
    if (Tokens[uiCurToken]->m_iType != ezTokenType::Identifier)
    {
      Output.PushBack(Tokens[uiCurToken]);
      ++uiCurToken;
      continue;
    }

    const ezString sIdentifier = Tokens[uiCurToken]->m_DataView;

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
      --uiCurToken;
      // although the identifier is a function macro name, it is not used as a function macro -> just pass it through

      Output.PushBack(Tokens[uiCurToken]);
      ++uiCurToken;
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
  if (Expand(Macro.m_Replacement, MacroOutput).Failed())
    return EZ_FAILURE;

  // TODO: check whether this is necessary
  // after the macro has been expanded, expand the result once again to make sure all concatenated etc. stuff ends up correctly expanded
  if (Expand(MacroOutput, Output).Failed())
    return EZ_FAILURE;

  Macro.m_bCurrentlyExpanding = false;

  m_MacroParamStack.PopBack();
  m_MacroParamStackExpanded.PopBack();

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::ExpandMacroParam(const ezToken& MacroToken, ezUInt32 uiParam, TokenStream& Output)
{
  EZ_ASSERT(!m_MacroParamStack.IsEmpty(), "Implementation error.");

  const MacroParameters& Params = *m_MacroParamStack.PeekBack();
  const MacroParameters& ParamsExpanded = *m_MacroParamStackExpanded.PeekBack();

  if (uiParam >= Params.GetCount())
  {
    ezToken* pWhitespace = AddCustomToken(&MacroToken, " ");
    pWhitespace->m_iType = ezTokenType::Whitespace;

    Output.PushBack(pWhitespace);

    PP_LOG(Warning, "Trying to access parameter %u, but only %u parameters were passed along", (&MacroToken), uiParam, Params.GetCount());
    return EZ_SUCCESS;
  }

  // todo: Do not expand when stringified or concatenated

  if (Expand(ParamsExpanded[uiParam], Output).Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

