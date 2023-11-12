#pragma once

#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Containers/HybridArray.h>

namespace ezTokenParseUtils
{
  using TokenStream = ezHybridArray<const ezToken*, 32>;

  EZ_FOUNDATION_DLL void SkipWhitespace(const TokenStream& tokens, ezUInt32& ref_uiCurToken);
  EZ_FOUNDATION_DLL void SkipWhitespaceAndNewline(const TokenStream& tokens, ezUInt32& ref_uiCurToken);
  EZ_FOUNDATION_DLL bool IsEndOfLine(const TokenStream& tokens, ezUInt32 uiCurToken, bool bIgnoreWhitespace);
  EZ_FOUNDATION_DLL void CopyRelevantTokens(const TokenStream& source, ezUInt32 uiFirstSourceToken, TokenStream& ref_destination, bool bPreserveNewLines);

  EZ_FOUNDATION_DLL bool Accept(const TokenStream& tokens, ezUInt32& ref_uiCurToken, ezStringView sToken, ezUInt32* pAccepted = nullptr);
  EZ_FOUNDATION_DLL bool Accept(const TokenStream& tokens, ezUInt32& ref_uiCurToken, ezTokenType::Enum type, ezUInt32* pAccepted = nullptr);
  EZ_FOUNDATION_DLL bool Accept(const TokenStream& tokens, ezUInt32& ref_uiCurToken, ezStringView sToken1, ezStringView sToken2, ezUInt32* pAccepted = nullptr);
  EZ_FOUNDATION_DLL bool AcceptUnless(const TokenStream& tokens, ezUInt32& ref_uiCurToken, ezStringView sToken1, ezStringView sToken2, ezUInt32* pAccepted = nullptr);

  EZ_FOUNDATION_DLL void CombineTokensToString(const TokenStream& tokens, ezUInt32 uiCurToken, ezStringBuilder& ref_sResult, bool bKeepComments = true, bool bRemoveRedundantWhitespace = false, bool bInsertLine = false);
  EZ_FOUNDATION_DLL void CombineRelevantTokensToString(const TokenStream& tokens, ezUInt32 uiCurToken, ezStringBuilder& ref_sResult);
  EZ_FOUNDATION_DLL void CreateCleanTokenStream(const TokenStream& tokens, ezUInt32 uiCurToken, TokenStream& ref_destination, bool bKeepComments);
} // namespace ezTokenParseUtils
