#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/CodeUtils/Tokenizer.h>

namespace ezTokenParseUtils
{
  typedef ezHybridArray<const ezToken*, 32> TokenStream;

  void SkipWhitespace(const TokenStream& Tokens, ezUInt32& uiCurToken);
  void SkipWhitespaceAndNewline(const TokenStream& Tokens, ezUInt32& uiCurToken);
  bool IsEndOfLine(const TokenStream& Tokens, ezUInt32 uiCurToken, bool bIgnoreWhitespace);
  void CopyRelevantTokens(const TokenStream& Source, ezUInt32 uiFirstSourceToken, TokenStream& Destination, bool bPreserveNewLines);

  bool Accept(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken, ezUInt32* pAccepted = nullptr);
  bool Accept(const TokenStream& Tokens, ezUInt32& uiCurToken, ezTokenType::Enum Type, ezUInt32* pAccepted = nullptr);
  bool Accept(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken1, const char* szToken2, ezUInt32* pAccepted = nullptr);
  bool AcceptUnless(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken1, const char* szToken2, ezUInt32* pAccepted = nullptr);

  void CombineTokensToString(const TokenStream& Tokens, ezUInt32 uiCurToken, ezStringBuilder& sResult, bool bKeepComments = true, bool bRemoveRedundantWhitespace = false, bool bInsertLine = false);
  void CombineRelevantTokensToString(const TokenStream& Tokens, ezUInt32 uiCurToken, ezStringBuilder& sResult);
  void CreateCleanTokenStream(const TokenStream& Tokens, ezUInt32 uiCurToken, TokenStream& Destination, bool bKeepComments);
}

