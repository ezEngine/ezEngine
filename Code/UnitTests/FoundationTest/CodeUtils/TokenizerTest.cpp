#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>

namespace
{
  using TokenMatch = ezTokenParseUtils::TokenMatch;

  void CompareResults(const ezDynamicArray<TokenMatch>& expected, ezTokenizer& inout_tokenizer, bool bIgnoreWhitespace)
  {
    auto& tokens = inout_tokenizer.GetTokens();

    const ezUInt32 expectedCount = expected.GetCount();
    const ezUInt32 tokenCount = tokens.GetCount();

    ezUInt32 expectedIndex = 0, tokenIndex = 0;
    while (expectedIndex < expectedCount && tokenIndex < tokenCount)
    {
      auto& token = tokens[tokenIndex];
      if (bIgnoreWhitespace && (token.m_iType == ezTokenType::Whitespace || token.m_iType == ezTokenType::Newline))
      {
        tokenIndex++;
        continue;
      }

      auto& e = expected[expectedIndex];

      if (!EZ_TEST_BOOL_MSG(e.m_Type == token.m_iType, "Token with index %u does not match in type, expected %d actual %d", expectedIndex, e.m_Type, token.m_iType))
      {
        return;
      }

      if (!EZ_TEST_BOOL_MSG(e.m_sToken == token.m_DataView, "Token with index %u does not match, expected '%.*s' actual '%.*s'", expectedIndex, e.m_sToken.GetElementCount(), e.m_sToken.GetStartPointer(), token.m_DataView.GetElementCount(), token.m_DataView.GetStartPointer()))
      {
        return;
      }
      tokenIndex++;
      expectedIndex++;
    }

    // Skip remaining whitespace and newlines
    if (bIgnoreWhitespace)
    {
      while (tokenIndex < tokenCount)
      {
        auto& token = tokens[tokenIndex];
        if (token.m_iType != ezTokenType::Whitespace && token.m_iType != ezTokenType::Newline)
        {
          break;
        }
        tokenIndex++;
      }
    }

    if (EZ_TEST_BOOL_MSG(tokenIndex == tokenCount - 1, "Not all tokens have been consumed"))
    {
      EZ_TEST_BOOL_MSG(tokens[tokenIndex].m_iType == ezTokenType::EndOfFile, "Last token must be end of file token");
    }

    EZ_TEST_BOOL_MSG(expectedIndex == expectedCount, "Not all expected values have been consumed");
  }
} // namespace

EZ_CREATE_SIMPLE_TEST(CodeUtils, Tokenizer)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Token Types")
  {
    const char* stringLiteral = R"(
float f=10.3f + 100'000.0;
int i=100'000*12345;
// line comment
/*
block comment
*/
char c='f';
const char* bla =  "blup";
)";
    ezTokenizer tokenizer(ezFoundation::GetDefaultAllocator());
    tokenizer.Tokenize(ezMakeArrayPtr(reinterpret_cast<const ezUInt8*>(stringLiteral), ezStringUtils::GetStringElementCount(stringLiteral)), ezLog::GetThreadLocalLogSystem(), false);

    EZ_TEST_BOOL(tokenizer.GetTokenizedData().IsEmpty());

    ezDynamicArray<TokenMatch> expectedResult;
    expectedResult.PushBack({ezTokenType::Newline, "\n"});

    expectedResult.PushBack({ezTokenType::Identifier, "float"});
    expectedResult.PushBack({ezTokenType::Whitespace, " "});
    expectedResult.PushBack({ezTokenType::Identifier, "f"});
    expectedResult.PushBack({ezTokenType::NonIdentifier, "="});
    expectedResult.PushBack({ezTokenType::Float, "10.3f"});
    expectedResult.PushBack({ezTokenType::Whitespace, " "});
    expectedResult.PushBack({ezTokenType::NonIdentifier, "+"});
    expectedResult.PushBack({ezTokenType::Whitespace, " "});
    expectedResult.PushBack({ezTokenType::Float, "100'000.0"});
    expectedResult.PushBack({ezTokenType::NonIdentifier, ";"});
    expectedResult.PushBack({ezTokenType::Newline, "\n"});

    expectedResult.PushBack({ezTokenType::Identifier, "int"});
    expectedResult.PushBack({ezTokenType::Whitespace, " "});
    expectedResult.PushBack({ezTokenType::Identifier, "i"});
    expectedResult.PushBack({ezTokenType::NonIdentifier, "="});
    expectedResult.PushBack({ezTokenType::Integer, "100'000"});
    expectedResult.PushBack({ezTokenType::NonIdentifier, "*"});
    expectedResult.PushBack({ezTokenType::Integer, "12345"});
    expectedResult.PushBack({ezTokenType::NonIdentifier, ";"});
    expectedResult.PushBack({ezTokenType::Newline, "\n"});

    expectedResult.PushBack({ezTokenType::LineComment, "// line comment"});
    expectedResult.PushBack({ezTokenType::Newline, "\n"});

    expectedResult.PushBack({ezTokenType::BlockComment, "/*\nblock comment\n*/"});
    expectedResult.PushBack({ezTokenType::Newline, "\n"});

    expectedResult.PushBack({ezTokenType::Identifier, "char"});
    expectedResult.PushBack({ezTokenType::Whitespace, " "});
    expectedResult.PushBack({ezTokenType::Identifier, "c"});
    expectedResult.PushBack({ezTokenType::NonIdentifier, "="});
    expectedResult.PushBack({ezTokenType::String2, "'f'"});
    expectedResult.PushBack({ezTokenType::NonIdentifier, ";"});
    expectedResult.PushBack({ezTokenType::Newline, "\n"});

    expectedResult.PushBack({ezTokenType::Identifier, "const"});
    expectedResult.PushBack({ezTokenType::Whitespace, " "});
    expectedResult.PushBack({ezTokenType::Identifier, "char"});
    expectedResult.PushBack({ezTokenType::NonIdentifier, "*"});
    expectedResult.PushBack({ezTokenType::Whitespace, " "});
    expectedResult.PushBack({ezTokenType::Identifier, "bla"});
    expectedResult.PushBack({ezTokenType::Whitespace, " "});
    expectedResult.PushBack({ezTokenType::NonIdentifier, "="});
    expectedResult.PushBack({ezTokenType::Whitespace, "  "});
    expectedResult.PushBack({ezTokenType::String1, "\"blup\""});
    expectedResult.PushBack({ezTokenType::NonIdentifier, ";"});
    expectedResult.PushBack({ezTokenType::Newline, "\n"});

    CompareResults(expectedResult, tokenizer, false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Raw string literal")
  {
    const char* stringLiteral = R"token(
const char* test = R"(
eins
zwei)";
const char* test2 = R"foo(
vier,
fuenf
)foo";
)token";

    ezTokenizer tokenizer(ezFoundation::GetDefaultAllocator());
    tokenizer.Tokenize(ezMakeArrayPtr(reinterpret_cast<const ezUInt8*>(stringLiteral), ezStringUtils::GetStringElementCount(stringLiteral)), ezLog::GetThreadLocalLogSystem());

    ezDynamicArray<TokenMatch> expectedResult;
    expectedResult.PushBack({ezTokenType::Identifier, "const"});
    expectedResult.PushBack({ezTokenType::Identifier, "char"});
    expectedResult.PushBack({ezTokenType::NonIdentifier, "*"});
    expectedResult.PushBack({ezTokenType::Identifier, "test"});
    expectedResult.PushBack({ezTokenType::NonIdentifier, "="});
    expectedResult.PushBack({ezTokenType::RawString1Prefix, "R\"("});
    expectedResult.PushBack({ezTokenType::RawString1, "\neins\nzwei"});
    expectedResult.PushBack({ezTokenType::RawString1Postfix, ")\""});
    expectedResult.PushBack({ezTokenType::NonIdentifier, ";"});
    expectedResult.PushBack({ezTokenType::Identifier, "const"});
    expectedResult.PushBack({ezTokenType::Identifier, "char"});
    expectedResult.PushBack({ezTokenType::NonIdentifier, "*"});
    expectedResult.PushBack({ezTokenType::Identifier, "test2"});
    expectedResult.PushBack({ezTokenType::NonIdentifier, "="});
    expectedResult.PushBack({ezTokenType::RawString1Prefix, "R\"foo("});
    expectedResult.PushBack({ezTokenType::RawString1, "\nvier,\nfuenf\n"});
    expectedResult.PushBack({ezTokenType::RawString1Postfix, ")foo\""});
    expectedResult.PushBack({ezTokenType::NonIdentifier, ";"});

    CompareResults(expectedResult, tokenizer, true);
  }
}
