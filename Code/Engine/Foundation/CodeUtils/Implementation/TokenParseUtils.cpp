#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>

namespace ezTokenParseUtils
{
  void SkipWhitespace(const TokenStream& tokens, ezUInt32& ref_uiCurToken)
  {
    while (ref_uiCurToken < tokens.GetCount() && ((tokens[ref_uiCurToken]->m_iType == ezTokenType::Whitespace) || (tokens[ref_uiCurToken]->m_iType == ezTokenType::BlockComment) || (tokens[ref_uiCurToken]->m_iType == ezTokenType::LineComment)))
      ++ref_uiCurToken;
  }

  void SkipWhitespaceAndNewline(const TokenStream& tokens, ezUInt32& ref_uiCurToken)
  {
    while (ref_uiCurToken < tokens.GetCount() && ((tokens[ref_uiCurToken]->m_iType == ezTokenType::Whitespace) || (tokens[ref_uiCurToken]->m_iType == ezTokenType::BlockComment) || (tokens[ref_uiCurToken]->m_iType == ezTokenType::Newline) || (tokens[ref_uiCurToken]->m_iType == ezTokenType::LineComment)))
      ++ref_uiCurToken;
  }

  bool IsEndOfLine(const TokenStream& tokens, ezUInt32 uiCurToken, bool bIgnoreWhitespace)
  {
    if (bIgnoreWhitespace)
      SkipWhitespace(tokens, uiCurToken);

    if (uiCurToken >= tokens.GetCount())
      return true;

    return tokens[uiCurToken]->m_iType == ezTokenType::Newline || tokens[uiCurToken]->m_iType == ezTokenType::EndOfFile;
  }

  void CopyRelevantTokens(const TokenStream& source, ezUInt32 uiFirstSourceToken, TokenStream& ref_destination, bool bPreserveNewLines)
  {
    ref_destination.Reserve(ref_destination.GetCount() + source.GetCount() - uiFirstSourceToken);

    {
      // skip all whitespace at the start of the replacement string
      ezUInt32 i = uiFirstSourceToken;
      SkipWhitespace(source, i);

      // add all the relevant tokens to the definition
      for (; i < source.GetCount(); ++i)
      {
        if (source[i]->m_iType == ezTokenType::BlockComment || source[i]->m_iType == ezTokenType::LineComment || source[i]->m_iType == ezTokenType::EndOfFile || (!bPreserveNewLines && source[i]->m_iType == ezTokenType::Newline))
          continue;

        ref_destination.PushBack(source[i]);
      }
    }

    // remove whitespace at end of macro
    while (!ref_destination.IsEmpty() && ref_destination.PeekBack()->m_iType == ezTokenType::Whitespace)
      ref_destination.PopBack();
  }

  bool Accept(const TokenStream& tokens, ezUInt32& ref_uiCurToken, ezStringView sToken, ezUInt32* pAccepted)
  {
    SkipWhitespace(tokens, ref_uiCurToken);

    if (ref_uiCurToken >= tokens.GetCount())
      return false;

    if (tokens[ref_uiCurToken]->m_DataView == sToken)
    {
      if (pAccepted)
        *pAccepted = ref_uiCurToken;

      ref_uiCurToken++;
      return true;
    }

    return false;
  }

  bool Accept(const TokenStream& tokens, ezUInt32& ref_uiCurToken, ezTokenType::Enum type, ezUInt32* pAccepted)
  {
    SkipWhitespace(tokens, ref_uiCurToken);

    if (ref_uiCurToken >= tokens.GetCount())
      return false;

    if (tokens[ref_uiCurToken]->m_iType == type)
    {
      if (pAccepted)
        *pAccepted = ref_uiCurToken;

      ref_uiCurToken++;
      return true;
    }

    return false;
  }

  bool Accept(const TokenStream& tokens, ezUInt32& ref_uiCurToken, ezStringView sToken1, ezStringView sToken2, ezUInt32* pAccepted)
  {
    SkipWhitespace(tokens, ref_uiCurToken);

    if (ref_uiCurToken + 1 >= tokens.GetCount())
      return false;

    if (tokens[ref_uiCurToken]->m_DataView == sToken1 && tokens[ref_uiCurToken + 1]->m_DataView == sToken2)
    {
      if (pAccepted)
        *pAccepted = ref_uiCurToken;

      ref_uiCurToken += 2;
      return true;
    }

    return false;
  }

  bool AcceptUnless(const TokenStream& tokens, ezUInt32& ref_uiCurToken, ezStringView sToken1, ezStringView sToken2, ezUInt32* pAccepted)
  {
    SkipWhitespace(tokens, ref_uiCurToken);

    if (ref_uiCurToken + 1 >= tokens.GetCount())
      return false;

    if (tokens[ref_uiCurToken]->m_DataView == sToken1 && tokens[ref_uiCurToken + 1]->m_DataView != sToken2)
    {
      if (pAccepted)
        *pAccepted = ref_uiCurToken;

      ref_uiCurToken += 1;
      return true;
    }

    return false;
  }

  bool Accept(const TokenStream& tokens, ezUInt32& ref_uiCurToken, ezArrayPtr<const TokenMatch> matches, ezDynamicArray<ezUInt32>* pAccepted)
  {
    if (pAccepted)
      pAccepted->Clear();

    ezUInt32 uiCurToken = ref_uiCurToken;
    bool bAccepted = true;
    for (ezUInt32 i = 0; i < matches.GetCount() && bAccepted; ++i)
    {
      ezUInt32 uiAcceptedToken = uiCurToken;
      const TokenMatch& match = matches[i];
      if (match.m_Type == ezTokenType::Unknown)
      {
        bAccepted = Accept(tokens, uiCurToken, match.m_sToken, &uiAcceptedToken);
      }
      else
      {
        bAccepted = Accept(tokens, uiCurToken, match.m_Type, &uiAcceptedToken);
      }

      if (pAccepted && bAccepted)
        pAccepted->PushBack(uiAcceptedToken);
    }

    if (bAccepted)
    {
      ref_uiCurToken = uiCurToken;
    }
    else
    {
      if (pAccepted)
        pAccepted->Clear();
    }
    return bAccepted;
  }

  void CombineRelevantTokensToString(const TokenStream& tokens, ezUInt32 uiCurToken, ezStringBuilder& ref_sResult)
  {
    ref_sResult.Clear();
    ezStringBuilder sTemp;

    for (ezUInt32 t = uiCurToken; t < tokens.GetCount(); ++t)
    {
      if ((tokens[t]->m_iType == ezTokenType::LineComment) || (tokens[t]->m_iType == ezTokenType::BlockComment) || (tokens[t]->m_iType == ezTokenType::Newline) || (tokens[t]->m_iType == ezTokenType::EndOfFile))
        continue;

      sTemp = tokens[t]->m_DataView;
      ref_sResult.Append(sTemp.GetView());
    }
  }

  void CreateCleanTokenStream(const TokenStream& tokens, ezUInt32 uiCurToken, TokenStream& ref_destination)
  {
    SkipWhitespace(tokens, uiCurToken);

    for (ezUInt32 t = uiCurToken; t < tokens.GetCount(); ++t)
    {
      if (tokens[t]->m_iType == ezTokenType::Newline)
      {
        // remove all whitespace before a newline
        while (!ref_destination.IsEmpty() && ref_destination.PeekBack()->m_iType == ezTokenType::Whitespace)
          ref_destination.PopBack();

        // if there is already a newline stored, discard the new one
        if (!ref_destination.IsEmpty() && ref_destination.PeekBack()->m_iType == ezTokenType::Newline)
          continue;
      }

      ref_destination.PushBack(tokens[t]);
    }
  }

  void CombineTokensToString(const TokenStream& tokens0, ezUInt32 uiCurToken, ezStringBuilder& ref_sResult, bool bKeepComments, bool bRemoveRedundantWhitespace, bool bInsertLine)
  {
    TokenStream Tokens;

    if (bRemoveRedundantWhitespace)
    {
      CreateCleanTokenStream(tokens0, uiCurToken, Tokens);
      uiCurToken = 0;
    }
    else
      Tokens = tokens0;

    ref_sResult.Clear();
    ezStringBuilder sTemp;

    ezUInt32 uiCurLine = 0xFFFFFFFF;
    ezHashedString sCurFile;

    for (ezUInt32 t = uiCurToken; t < Tokens.GetCount(); ++t)
    {
      // skip all comments, if not desired
      if ((Tokens[t]->m_iType == ezTokenType::BlockComment || Tokens[t]->m_iType == ezTokenType::LineComment) && !bKeepComments)
        continue;

      if (Tokens[t]->m_iType == ezTokenType::EndOfFile)
        return;

      if (bInsertLine)
      {
        if (ref_sResult.IsEmpty())
        {
          ref_sResult.AppendFormat("#line {0} \"{1}\"\n", Tokens[t]->m_uiLine, Tokens[t]->m_File);
          uiCurLine = Tokens[t]->m_uiLine;
          sCurFile = Tokens[t]->m_File;
        }

        if (t > 0 && Tokens[t - 1]->m_iType == ezTokenType::Newline)
        {
          if (Tokens[t]->m_uiLine != uiCurLine || Tokens[t]->m_File != sCurFile)
          {
            if (!ref_sResult.EndsWith("\n"))
              ref_sResult.Append("\n");

            ref_sResult.AppendFormat("#line {0} \"{1}\"\n", Tokens[t]->m_uiLine, Tokens[t]->m_File);
            uiCurLine = Tokens[t]->m_uiLine;
            sCurFile = Tokens[t]->m_File;
          }
        }

        if (Tokens[t]->m_iType == ezTokenType::Newline)
        {
          ++uiCurLine;
        }
      }

      sTemp = Tokens[t]->m_DataView;
      ref_sResult.Append(sTemp.GetView());
    }
  }
} // namespace ezTokenParseUtils
