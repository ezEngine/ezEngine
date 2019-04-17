#pragma once

#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Containers/HybridArray.h>

namespace ezTokenParseUtils
{
  typedef ezHybridArray<const ezToken*, 32> TokenStream;

  EZ_FOUNDATION_DLL void SkipWhitespace(const TokenStream& Tokens, ezUInt32& uiCurToken);
  EZ_FOUNDATION_DLL void SkipWhitespaceAndNewline(const TokenStream& Tokens, ezUInt32& uiCurToken);
  EZ_FOUNDATION_DLL bool IsEndOfLine(const TokenStream& Tokens, ezUInt32 uiCurToken, bool bIgnoreWhitespace);
  EZ_FOUNDATION_DLL void CopyRelevantTokens(const TokenStream& Source, ezUInt32 uiFirstSourceToken, TokenStream& Destination, bool bPreserveNewLines);

  EZ_FOUNDATION_DLL bool Accept(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken, ezUInt32* pAccepted = nullptr);
  EZ_FOUNDATION_DLL bool Accept(const TokenStream& Tokens, ezUInt32& uiCurToken, ezTokenType::Enum Type, ezUInt32* pAccepted = nullptr);
  EZ_FOUNDATION_DLL bool Accept(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken1, const char* szToken2, ezUInt32* pAccepted = nullptr);
  EZ_FOUNDATION_DLL bool AcceptUnless(const TokenStream& Tokens, ezUInt32& uiCurToken, const char* szToken1, const char* szToken2, ezUInt32* pAccepted = nullptr);

  EZ_FOUNDATION_DLL void CombineTokensToString(const TokenStream& Tokens, ezUInt32 uiCurToken, ezStringBuilder& sResult, bool bKeepComments = true, bool bRemoveRedundantWhitespace = false, bool bInsertLine = false);
  EZ_FOUNDATION_DLL void CombineRelevantTokensToString(const TokenStream& Tokens, ezUInt32 uiCurToken, ezStringBuilder& sResult);
  EZ_FOUNDATION_DLL void CreateCleanTokenStream(const TokenStream& Tokens, ezUInt32 uiCurToken, TokenStream& Destination, bool bKeepComments);
}

