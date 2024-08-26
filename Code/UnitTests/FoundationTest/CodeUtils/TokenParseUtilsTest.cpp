#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>

EZ_CREATE_SIMPLE_TEST(CodeUtils, TokenParseUtils)
{
  const char* stringLiteral = R"(
// Some comment
/* A block comment
Some block
*/
Identifier
)";

  ezTokenizer tokenizer(ezFoundation::GetDefaultAllocator());
  tokenizer.Tokenize(ezMakeArrayPtr(reinterpret_cast<const ezUInt8*>(stringLiteral), ezStringUtils::GetStringElementCount(stringLiteral)), ezLog::GetThreadLocalLogSystem(), false);

  ezTokenParseUtils::TokenStream tokens;
  tokenizer.GetAllTokens(tokens);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SkipWhitespace / IsEndOfLine")
  {
    ezUInt32 uiCurToken = 0;
    EZ_TEST_BOOL(ezTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    uiCurToken++;
    EZ_TEST_BOOL(!ezTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    EZ_TEST_BOOL(ezTokenParseUtils::IsEndOfLine(tokens, uiCurToken, true));
    ezTokenParseUtils::SkipWhitespace(tokens, uiCurToken);
    EZ_TEST_INT(uiCurToken, 2);
    EZ_TEST_BOOL(ezTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    uiCurToken++;
    EZ_TEST_BOOL(!ezTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    EZ_TEST_BOOL(ezTokenParseUtils::IsEndOfLine(tokens, uiCurToken, true));
    ezTokenParseUtils::SkipWhitespace(tokens, uiCurToken);
    EZ_TEST_INT(uiCurToken, 4);
    EZ_TEST_BOOL(ezTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    uiCurToken++;
    EZ_TEST_BOOL(!ezTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    EZ_TEST_BOOL(!ezTokenParseUtils::IsEndOfLine(tokens, uiCurToken, true));
    EZ_TEST_INT(tokens[uiCurToken]->m_iType, ezTokenType::Identifier);
    uiCurToken++;
    EZ_TEST_BOOL(ezTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    uiCurToken++;
    EZ_TEST_INT(tokens[uiCurToken]->m_iType, ezTokenType::EndOfFile);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SkipWhitespaceAndNewline")
  {
    ezUInt32 uiCurToken = 0;
    EZ_TEST_BOOL(ezTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    ezTokenParseUtils::SkipWhitespaceAndNewline(tokens, uiCurToken);
    EZ_TEST_INT(uiCurToken, 5);
    EZ_TEST_INT(tokens[uiCurToken]->m_iType, ezTokenType::Identifier);
    uiCurToken++;
    EZ_TEST_BOOL(ezTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    ezTokenParseUtils::SkipWhitespaceAndNewline(tokens, uiCurToken);
    EZ_TEST_INT(tokens[uiCurToken]->m_iType, ezTokenType::EndOfFile);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CopyRelevantTokens")
  {
    ezUInt32 uiCurToken = 0;
    ezTokenParseUtils::TokenStream relevantTokens;
    ezTokenParseUtils::CopyRelevantTokens(tokens, uiCurToken, relevantTokens, true);

    EZ_TEST_INT(relevantTokens.GetCount(), 5);
    for (ezUInt32 i = 0; i < relevantTokens.GetCount(); ++i)
    {
      if (i == 3)
      {
        EZ_TEST_INT(relevantTokens[i]->m_iType, ezTokenType::Identifier);
      }
      else
      {
        EZ_TEST_INT(relevantTokens[i]->m_iType, ezTokenType::Newline);
      }
    }

    relevantTokens.Clear();
    ezTokenParseUtils::CopyRelevantTokens(tokens, uiCurToken, relevantTokens, false);
    EZ_TEST_INT(relevantTokens.GetCount(), 1);
    EZ_TEST_INT(relevantTokens[0]->m_iType, ezTokenType::Identifier);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Accept")
  {
    ezUInt32 uiCurToken = 0;
    EZ_TEST_BOOL(ezTokenParseUtils::Accept(tokens, uiCurToken, "\n"_ezsv, nullptr));
    EZ_TEST_INT(uiCurToken, 1);
    EZ_TEST_BOOL(ezTokenParseUtils::Accept(tokens, uiCurToken, ezTokenType::Newline, nullptr));
    EZ_TEST_INT(uiCurToken, 3);
    EZ_TEST_BOOL(ezTokenParseUtils::Accept(tokens, uiCurToken, "\n"_ezsv, nullptr));
    EZ_TEST_INT(uiCurToken, 5);

    ezUInt32 uiIdentifierToken = 0;
    EZ_TEST_BOOL(ezTokenParseUtils::Accept(tokens, uiCurToken, ezTokenType::Identifier, &uiIdentifierToken));
    EZ_TEST_INT(uiIdentifierToken, 5);
    EZ_TEST_INT(uiCurToken, 6);

    EZ_TEST_BOOL(ezTokenParseUtils::Accept(tokens, uiCurToken, ezTokenType::Newline, nullptr));

    EZ_TEST_BOOL(!ezTokenParseUtils::Accept(tokens, uiCurToken, ezTokenType::Newline, nullptr));
    EZ_TEST_BOOL(ezTokenParseUtils::Accept(tokens, uiCurToken, ezTokenType::EndOfFile, nullptr));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Accept2")
  {
    ezUInt32 uiCurToken = 0;
    EZ_TEST_BOOL(ezTokenParseUtils::Accept(tokens, uiCurToken, "\n"_ezsv, "// Some comment"_ezsv, nullptr));
    EZ_TEST_INT(uiCurToken, 2);
    ezUInt32 uiTouple1Token = 0;
    EZ_TEST_BOOL(ezTokenParseUtils::Accept(tokens, uiCurToken, "\n"_ezsv, "/* A block comment\nSome block\n*/"_ezsv, &uiTouple1Token));
    EZ_TEST_INT(uiTouple1Token, 2);
    EZ_TEST_INT(uiCurToken, 4);
    EZ_TEST_BOOL(!ezTokenParseUtils::AcceptUnless(tokens, uiCurToken, "\n"_ezsv, "Identifier"_ezsv, nullptr));
    uiCurToken++;
    ezUInt32 uiIdentifierToken = 0;
    EZ_TEST_BOOL(ezTokenParseUtils::AcceptUnless(tokens, uiCurToken, "Identifier"_ezsv, "ScaryString"_ezsv, &uiIdentifierToken));
    EZ_TEST_INT(uiIdentifierToken, 5);
    EZ_TEST_INT(uiCurToken, 6);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Accept3")
  {
    ezUInt32 uiCurToken = 0;
    ezTokenParseUtils::TokenMatch templatePattern[] = {ezTokenType::Newline, ezTokenType::Newline, "Identifier"_ezsv};
    ezHybridArray<ezUInt32, 8> acceptedTokens;
    EZ_TEST_BOOL(!ezTokenParseUtils::Accept(tokens, uiCurToken, templatePattern, &acceptedTokens));
    uiCurToken++;
    EZ_TEST_BOOL(ezTokenParseUtils::Accept(tokens, uiCurToken, templatePattern, &acceptedTokens));

    EZ_TEST_INT(acceptedTokens.GetCount(), EZ_ARRAY_SIZE(templatePattern));
    EZ_TEST_INT(acceptedTokens[0], 2);
    EZ_TEST_INT(acceptedTokens[1], 4);
    EZ_TEST_INT(acceptedTokens[2], 5);
    EZ_TEST_INT(uiCurToken, 6);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Accept4")
  {
    const char* vectorString = "Vec2(2.2, 1.1)";

    ezTokenizer tokenizer2(ezFoundation::GetDefaultAllocator());
    tokenizer2.Tokenize(ezMakeArrayPtr(reinterpret_cast<const ezUInt8*>(vectorString), ezStringUtils::GetStringElementCount(vectorString)), ezLog::GetThreadLocalLogSystem(), false);

    ezTokenParseUtils::TokenStream tokens2;
    tokenizer2.GetAllTokens(tokens2);

    ezUInt32 uiCurToken = 0;
    ezTokenParseUtils::TokenMatch templatePattern[] = {"Vec2"_ezsv, "("_ezsv, ezTokenType::Float, ","_ezsv, ezTokenType::Float, ")"_ezsv};
    ezHybridArray<ezUInt32, 6> acceptedTokens;
    EZ_TEST_BOOL(ezTokenParseUtils::Accept(tokens2, uiCurToken, templatePattern, &acceptedTokens));
    EZ_TEST_INT(uiCurToken, 7);
    EZ_TEST_INT(acceptedTokens.GetCount(), EZ_ARRAY_SIZE(templatePattern));
    EZ_TEST_INT(acceptedTokens[2], 2);
    EZ_TEST_INT(acceptedTokens[4], 5);
    EZ_TEST_STRING(tokens2[acceptedTokens[2]]->m_DataView, "2.2");
    EZ_TEST_STRING(tokens2[acceptedTokens[4]]->m_DataView, "1.1");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CombineTokensToString")
  {
    ezUInt32 uiCurToken = 0;
    ezStringBuilder sResult;
    ezTokenParseUtils::CombineTokensToString(tokens, uiCurToken, sResult);
    EZ_TEST_STRING(sResult, stringLiteral);

    ezTokenParseUtils::CombineTokensToString(tokens, uiCurToken, sResult, false, true);
    EZ_TEST_STRING(sResult, "\n\n\nIdentifier\n");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CombineRelevantTokensToString")
  {
    ezUInt32 uiCurToken = 0;
    ezStringBuilder sResult;
    ezTokenParseUtils::CombineRelevantTokensToString(tokens, uiCurToken, sResult);
    EZ_TEST_STRING(sResult, "Identifier");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CreateCleanTokenStream")
  {
    const char* stringLiteralWithRedundantStuff = "\n\nID1 \nID2";

    ezTokenizer tokenizer2(ezFoundation::GetDefaultAllocator());
    tokenizer2.Tokenize(ezMakeArrayPtr(reinterpret_cast<const ezUInt8*>(stringLiteralWithRedundantStuff), ezStringUtils::GetStringElementCount(stringLiteralWithRedundantStuff)), ezLog::GetThreadLocalLogSystem(), false);

    ezTokenParseUtils::TokenStream tokens2;
    tokenizer2.GetAllTokens(tokens2);

    ezUInt32 uiCurToken = 0;
    ezTokenParseUtils::TokenStream result;
    ezTokenParseUtils::CreateCleanTokenStream(tokens2, uiCurToken, result);

    EZ_TEST_INT(result.GetCount(), 5);

    ezTokenParseUtils::TokenMatch templatePattern[] = {ezTokenType::Newline, "ID1"_ezsv, ezTokenType::Newline, "ID2"_ezsv, ezTokenType::EndOfFile};
    ezHybridArray<ezUInt32, 8> acceptedTokens;
    EZ_TEST_BOOL(ezTokenParseUtils::Accept(result, uiCurToken, templatePattern, nullptr));
    EZ_TEST_INT(uiCurToken, 5);
  }
}
