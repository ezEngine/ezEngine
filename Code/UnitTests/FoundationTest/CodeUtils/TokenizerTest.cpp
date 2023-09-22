#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/Tokenizer.h>

namespace
{
  void CompareResults(const ezDynamicArray<ezStringView> expected, ezTokenizer& tokenizer)
  {
    auto& tokens = tokenizer.GetTokens();

    EZ_TEST_INT_MSG(expected.GetCount(), tokens.GetCount(), "Number of tokens does not match number of expected tokens");

    ezUInt32 count = ezMath::Min(expected.GetCount(), tokens.GetCount());

    for (ezUInt32 i = 0; i < count; ++i)
    {
      if (!EZ_TEST_BOOL_MSG(expected[i] == tokens[i].m_DataView, "Token with index %u does not match, expected '%.*s' actual '%.*s'", i, expected[i].GetElementCount(), expected[i], tokens[i].m_DataView.GetElementCount(), tokens[i].m_DataView))
      {
        break;
      }
    }
  }
} // namespace

EZ_CREATE_SIMPLE_TEST(CodeUtils, Tokenizer)
{
  /// \test Add ezTokenizer tests
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Raw string literal")
  {
    const char* stringLiteral = R"token(
const char* test = R"(
eins
zwei)";
)token";

    ezTokenizer tokenizer(ezFoundation::GetDefaultAllocator());
    tokenizer.Tokenize(ezMakeArrayPtr(reinterpret_cast<const ezUInt8*>(stringLiteral), ezStringUtils::GetStringElementCount(stringLiteral)), ezLog::GetThreadLocalLogSystem());

    ezDynamicArray<ezStringView> expectedResult;
    expectedResult.PushBack("const");
    expectedResult.PushBack("char");
    expectedResult.PushBack("*");
    expectedResult.PushBack("=");

    CompareResults(expectedResult, tokenizer);
  }
}
