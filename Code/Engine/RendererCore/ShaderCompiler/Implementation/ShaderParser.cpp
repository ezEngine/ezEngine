#include <RendererCore/RendererCorePCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

using namespace ezTokenParseUtils;

namespace
{
  static ezHashTable<ezStringView, const ezRTTI*> s_NameToTypeTable;

  void InitializeTables()
  {
    if (!s_NameToTypeTable.IsEmpty())
      return;

    s_NameToTypeTable.Insert("float", ezGetStaticRTTI<float>());
    s_NameToTypeTable.Insert("float2", ezGetStaticRTTI<ezVec2>());
    s_NameToTypeTable.Insert("float3", ezGetStaticRTTI<ezVec3>());
    s_NameToTypeTable.Insert("float4", ezGetStaticRTTI<ezVec4>());
    s_NameToTypeTable.Insert("int", ezGetStaticRTTI<int>());
    s_NameToTypeTable.Insert("int2", ezGetStaticRTTI<ezVec2I32>());
    s_NameToTypeTable.Insert("int3", ezGetStaticRTTI<ezVec3I32>());
    s_NameToTypeTable.Insert("int4", ezGetStaticRTTI<ezVec4I32>());
    s_NameToTypeTable.Insert("uint", ezGetStaticRTTI<ezUInt32>());
    s_NameToTypeTable.Insert("uint2", ezGetStaticRTTI<ezVec2U32>());
    s_NameToTypeTable.Insert("uint3", ezGetStaticRTTI<ezVec3U32>());
    s_NameToTypeTable.Insert("uint4", ezGetStaticRTTI<ezVec4U32>());
    s_NameToTypeTable.Insert("bool", ezGetStaticRTTI<bool>());
    s_NameToTypeTable.Insert("Color", ezGetStaticRTTI<ezColor>());
    /// \todo Are we going to support linear UB colors ?
    s_NameToTypeTable.Insert("Texture2D", ezGetStaticRTTI<ezString>());
    s_NameToTypeTable.Insert("Texture3D", ezGetStaticRTTI<ezString>());
    s_NameToTypeTable.Insert("TextureCube", ezGetStaticRTTI<ezString>());
  }

  const ezRTTI* GetType(ezStringView sType)
  {
    InitializeTables();

    const ezRTTI* pType = nullptr;
    s_NameToTypeTable.TryGetValue(sType, pType);
    return pType;
  }

  ezVariant ParseValue(const TokenStream& tokens, ezUInt32& ref_uiCurToken)
  {
    ezUInt32 uiValueToken = ref_uiCurToken;

    if (Accept(tokens, ref_uiCurToken, ezTokenType::String1, &uiValueToken) || Accept(tokens, ref_uiCurToken, ezTokenType::String2, &uiValueToken))
    {
      ezStringBuilder sValue = tokens[uiValueToken]->m_DataView;
      sValue.Trim("\"'");

      return ezVariant(sValue.GetData());
    }

    if (Accept(tokens, ref_uiCurToken, ezTokenType::Integer, &uiValueToken))
    {
      ezString sValue = tokens[uiValueToken]->m_DataView;

      ezInt64 iValue = 0;
      if (sValue.StartsWith_NoCase("0x"))
      {
        ezUInt32 uiValue32 = 0;
        ezConversionUtils::ConvertHexStringToUInt32(sValue, uiValue32).IgnoreResult();

        iValue = uiValue32;
      }
      else
      {
        ezConversionUtils::StringToInt64(sValue, iValue).IgnoreResult();
      }

      return ezVariant(iValue);
    }

    if (Accept(tokens, ref_uiCurToken, ezTokenType::Float, &uiValueToken))
    {
      ezString sValue = tokens[uiValueToken]->m_DataView;

      double fValue = 0;
      ezConversionUtils::StringToFloat(sValue, fValue).IgnoreResult();

      return ezVariant(fValue);
    }

    if (Accept(tokens, ref_uiCurToken, "true", &uiValueToken) || Accept(tokens, ref_uiCurToken, "false", &uiValueToken))
    {
      bool bValue = tokens[uiValueToken]->m_DataView == "true";
      return ezVariant(bValue);
    }

    auto& dataView = tokens[ref_uiCurToken]->m_DataView;
    if (tokens[ref_uiCurToken]->m_iType == ezTokenType::Identifier && ezStringUtils::IsValidIdentifierName(dataView.GetStartPointer(), dataView.GetEndPointer()))
    {
      // complex type constructor
      const ezRTTI* pType = nullptr;
      if (!s_NameToTypeTable.TryGetValue(dataView, pType))
      {
        ezLog::Error("Invalid type name '{}'", dataView);
        return ezVariant();
      }

      ++ref_uiCurToken;
      Accept(tokens, ref_uiCurToken, "(");

      ezHybridArray<ezVariant, 8> constructorArgs;

      while (!Accept(tokens, ref_uiCurToken, ")"))
      {
        ezVariant value = ParseValue(tokens, ref_uiCurToken);
        if (value.IsValid())
        {
          constructorArgs.PushBack(value);
        }
        else
        {
          ezLog::Error("Invalid arguments for constructor '{}'", pType->GetTypeName());
          return EZ_FAILURE;
        }

        Accept(tokens, ref_uiCurToken, ",");
      }

      // find matching constructor
      auto& functions = pType->GetFunctions();
      for (auto pFunc : functions)
      {
        if (pFunc->GetFunctionType() == ezFunctionType::Constructor && pFunc->GetArgumentCount() == constructorArgs.GetCount())
        {
          ezHybridArray<ezVariant, 8> convertedArgs;
          bool bAllArgsValid = true;

          for (ezUInt32 uiArg = 0; uiArg < pFunc->GetArgumentCount(); ++uiArg)
          {
            const ezRTTI* pArgType = pFunc->GetArgumentType(uiArg);
            ezResult conversionResult = EZ_FAILURE;
            convertedArgs.PushBack(constructorArgs[uiArg].ConvertTo(pArgType->GetVariantType(), &conversionResult));
            if (conversionResult.Failed())
            {
              bAllArgsValid = false;
              break;
            }
          }

          if (bAllArgsValid)
          {
            ezVariant result;
            pFunc->Execute(nullptr, convertedArgs, result);

            if (result.IsValid())
            {
              return result;
            }
          }
        }
      }
    }

    return ezVariant();
  }

  ezResult ParseAttribute(const TokenStream& tokens, ezUInt32& ref_uiCurToken, ezShaderParser::ParameterDefinition& out_parameterDefinition)
  {
    if (!Accept(tokens, ref_uiCurToken, "@"))
    {
      return EZ_FAILURE;
    }

    ezUInt32 uiTypeToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, ezTokenType::Identifier, &uiTypeToken))
    {
      return EZ_FAILURE;
    }

    ezShaderParser::AttributeDefinition& attributeDef = out_parameterDefinition.m_Attributes.ExpandAndGetRef();
    attributeDef.m_sType = tokens[uiTypeToken]->m_DataView;

    Accept(tokens, ref_uiCurToken, "(");

    while (!Accept(tokens, ref_uiCurToken, ")"))
    {
      ezVariant value = ParseValue(tokens, ref_uiCurToken);
      if (value.IsValid())
      {
        attributeDef.m_Values.PushBack(value);
      }
      else
      {
        ezLog::Error("Invalid arguments for attribute '{}'", attributeDef.m_sType);
        return EZ_FAILURE;
      }

      Accept(tokens, ref_uiCurToken, ",");
    }

    return EZ_SUCCESS;
  }

  ezResult ParseParameter(const TokenStream& tokens, ezUInt32& ref_uiCurToken, ezShaderParser::ParameterDefinition& out_parameterDefinition)
  {
    ezUInt32 uiTypeToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, ezTokenType::Identifier, &uiTypeToken))
    {
      return EZ_FAILURE;
    }

    out_parameterDefinition.m_sType = tokens[uiTypeToken]->m_DataView;
    out_parameterDefinition.m_pType = GetType(out_parameterDefinition.m_sType);

    ezUInt32 uiNameToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, ezTokenType::Identifier, &uiNameToken))
    {
      return EZ_FAILURE;
    }

    out_parameterDefinition.m_sName = tokens[uiNameToken]->m_DataView;

    while (!Accept(tokens, ref_uiCurToken, ";"))
    {
      if (ParseAttribute(tokens, ref_uiCurToken, out_parameterDefinition).Failed())
      {
        return EZ_FAILURE;
      }
    }

    return EZ_SUCCESS;
  }

  ezResult ParseEnum(const TokenStream& tokens, ezUInt32& ref_uiCurToken, ezShaderParser::EnumDefinition& out_enumDefinition, bool bCheckPrefix)
  {
    if (!Accept(tokens, ref_uiCurToken, "enum"))
    {
      return EZ_FAILURE;
    }

    ezUInt32 uiNameToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, ezTokenType::Identifier, &uiNameToken))
    {
      return EZ_FAILURE;
    }

    out_enumDefinition.m_sName = tokens[uiNameToken]->m_DataView;
    ezStringBuilder sEnumPrefix(out_enumDefinition.m_sName, "_");

    if (!Accept(tokens, ref_uiCurToken, "{"))
    {
      ezLog::Error("Opening bracket expected for enum definition.");
      return EZ_FAILURE;
    }

    ezUInt32 uiDefaultValue = 0;
    ezUInt32 uiCurrentValue = 0;

    while (true)
    {
      ezUInt32 uiValueNameToken = ref_uiCurToken;
      if (!Accept(tokens, ref_uiCurToken, ezTokenType::Identifier, &uiValueNameToken))
      {
        return EZ_FAILURE;
      }

      ezStringView sValueName = tokens[uiValueNameToken]->m_DataView;

      if (Accept(tokens, ref_uiCurToken, "="))
      {
        ezUInt32 uiValueToken = ref_uiCurToken;
        Accept(tokens, ref_uiCurToken, ezTokenType::Integer, &uiValueToken);

        ezInt32 iValue = 0;
        if (ezConversionUtils::StringToInt(tokens[uiValueToken]->m_DataView.GetStartPointer(), iValue).Succeeded() && iValue >= 0)
        {
          uiCurrentValue = iValue;
        }
        else
        {
          ezLog::Error("Invalid enum value '{0}'. Only positive numbers are allowed.", tokens[uiValueToken]->m_DataView);
        }
      }

      if (sValueName.IsEqual_NoCase("default"))
      {
        uiDefaultValue = uiCurrentValue;
      }
      else
      {
        if (bCheckPrefix && !sValueName.StartsWith(sEnumPrefix))
        {
          ezLog::Error("Enum value does not start with the expected enum name as prefix: '{0}'", sEnumPrefix);
        }

        auto& ev = out_enumDefinition.m_Values.ExpandAndGetRef();

        const ezStringBuilder sFinalName = sValueName;
        ev.m_sValueName.Assign(sFinalName.GetData());
        ev.m_iValueValue = static_cast<ezInt32>(uiCurrentValue);
      }

      if (Accept(tokens, ref_uiCurToken, ","))
      {
        ++uiCurrentValue;
      }
      else
      {
        break;
      }

      if (Accept(tokens, ref_uiCurToken, "}"))
        goto after_braces;
    }

    if (!Accept(tokens, ref_uiCurToken, "}"))
    {
      ezLog::Error("Closing bracket expected for enum definition.");
      return EZ_FAILURE;
    }

  after_braces:

    out_enumDefinition.m_uiDefaultValue = uiDefaultValue;

    Accept(tokens, ref_uiCurToken, ";");

    return EZ_SUCCESS;
  }

  void SkipWhitespace(ezStringView& s)
  {
    while (s.IsValid() && ezStringUtils::IsWhiteSpace(s.GetCharacter()))
    {
      ++s;
    }
  }
} // namespace

// static
void ezShaderParser::ParseMaterialParameterSection(ezStreamReader& inout_stream, ezHybridArray<ParameterDefinition, 16>& out_parameter, ezHybridArray<EnumDefinition, 4>& out_enumDefinitions)
{
  ezString sContent;
  sContent.ReadAll(inout_stream);

  ezShaderHelper::ezTextSectionizer Sections;
  ezShaderHelper::GetShaderSections(sContent.GetData(), Sections);

  ezUInt32 uiFirstLine = 0;
  ezStringView s = Sections.GetSectionContent(ezShaderHelper::ezShaderSections::MATERIALPARAMETER, uiFirstLine);

  ezTokenizer tokenizer;
  tokenizer.Tokenize(ezArrayPtr<const ezUInt8>((const ezUInt8*)s.GetStartPointer(), s.GetElementCount()), ezLog::GetThreadLocalLogSystem());

  TokenStream tokens;
  tokenizer.GetAllLines(tokens);

  ezUInt32 uiCurToken = 0;

  while (!Accept(tokens, uiCurToken, ezTokenType::EndOfFile))
  {
    EnumDefinition enumDef;
    if (ParseEnum(tokens, uiCurToken, enumDef, false).Succeeded())
    {
      EZ_ASSERT_DEV(!enumDef.m_sName.IsEmpty(), "");

      out_enumDefinitions.PushBack(std::move(enumDef));
      continue;
    }

    ParameterDefinition paramDef;
    if (ParseParameter(tokens, uiCurToken, paramDef).Succeeded())
    {
      out_parameter.PushBack(std::move(paramDef));
      continue;
    }

    ezLog::Error("Invalid token in material parameter section '{}'", tokens[uiCurToken]->m_DataView);
    break;
  }
}

// static
void ezShaderParser::ParsePermutationSection(ezStreamReader& inout_stream, ezHybridArray<ezHashedString, 16>& out_permVars, ezHybridArray<ezPermutationVar, 16>& out_fixedPermVars)
{
  ezString sContent;
  sContent.ReadAll(inout_stream);

  ezShaderHelper::ezTextSectionizer Sections;
  ezShaderHelper::GetShaderSections(sContent.GetData(), Sections);

  ezUInt32 uiFirstLine = 0;
  ezStringView sPermutations = Sections.GetSectionContent(ezShaderHelper::ezShaderSections::PERMUTATIONS, uiFirstLine);
  ParsePermutationSection(sPermutations, out_permVars, out_fixedPermVars);
}

// static
void ezShaderParser::ParsePermutationSection(ezStringView s, ezHybridArray<ezHashedString, 16>& out_permVars, ezHybridArray<ezPermutationVar, 16>& out_fixedPermVars)
{
  out_permVars.Clear();
  out_fixedPermVars.Clear();

  ezTokenizer tokenizer;
  tokenizer.Tokenize(ezArrayPtr<const ezUInt8>((const ezUInt8*)s.GetStartPointer(), s.GetElementCount()), ezLog::GetThreadLocalLogSystem());

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
        out_permVars.ExpandAndGetRef().Assign(sVarName.GetData());
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
        auto& res = out_fixedPermVars.ExpandAndGetRef();
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
void ezShaderParser::ParsePermutationVarConfig(ezStringView s, ezVariant& out_defaultValue, EnumDefinition& out_enumDefinition)
{
  SkipWhitespace(s);

  ezStringBuilder name;

  if (s.StartsWith("bool"))
  {
    bool bDefaultValue = false;

    const char* szDefaultValue = s.FindSubString("=");
    if (szDefaultValue != nullptr)
    {
      name.SetSubString_FromTo(s.GetStartPointer() + 4, szDefaultValue);

      ++szDefaultValue;
      ezConversionUtils::StringToBool(szDefaultValue, bDefaultValue).IgnoreResult();
    }
    else
    {
      name.SetSubString_FromTo(s.GetStartPointer() + 4, s.GetEndPointer());
    }

    name.Trim(" \t\r\n");
    out_enumDefinition.m_sName = name;
    out_defaultValue = bDefaultValue;
  }
  else if (s.StartsWith("enum"))
  {
    ezTokenizer tokenizer;
    tokenizer.Tokenize(ezArrayPtr<const ezUInt8>((const ezUInt8*)s.GetStartPointer(), s.GetElementCount()), ezLog::GetThreadLocalLogSystem());

    TokenStream tokens;
    tokenizer.GetAllLines(tokens);

    ezUInt32 uiCurToken = 0;
    if (ParseEnum(tokens, uiCurToken, out_enumDefinition, true).Failed())
    {
      ezLog::Error("Invalid enum PermutationVar definition.");
    }
    else
    {
      EZ_ASSERT_DEV(!out_enumDefinition.m_sName.IsEmpty(), "");

      out_defaultValue = out_enumDefinition.m_uiDefaultValue;
    }
  }
  else
  {
    ezLog::Error("Unknown permutation var type");
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderParser);
