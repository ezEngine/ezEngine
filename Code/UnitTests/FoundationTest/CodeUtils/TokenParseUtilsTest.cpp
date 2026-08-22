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
    ezTempHybridArray<ezUInt32, 8> acceptedTokens;
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
    ezTempHybridArray<ezUInt32, 6> acceptedTokens;
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
    ezTempHybridArray<ezUInt32, 8> acceptedTokens;
    EZ_TEST_BOOL(ezTokenParseUtils::Accept(result, uiCurToken, templatePattern, nullptr));
    EZ_TEST_INT(uiCurToken, 5);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RenderTemplate")
  {
    // Renders every placeholder as '<NAME>' or '<NAME:INDEX>' so that both the resolved name and index are visible in the result. Optional placeholders use '(..)' instead of '<..>'.
    auto Resolver = [](ezStringView sPlaceholder, ezVariant index, bool bOptional, ezStringBuilder& ref_sOutput)
    {
      ref_sOutput.Append(bOptional ? "(" : "<", sPlaceholder);
      if (index.IsValid())
      {
        const ezString sIndex = index.ConvertTo<ezString>();
        ref_sOutput.Append(":", sIndex.GetView());
      }
      ref_sOutput.Append(bOptional ? ")" : ">");
    };

    ezStringBuilder sOutput;

    struct TemplateTest
    {
      ezStringView m_sTemplate;
      ezStringView m_sExpected;
    };

    TemplateTest tests[] = {
      // trivial cases
      {""_ezsv, ""_ezsv},
      {"Perlin Noise"_ezsv, "Perlin Noise"_ezsv},
      {"{Name}"_ezsv, "<Name>"_ezsv},

      // examples taken from existing ezTitleAttribute usages
      {"Random: {Seed}"_ezsv, "Random: <Seed>"_ezsv},
      {"Subgraph: {Name}"_ezsv, "Subgraph: <Name>"_ezsv},
      {"{Active} Placement Output: {Name}"_ezsv, "<Active> Placement Output: <Name>"_ezsv},
      {"{Operator}({A}, {B})"_ezsv, "<Operator>(<A>, <B>)"_ezsv},
      {"Remap: [{InputMin}, {InputMax}] -> [{OutputMin}, {OutputMax}]"_ezsv, "Remap: [<InputMin>, <InputMax>] -> [<OutputMin>, <OutputMax>]"_ezsv},
      {"Height: [{MinHeight}, {MaxHeight}]"_ezsv, "Height: [<MinHeight>, <MaxHeight>]"_ezsv},
      {"Coroutine::MoveTo {TargetPos}"_ezsv, "Coroutine::MoveTo <TargetPos>"_ezsv},
      {"= {Expression}"_ezsv, "= <Expression>"_ezsv},
      {"Compare: Number {Comparison} {ReferenceValue}"_ezsv, "Compare: Number <Comparison> <ReferenceValue>"_ezsv},
      {"{Type}::Set {Property} = {Value}"_ezsv, "<Type>::Set <Property> = <Value>"_ezsv},
      {"ForLoop [{FirstIndex}..{LastIndex}]"_ezsv, "ForLoop [<FirstIndex>..<LastIndex>]"_ezsv},
      {"Array::GetElement[{Index}]"_ezsv, "Array::GetElement[<Index>]"_ezsv},
      {"Clamp({X}, {Min}, {Max})"_ezsv, "Clamp(<X>, <Min>, <Max>)"_ezsv},
      {"{Condition} ? {A} : {B}"_ezsv, "<Condition> ? <A> : <B>"_ezsv},
      {"Expression::{Expression}"_ezsv, "Expression::<Expression>"_ezsv},
      {"Variant::ConvertTo {Type}"_ezsv, "Variant::ConvertTo <Type>"_ezsv},

      // indexed placeholders
      {"BlendSpace 1D: {Clips[0]} {Clips[1]} {Clips[2]}"_ezsv, "BlendSpace 1D: <Clips:0> <Clips:1> <Clips:2>"_ezsv},
      {"Bone Weights {RootBones[10]}"_ezsv, "Bone Weights <RootBones:10>"_ezsv},

      // whitespace between the placeholder tokens is tolerated and stripped
      {"{ Name }"_ezsv, "<Name>"_ezsv},
      {"{ Clips [ 2 ] }"_ezsv, "<Clips:2>"_ezsv},

      // the optional '$' prefix is stripped, as used by the visual shader titles
      {"{$Name}"_ezsv, "<Name>"_ezsv},
      {"{$Clips[0]}"_ezsv, "<Clips:0>"_ezsv},
      {"{$in0} + {$in1}"_ezsv, "<in0> + <in1>"_ezsv},
      {"Color: {$prop0}"_ezsv, "Color: <prop0>"_ezsv},
      {"Lerp: {$in0} -> {$in1} ({$in2})"_ezsv, "Lerp: <in0> -> <in1> (<in2>)"_ezsv},
      {"{ $ Name }"_ezsv, "<Name>"_ezsv},

      // the optional '?' prefix marks a placeholder as optional and can be combined with '$'
      {"{?Name}"_ezsv, "(Name)"_ezsv},
      {"{?Clips[2]}"_ezsv, "(Clips:2)"_ezsv},
      {"{?$in0}"_ezsv, "(in0)"_ezsv},
      {"{?$Clips[1]}"_ezsv, "(Clips:1)"_ezsv},
      {"{ ? Name }"_ezsv, "(Name)"_ezsv},
      {"Set Bool: '{BlackboardEntry}' to {?Bool}"_ezsv, "Set Bool: '<BlackboardEntry>' to (Bool)"_ezsv},

      // placeholders don't need to be separated by anything
      {"{A}{B}"_ezsv, "<A><B>"_ezsv},

      // the index goes through the integer conversion, which strips leading zeros
      {"{Clips[007]}"_ezsv, "<Clips:7>"_ezsv},

      // a template may span several lines
      {"{A}\n{B}"_ezsv, "<A>\n<B>"_ezsv},

      // non-ASCII text is passed through unchanged
      {"Gr\u00f6\u00dfe: {Size}"_ezsv, "Gr\u00f6\u00dfe: <Size>"_ezsv},

      // comments count as whitespace inside a placeholder and are swallowed with it
      {"{ /*c*/ Name }"_ezsv, "<Name>"_ezsv},

      // a comment is a single token, so a placeholder inside one is not resolved
      {"/* {Name} */"_ezsv, "/* {Name} */"_ezsv},

      // nothing that doesn't form a complete placeholder is touched
      {"{Name"_ezsv, "{Name"_ezsv},
      {"Name}"_ezsv, "Name}"_ezsv},
      {"{}"_ezsv, "{}"_ezsv},
      {"{123}"_ezsv, "{123}"_ezsv},
      {"{Clips[]}"_ezsv, "{Clips[]}"_ezsv},
      {"{Clips[a]}"_ezsv, "{Clips[a]}"_ezsv},
      {"{Clips[-1]}"_ezsv, "{Clips[-1]}"_ezsv},
      {"{Clips[99999999999999]}"_ezsv, "{Clips[99999999999999]}"_ezsv},
      {"{$}"_ezsv, "{$}"_ezsv},
      {"{$$Name}"_ezsv, "{$$Name}"_ezsv},
      {"{$0}"_ezsv, "{$0}"_ezsv},
      {"{?}"_ezsv, "{?}"_ezsv},
      {"{??Name}"_ezsv, "{??Name}"_ezsv},
      {"{$?Name}"_ezsv, "{$?Name}"_ezsv},
      {"{{Name}}"_ezsv, "{<Name>}"_ezsv},
      {"100% {Name} & <stuff>"_ezsv, "100% <Name> & <stuff>"_ezsv},

      // placeholders inside quoted sections are resolved as well, the quotes are preserved
      {"Sample Clip: '{Clip}'"_ezsv, "Sample Clip: '<Clip>'"_ezsv},
      {"Log: \"{Text}\""_ezsv, "Log: \"<Text>\""_ezsv},
      {"Set Number: '{BlackboardEntry}' to {Number}"_ezsv, "Set Number: '<BlackboardEntry>' to <Number>"_ezsv},
      {"BlendSpace 1D: '{Clips[0]}' '{Clips[1]}'"_ezsv, "BlendSpace 1D: '<Clips:0>' '<Clips:1>'"_ezsv},
      {"'\"{A}\"'"_ezsv, "'\"<A>\"'"_ezsv},
      {"'{?A}'"_ezsv, "'(A)'"_ezsv},
      {"Log: 'nothing here'"_ezsv, "Log: 'nothing here'"_ezsv},
      {"''"_ezsv, "''"_ezsv},
      {"\"\""_ezsv, "\"\""_ezsv},
    };

    for (const auto& test : tests)
    {
      ezTokenParseUtils::RenderTemplate(test.m_sTemplate, Resolver, sOutput);
      EZ_TEST_STRING(sOutput, test.m_sExpected);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RenderTemplate: optional flag")
  {
    // Mimics a node title that omits optional placeholders.
    auto Resolver = [](ezStringView sPlaceholder, ezVariant index, bool bOptional, ezStringBuilder& ref_sOutput)
    {
      EZ_IGNORE_UNUSED(index);
      if (!bOptional)
      {
        ref_sOutput.Append(sPlaceholder);
      }
    };

    ezStringBuilder sOutput;

    ezTokenParseUtils::RenderTemplate("{?A}"_ezsv, Resolver, sOutput);
    EZ_TEST_STRING(sOutput, "");

    ezTokenParseUtils::RenderTemplate("{?A}{?B}{C}"_ezsv, Resolver, sOutput);
    EZ_TEST_STRING(sOutput, "C");

    ezTokenParseUtils::RenderTemplate("Set {Name} to {?Value}"_ezsv, Resolver, sOutput);
    EZ_TEST_STRING(sOutput, "Set Name to ");

    // the quotes around a dropped placeholder remain, which is what callers have to clean up afterwards
    ezTokenParseUtils::RenderTemplate("['{?A}' '{?B}']"_ezsv, Resolver, sOutput);
    EZ_TEST_STRING(sOutput, "['' '']");
  }
}
