#include <RendererCorePCH.h>

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

  ezVariant ParseValue(const TokenStream& Tokens, ezUInt32& uiCurToken)
  {
    ezUInt32 uiValueToken = uiCurToken;

    if (Accept(Tokens, uiCurToken, ezTokenType::String1, &uiValueToken) || Accept(Tokens, uiCurToken, ezTokenType::String2, &uiValueToken))
    {
      ezStringBuilder sValue = Tokens[uiValueToken]->m_DataView;
      sValue.Trim("\"'");

      return ezVariant(sValue.GetData());
    }

    if (Accept(Tokens, uiCurToken, ezTokenType::Integer, &uiValueToken))
    {
      ezString sValue = Tokens[uiValueToken]->m_DataView;

      ezInt64 iValue = 0;
      if (sValue.StartsWith_NoCase("0x"))
      {
        iValue = ezConversionUtils::ConvertHexStringToUInt32(sValue);
      }
      else
      {
        ezConversionUtils::StringToInt64(sValue, iValue);
      }

      return ezVariant(iValue);
    }

    if (Accept(Tokens, uiCurToken, ezTokenType::Float, &uiValueToken))
    {
      ezString sValue = Tokens[uiValueToken]->m_DataView;

      double fValue = 0;
      ezConversionUtils::StringToFloat(sValue, fValue);

      return ezVariant(fValue);
    }

    if (Accept(Tokens, uiCurToken, "true", &uiValueToken) || Accept(Tokens, uiCurToken, "false", &uiValueToken))
    {
      bool bValue = Tokens[uiValueToken]->m_DataView == "true";
      return ezVariant(bValue);
    }

    auto& dataView = Tokens[uiCurToken]->m_DataView;
    if (Tokens[uiCurToken]->m_iType == ezTokenType::Identifier &&
        ezStringUtils::IsValidIdentifierName(dataView.GetStartPosition(), dataView.GetEndPosition()))
    {
      // complex type constructor
      const ezRTTI* pType = nullptr;
      if (!s_NameToTypeTable.TryGetValue(dataView, pType))
      {
        ezLog::Error("Invalid type name '{}'", dataView);
        return ezVariant();
      }

      ++uiCurToken;
      Accept(Tokens, uiCurToken, "(");

      ezHybridArray<ezVariant, 8> constructorArgs;

      while (!Accept(Tokens, uiCurToken, ")"))
      {
        ezVariant value = ParseValue(Tokens, uiCurToken);
        if (value.IsValid())
        {
          constructorArgs.PushBack(value);
        }
        else
        {
          ezLog::Error("Invalid arguments for constructor '{}'", pType->GetTypeName());
          return EZ_FAILURE;
        }

        Accept(Tokens, uiCurToken, ",");
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

  ezResult ParseAttribute(const TokenStream& Tokens, ezUInt32& uiCurToken, ezShaderParser::ParameterDefinition& out_ParameterDefinition)
  {
    if (!Accept(Tokens, uiCurToken, "@"))
    {
      return EZ_FAILURE;
    }

    ezUInt32 uiTypeToken = uiCurToken;
    if (!Accept(Tokens, uiCurToken, ezTokenType::Identifier, &uiTypeToken))
    {
      return EZ_FAILURE;
    }

    ezShaderParser::AttributeDefinition& attributeDef = out_ParameterDefinition.m_Attributes.ExpandAndGetRef();
    attributeDef.m_sType = Tokens[uiTypeToken]->m_DataView;

    Accept(Tokens, uiCurToken, "(");

    while (!Accept(Tokens, uiCurToken, ")"))
    {
      ezVariant value = ParseValue(Tokens, uiCurToken);
      if (value.IsValid())
      {
        attributeDef.m_Values.PushBack(value);
      }
      else
      {
        ezLog::Error("Invalid arguments for attribute '{}'", attributeDef.m_sType);
        return EZ_FAILURE;
      }

      Accept(Tokens, uiCurToken, ",");
    }

    return EZ_SUCCESS;
  }

  ezResult ParseParameter(const TokenStream& Tokens, ezUInt32& uiCurToken, ezShaderParser::ParameterDefinition& out_ParameterDefinition)
  {
    ezUInt32 uiTypeToken = uiCurToken;
    if (!Accept(Tokens, uiCurToken, ezTokenType::Identifier, &uiTypeToken))
    {
      return EZ_FAILURE;
    }

    out_ParameterDefinition.m_sType = Tokens[uiTypeToken]->m_DataView;
    out_ParameterDefinition.m_pType = GetType(out_ParameterDefinition.m_sType);

    ezUInt32 uiNameToken = uiCurToken;
    if (!Accept(Tokens, uiCurToken, ezTokenType::Identifier, &uiNameToken))
    {
      return EZ_FAILURE;
    }

    out_ParameterDefinition.m_sName = Tokens[uiNameToken]->m_DataView;

    while (!Accept(Tokens, uiCurToken, ";"))
    {
      if (ParseAttribute(Tokens, uiCurToken, out_ParameterDefinition).Failed())
      {
        return EZ_FAILURE;
      }
    }

    return EZ_SUCCESS;
  }

  ezResult ParseEnum(const TokenStream& Tokens, ezUInt32& uiCurToken, ezShaderParser::EnumDefinition& out_EnumDefinition)
  {
    if (!Accept(Tokens, uiCurToken, "enum"))
    {
      return EZ_FAILURE;
    }

    ezUInt32 uiNameToken = uiCurToken;
    if (!Accept(Tokens, uiCurToken, ezTokenType::Identifier, &uiNameToken))
    {
      return EZ_FAILURE;
    }

    out_EnumDefinition.m_sName = Tokens[uiNameToken]->m_DataView;
    ezStringBuilder sEnumPrefix(out_EnumDefinition.m_sName, "_");

    if (!Accept(Tokens, uiCurToken, "{"))
    {
      ezLog::Error("Opening bracket expected for enum definition.");
      return EZ_FAILURE;
    }

    ezUInt32 uiDefaultValue = 0;
    ezUInt32 uiCurrentValue = 0;

    while (true)
    {
      ezUInt32 uiValueNameToken = uiCurToken;
      if (!Accept(Tokens, uiCurToken, ezTokenType::Identifier, &uiValueNameToken))
      {
        return EZ_FAILURE;
      }

      ezStringView sValueName = Tokens[uiValueNameToken]->m_DataView;

      if (Accept(Tokens, uiCurToken, "="))
      {
        ezUInt32 uiValueToken = uiCurToken;
        Accept(Tokens, uiCurToken, ezTokenType::Integer, &uiValueToken);

        ezInt32 iValue = 0;
        if (ezConversionUtils::StringToInt(Tokens[uiValueToken]->m_DataView.GetData(), iValue).Succeeded() && iValue >= 0)
        {
          uiCurrentValue = iValue;
        }
        else
        {
          ezLog::Error("Invalid enum value '{0}'. Only positive numbers are allowed.", Tokens[uiValueToken]->m_DataView);
        }
      }

      if (sValueName.IsEqual_NoCase("default"))
      {
        uiDefaultValue = uiCurrentValue;
      }

      if (!sValueName.StartsWith(sEnumPrefix))
      {
        ezLog::Error("Enum value does not start with the expected enum name as prefix: '{0}'", sEnumPrefix);
      }

      out_EnumDefinition.m_Values.EnsureCount(uiCurrentValue + 1);

      if (ezStringUtils::IsNullOrEmpty(out_EnumDefinition.m_Values[uiCurrentValue].GetData()))
      {
        const ezStringBuilder sFinalName = sValueName;
        out_EnumDefinition.m_Values[uiCurrentValue].Assign(sFinalName.GetData());
      }
      else
      {
        ezLog::Error("A enum value with '{0}' already exists: '{1}'", uiCurrentValue, out_EnumDefinition.m_Values[uiCurrentValue]);
      }

      if (Accept(Tokens, uiCurToken, ","))
      {
        ++uiCurrentValue;
      }
      else
      {
        break;
      }
    }

    out_EnumDefinition.m_uiDefaultValue = uiDefaultValue;

    if (!Accept(Tokens, uiCurToken, "}"))
    {
      ezLog::Error("Closing bracket expected for enum definition.");
      return EZ_FAILURE;
    }

    Accept(Tokens, uiCurToken, ";");

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
void ezShaderParser::ParseMaterialParameterSection(
  ezStreamReader& stream, ezHybridArray<ParameterDefinition, 16>& out_Parameter, ezHybridArray<EnumDefinition, 4>& out_EnumDefinitions)
{
  ezString sContent;
  sContent.ReadAll(stream);

  ezShaderHelper::ezTextSectionizer Sections;
  ezShaderHelper::GetShaderSections(sContent.GetData(), Sections);

  ezUInt32 uiFirstLine = 0;
  ezStringView s = Sections.GetSectionContent(ezShaderHelper::ezShaderSections::MATERIALPARAMETER, uiFirstLine);

  ezTokenizer tokenizer;
  tokenizer.Tokenize(ezArrayPtr<const ezUInt8>((const ezUInt8*)s.GetData(), s.GetElementCount()), ezLog::GetThreadLocalLogSystem());

  TokenStream tokens;
  tokenizer.GetAllLines(tokens);

  ezUInt32 uiCurToken = 0;

  while (!Accept(tokens, uiCurToken, ezTokenType::EndOfFile))
  {
    EnumDefinition enumDef;
    if (ParseEnum(tokens, uiCurToken, enumDef).Succeeded())
    {
      out_EnumDefinitions.PushBack(std::move(enumDef));
      continue;
    }

    ParameterDefinition paramDef;
    if (ParseParameter(tokens, uiCurToken, paramDef).Succeeded())
    {
      out_Parameter.PushBack(std::move(paramDef));
      continue;
    }

    ezLog::Error("Invalid token in material parameter section '{}'", tokens[uiCurToken]->m_DataView);
    break;
  }
}

// static
void ezShaderParser::ParsePermutationSection(
  ezStreamReader& stream, ezHybridArray<ezHashedString, 16>& out_PermVars, ezHybridArray<ezPermutationVar, 16>& out_FixedPermVars)
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
void ezShaderParser::ParsePermutationSection(
  ezStringView s, ezHybridArray<ezHashedString, 16>& out_PermVars, ezHybridArray<ezPermutationVar, 16>& out_FixedPermVars)
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
void ezShaderParser::ParsePermutationVarConfig(ezStringView s, ezVariant& out_DefaultValue, EnumDefinition& out_EnumDefinition)
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
    ezTokenizer tokenizer;
    tokenizer.Tokenize(ezArrayPtr<const ezUInt8>((const ezUInt8*)s.GetData(), s.GetElementCount()), ezLog::GetThreadLocalLogSystem());

    TokenStream tokens;
    tokenizer.GetAllLines(tokens);

    ezUInt32 uiCurToken = 0;
    ParseEnum(tokens, uiCurToken, out_EnumDefinition);

    out_DefaultValue = out_EnumDefinition.m_uiDefaultValue;
  }
  else
  {
    ezLog::Error("Unknown permutation var type");
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderParser);
