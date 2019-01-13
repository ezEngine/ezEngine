#include <PCH.h>

#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

namespace
{
  bool IsIdentifier(ezUInt32 c) { return !ezStringUtils::IsIdentifierDelimiter_C_Code(c); }

  void SkipWhitespace(ezStringView& s)
  {
    while (s.IsValid() && ezStringUtils::IsWhiteSpace(s.GetCharacter()))
    {
      ++s;
    }
  }

  void ParseAttribute(ezStringView& s, ezShaderParser::ParameterDefinition& def)
  {
    SkipWhitespace(s);

    if (!s.IsValid() || s.GetCharacter() != '@')
      return;

    ++s; // skip @

    const char* szNameStart = s.GetData();
    while (s.IsValid() && IsIdentifier(s.GetCharacter()))
    {
      ++s;
    }

    ezShaderParser::AttributeDefinition attributeDef;
    attributeDef.m_sName = ezStringView(szNameStart, s.GetData());

    SkipWhitespace(s);

    if (!s.IsValid() || s.GetCharacter() != '(')
      return;

    ++s; // skip (
    int braces = 0;

    const char* szValueStart = s.GetData();
    while (s.IsValid())
    {
      if (s.GetCharacter() == '(')
      {
        braces++;

        ++s; // skip (
        szValueStart = s.GetData();
      }
      else
      {
        if (s.GetCharacter() == ')')
        {
          if (braces == 0)
            break;
          braces--;
        }
        ++s;
      }
    }

    if (!s.IsValid() || s.GetCharacter() != ')')
      return;

    ezStringView view = ezStringView(szValueStart, s.GetData());

    ++s; // skip )

    // remove " at the start and end of the string, if given
    if (view.StartsWith("\""))
      view.Shrink(1, 0);
    if (view.EndsWith("\""))
      view.Shrink(0, 1);

    attributeDef.m_sValue = view;
    if (!def.m_sName.IsEmpty())
    {
      def.m_Attributes.PushBack(attributeDef);
    }
  }
} // namespace

// static
void ezShaderParser::ParseMaterialParameterSection(ezStreamReader& stream, ezHybridArray<ParameterDefinition, 16>& out_Parameter)
{
  ezString sContent;
  sContent.ReadAll(stream);

  ezShaderHelper::ezTextSectionizer Sections;
  ezShaderHelper::GetShaderSections(sContent.GetData(), Sections);

  ezUInt32 uiFirstLine = 0;
  ezStringView s = Sections.GetSectionContent(ezShaderHelper::ezShaderSections::MATERIALPARAMETER, uiFirstLine);

  SkipWhitespace(s);

  while (s.IsValid())
  {
    const char* szCurrentStart = s.GetData();

    const char* szTypeStart = s.GetData();
    while (s.IsValid() && IsIdentifier(s.GetCharacter()))
    {
      ++s;
    }

    ParameterDefinition def;
    def.m_sType = ezStringView(szTypeStart, s.GetData());

    SkipWhitespace(s);

    const char* szNameStart = s.GetData();
    while (s.IsValid() && IsIdentifier(s.GetCharacter()))
    {
      ++s;
    }

    def.m_sName = ezStringView(szNameStart, s.GetData());

    SkipWhitespace(s);

    while (s.IsValid() && s.GetCharacter() == '@')
    {
      ParseAttribute(s, def);

      SkipWhitespace(s);
    }

    if (!s.IsValid())
      break;

    if (s.GetCharacter() == ';')
      ++s; // skip ;

    if (!def.m_sType.IsEmpty() && !def.m_sName.IsEmpty())
    {
      out_Parameter.PushBack(def);
    }

    SkipWhitespace(s);

    if (szCurrentStart == s.GetData()) // no change -> parsing error
    {
      ezLog::Error("Error parsing material parameter section of shader");
      break;
    }
  }
}

// static
void ezShaderParser::ParsePermutationSection(ezStreamReader& stream, ezHybridArray<ezHashedString, 16>& out_PermVars,
                                             ezHybridArray<ezPermutationVar, 16>& out_FixedPermVars)
{
  ezString sContent;
  sContent.ReadAll(stream);

  ezShaderHelper::ezTextSectionizer Sections;
  ezShaderHelper::GetShaderSections(sContent.GetData(), Sections);

  ezUInt32 uiFirstLine = 0;
  ezStringView sPermutations = Sections.GetSectionContent(ezShaderHelper::ezShaderSections::PERMUTATIONS, uiFirstLine);
  ParsePermutationSection(sPermutations, out_PermVars, out_FixedPermVars);
}

// static
void ezShaderParser::ParsePermutationSection(ezStringView s, ezHybridArray<ezHashedString, 16>& out_PermVars,
                                             ezHybridArray<ezPermutationVar, 16>& out_FixedPermVars)
{
  out_PermVars.Clear();
  out_FixedPermVars.Clear();

  ezTokenizer tokenizer;
  tokenizer.Tokenize(ezArrayPtr<const ezUInt8>((const ezUInt8*)s.GetData(), s.GetElementCount()), ezLog::GetThreadLocalLogSystem());

  enum class State
  {
    Idle,
    HasName,
    HasEqual,
    HasValue
  };

  State state = State::Idle;
  ezStringBuilder sToken, sVarName;

  for (const auto& token : tokenizer.GetTokens())
  {
    if (token.m_iType == ezTokenType::Whitespace || token.m_iType == ezTokenType::BlockComment || token.m_iType == ezTokenType::LineComment)
      continue;

    if (token.m_iType == ezTokenType::String1 || token.m_iType == ezTokenType::String2)
    {
      sToken = token.m_DataView;
      ezLog::Error("Strings are not allowed in the permutation section: '{0}'", sToken);
      return;
    }

    if (token.m_iType == ezTokenType::Newline || token.m_iType == ezTokenType::EndOfFile)
    {
      if (state == State::HasEqual)
      {
        ezLog::Error("Missing assignment value in permutation section");
        return;
      }

      if (state == State::HasName)
      {
        out_PermVars.ExpandAndGetRef().Assign(sVarName.GetData());
      }

      state = State::Idle;
      continue;
    }

    sToken = token.m_DataView;

    if (token.m_iType == ezTokenType::NonIdentifier)
    {
      if (sToken == "=" && state == State::HasName)
      {
        state = State::HasEqual;
        continue;
      }
    }
    else if (token.m_iType == ezTokenType::Identifier)
    {
      if (state == State::Idle)
      {
        sVarName = sToken;
        state = State::HasName;
        continue;
      }

      if (state == State::HasEqual)
      {
        auto& res = out_FixedPermVars.ExpandAndGetRef();
        res.m_sName.Assign(sVarName.GetData());
        res.m_sValue.Assign(sToken.GetData());
        state = State::HasValue;
        continue;
      }
    }

    ezLog::Error("Invalid permutation section at token '{0}'", sToken);
  }
}

// static
void ezShaderParser::ParsePermutationVarConfig(const char* szPermVarName, ezStringView s, ezVariant& out_DefaultValue,
                                               ezHybridArray<ezHashedString, 16>& out_EnumValues)
{
  SkipWhitespace(s);

  if (s.StartsWith("bool"))
  {
    bool bDefaultValue = false;

    const char* szDefaultValue = s.FindSubString("=");
    if (!ezStringUtils::IsNullOrEmpty(szDefaultValue))
    {
      ++szDefaultValue;
      ezConversionUtils::StringToBool(szDefaultValue, bDefaultValue);
    }

    out_DefaultValue = bDefaultValue;
  }
  else if (s.StartsWith("enum"))
  {
    const ezStringBuilder sEnumPrefix(szPermVarName, "_");

    const char* szOpenBracket = s.FindSubString("{");
    const char* szCloseBracket = s.FindLastSubString("}");

    if (ezStringUtils::IsNullOrEmpty(szOpenBracket) || ezStringUtils::IsNullOrEmpty(szCloseBracket))
    {
      ezLog::Error("No brackets found for enum definition.");
    }

    ezStringBuilder sEnumValues = ezStringView(szOpenBracket + 1, szCloseBracket);

    ezHybridArray<ezStringView, 32> enumValues;
    sEnumValues.Split(false, enumValues, ",");

    ezUInt32 uiDefaultValue = 0;
    ezUInt32 uiCurrentValue = 0;
    for (ezStringView& sName : enumValues)
    {
      sName.Trim(" \r\n\t");

      if (sName.IsEmpty())
        continue;

      const char* szValue = sName.FindSubString("=");
      if (!ezStringUtils::IsNullOrEmpty(szValue))
      {
        // this feature seems to be unused at the moment

        sName = ezStringView(sName.GetStartPosition(), szValue);
        sName.Trim(" \r\n\t");

        ++szValue;

        ezInt32 iValue = 0;
        if (ezConversionUtils::StringToInt(szValue, iValue).Succeeded() && iValue >= 0)
        {
          uiCurrentValue = iValue;
        }
        else
        {
          ezLog::Error("Invalid enum value '{0}'. Only positive numbers are allowed.", szValue);
        }
      }

      // this feature seems to be unused at the moment
      if (sName.IsEqual_NoCase("default"))
      {
        uiDefaultValue = uiCurrentValue;
      }

      if (!sName.StartsWith(sEnumPrefix))
      {
        ezLog::Error("Enum value does not start with the expected enum name as prefix: '{0}'", sEnumPrefix);
      }

      out_EnumValues.EnsureCount(uiCurrentValue + 1);

      if (ezStringUtils::IsNullOrEmpty(out_EnumValues[uiCurrentValue].GetData()))
      {
        const ezStringBuilder sFinalName = sName;
        out_EnumValues[uiCurrentValue].Assign(sFinalName.GetData());
      }
      else
      {
        ezLog::Error("A enum value with '{0}' already exists: '{1}'", uiCurrentValue, out_EnumValues[uiCurrentValue]);
      }

      ++uiCurrentValue;
    }

    out_DefaultValue = uiDefaultValue;
  }
  else
  {
    ezLog::Error("Unknown permutation var type");
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderParser);
