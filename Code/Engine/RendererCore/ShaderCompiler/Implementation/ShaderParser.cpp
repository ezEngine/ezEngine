#include <RendererCore/RendererCorePCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <RendererFoundation/Shader/Types.h>

using namespace ezTokenParseUtils;

namespace
{
  static ezHashTable<ezStringView, const ezRTTI*> s_NameToTypeTable;
  static ezHashTable<ezStringView, ezEnum<ezGALShaderResourceType>> s_NameToDescriptorTable;
  static ezHashTable<ezStringView, ezEnum<ezGALShaderTextureType>> s_NameToTextureTable;
  static ezHashTable<ezStringView, ezEnum<ezShaderConstant::Type>> s_NameToShaderConstantTable;
  static ezDynamicArray<ezUInt32> s_ShaderConstantSize;
  static ezDynamicArray<ezUInt32> s_ShaderConstantScalarSize;

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

    s_NameToDescriptorTable.Insert("cbuffer"_ezsv, ezGALShaderResourceType::ConstantBuffer);
    s_NameToDescriptorTable.Insert("ConstantBuffer"_ezsv, ezGALShaderResourceType::ConstantBuffer);
    s_NameToDescriptorTable.Insert("SamplerState"_ezsv, ezGALShaderResourceType::Sampler);
    s_NameToDescriptorTable.Insert("SamplerComparisonState"_ezsv, ezGALShaderResourceType::Sampler);
    s_NameToDescriptorTable.Insert("Texture1D"_ezsv, ezGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("Texture1DArray"_ezsv, ezGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("Texture2D"_ezsv, ezGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("Texture2DArray"_ezsv, ezGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("Texture2DMS"_ezsv, ezGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("Texture2DMSArray"_ezsv, ezGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("Texture3D"_ezsv, ezGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("TextureCube"_ezsv, ezGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("TextureCubeArray"_ezsv, ezGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("Buffer"_ezsv, ezGALShaderResourceType::TexelBuffer);
    s_NameToDescriptorTable.Insert("StructuredBuffer"_ezsv, ezGALShaderResourceType::StructuredBuffer);
    s_NameToDescriptorTable.Insert("ByteAddressBuffer"_ezsv, ezGALShaderResourceType::StructuredBuffer);
    s_NameToDescriptorTable.Insert("RWTexture1D"_ezsv, ezGALShaderResourceType::TextureRW);
    s_NameToDescriptorTable.Insert("RWTexture1DArray"_ezsv, ezGALShaderResourceType::TextureRW);
    s_NameToDescriptorTable.Insert("RWTexture2D"_ezsv, ezGALShaderResourceType::TextureRW);
    s_NameToDescriptorTable.Insert("RWTexture2DArray"_ezsv, ezGALShaderResourceType::TextureRW);
    s_NameToDescriptorTable.Insert("RWTexture3D"_ezsv, ezGALShaderResourceType::TextureRW);
    s_NameToDescriptorTable.Insert("RWBuffer"_ezsv, ezGALShaderResourceType::TexelBufferRW);
    s_NameToDescriptorTable.Insert("RWStructuredBuffer"_ezsv, ezGALShaderResourceType::StructuredBufferRW);
    s_NameToDescriptorTable.Insert("RWByteAddressBuffer"_ezsv, ezGALShaderResourceType::StructuredBufferRW);
    s_NameToDescriptorTable.Insert("AppendStructuredBuffer"_ezsv, ezGALShaderResourceType::StructuredBufferRW);
    s_NameToDescriptorTable.Insert("ConsumeStructuredBuffer"_ezsv, ezGALShaderResourceType::StructuredBufferRW);

    s_NameToTextureTable.Insert("Texture1D"_ezsv, ezGALShaderTextureType::Texture1D);
    s_NameToTextureTable.Insert("Texture1DArray"_ezsv, ezGALShaderTextureType::Texture1DArray);
    s_NameToTextureTable.Insert("Texture2D"_ezsv, ezGALShaderTextureType::Texture2D);
    s_NameToTextureTable.Insert("Texture2DArray"_ezsv, ezGALShaderTextureType::Texture2DArray);
    s_NameToTextureTable.Insert("Texture2DMS"_ezsv, ezGALShaderTextureType::Texture2DMS);
    s_NameToTextureTable.Insert("Texture2DMSArray"_ezsv, ezGALShaderTextureType::Texture2DMSArray);
    s_NameToTextureTable.Insert("Texture3D"_ezsv, ezGALShaderTextureType::Texture3D);
    s_NameToTextureTable.Insert("TextureCube"_ezsv, ezGALShaderTextureType::TextureCube);
    s_NameToTextureTable.Insert("TextureCubeArray"_ezsv, ezGALShaderTextureType::TextureCubeArray);
    s_NameToTextureTable.Insert("RWTexture1D"_ezsv, ezGALShaderTextureType::Texture1D);
    s_NameToTextureTable.Insert("RWTexture1DArray"_ezsv, ezGALShaderTextureType::Texture1DArray);
    s_NameToTextureTable.Insert("RWTexture2D"_ezsv, ezGALShaderTextureType::Texture2D);
    s_NameToTextureTable.Insert("RWTexture2DArray"_ezsv, ezGALShaderTextureType::Texture2DArray);
    s_NameToTextureTable.Insert("RWTexture3D"_ezsv, ezGALShaderTextureType::Texture3D);

    s_NameToShaderConstantTable.Insert("FLOAT1"_ezsv, ezShaderConstant::Type::Float1);
    s_NameToShaderConstantTable.Insert("FLOAT2"_ezsv, ezShaderConstant::Type::Float2);
    s_NameToShaderConstantTable.Insert("FLOAT3"_ezsv, ezShaderConstant::Type::Float3);
    s_NameToShaderConstantTable.Insert("FLOAT4"_ezsv, ezShaderConstant::Type::Float4);
    s_NameToShaderConstantTable.Insert("INT1"_ezsv, ezShaderConstant::Type::Int1);
    s_NameToShaderConstantTable.Insert("INT2"_ezsv, ezShaderConstant::Type::Int2);
    s_NameToShaderConstantTable.Insert("INT3"_ezsv, ezShaderConstant::Type::Int3);
    s_NameToShaderConstantTable.Insert("INT4"_ezsv, ezShaderConstant::Type::Int4);
    s_NameToShaderConstantTable.Insert("UINT1"_ezsv, ezShaderConstant::Type::UInt1);
    s_NameToShaderConstantTable.Insert("UINT2"_ezsv, ezShaderConstant::Type::UInt2);
    s_NameToShaderConstantTable.Insert("UINT3"_ezsv, ezShaderConstant::Type::UInt3);
    s_NameToShaderConstantTable.Insert("UINT4"_ezsv, ezShaderConstant::Type::UInt4);
    s_NameToShaderConstantTable.Insert("MAT3"_ezsv, ezShaderConstant::Type::Mat3x3);
    s_NameToShaderConstantTable.Insert("MAT4"_ezsv, ezShaderConstant::Type::Mat4x4);
    s_NameToShaderConstantTable.Insert("TRANSFORM"_ezsv, ezShaderConstant::Type::Transform);
    s_NameToShaderConstantTable.Insert("COLOR4F"_ezsv, ezShaderConstant::Type::Float4);
    s_NameToShaderConstantTable.Insert("BOOL1"_ezsv, ezShaderConstant::Type::Bool);
    // Handled separately
    // s_NameToShaderConstantTable.Insert("PACKEDHALF2"_ezsv, ezShaderConstant::Type::UInt1);
    // s_NameToShaderConstantTable.Insert("PACKEDCOLOR4H"_ezsv, ezShaderConstant::Type::Bool);

    s_ShaderConstantSize.SetCount(ezShaderConstant::Type::ENUM_COUNT);
    s_ShaderConstantSize[ezShaderConstant::Type::Float1] = sizeof(float);
    s_ShaderConstantSize[ezShaderConstant::Type::Float2] = sizeof(ezVec2);
    s_ShaderConstantSize[ezShaderConstant::Type::Float3] = sizeof(ezVec3);
    s_ShaderConstantSize[ezShaderConstant::Type::Float4] = sizeof(ezVec4);
    s_ShaderConstantSize[ezShaderConstant::Type::Int1] = sizeof(ezInt32);
    s_ShaderConstantSize[ezShaderConstant::Type::Int2] = sizeof(ezVec2I32);
    s_ShaderConstantSize[ezShaderConstant::Type::Int3] = sizeof(ezVec3I32);
    s_ShaderConstantSize[ezShaderConstant::Type::Int4] = sizeof(ezVec4I32);
    s_ShaderConstantSize[ezShaderConstant::Type::UInt1] = sizeof(ezUInt32);
    s_ShaderConstantSize[ezShaderConstant::Type::UInt2] = sizeof(ezVec2U32);
    s_ShaderConstantSize[ezShaderConstant::Type::UInt3] = sizeof(ezVec3U32);
    s_ShaderConstantSize[ezShaderConstant::Type::UInt4] = sizeof(ezVec4U32);
    s_ShaderConstantSize[ezShaderConstant::Type::Mat3x3] = sizeof(ezShaderMat3);
    s_ShaderConstantSize[ezShaderConstant::Type::Mat4x4] = sizeof(ezShaderMat4);
    s_ShaderConstantSize[ezShaderConstant::Type::Transform] = sizeof(ezShaderTransform);
    s_ShaderConstantSize[ezShaderConstant::Type::Bool] = sizeof(ezShaderBool);
    s_ShaderConstantSize[ezShaderConstant::Type::Struct] = 0;

    s_ShaderConstantScalarSize.SetCount(ezShaderConstant::Type::ENUM_COUNT);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::Float1] = sizeof(float);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::Float2] = sizeof(float);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::Float3] = sizeof(float);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::Float4] = sizeof(float);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::Int1] = sizeof(ezInt32);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::Int2] = sizeof(ezInt32);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::Int3] = sizeof(ezInt32);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::Int4] = sizeof(ezInt32);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::UInt1] = sizeof(ezUInt32);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::UInt2] = sizeof(ezUInt32);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::UInt3] = sizeof(ezUInt32);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::UInt4] = sizeof(ezUInt32);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::Mat3x3] = sizeof(float);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::Mat4x4] = sizeof(float);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::Transform] = sizeof(float);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::Bool] = sizeof(ezShaderBool);
    s_ShaderConstantScalarSize[ezShaderConstant::Type::Struct] = 0;
  }

  const ezRTTI* GetType(const char* szType)
  {
    InitializeTables();

    const ezRTTI* pType = nullptr;
    s_NameToTypeTable.TryGetValue(szType, pType);
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

    bool bValueIsNegative = false;
    if (Accept(tokens, ref_uiCurToken, "-"))
    {
      bValueIsNegative = true;
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

      return ezVariant(bValueIsNegative ? -iValue : iValue);
    }

    if (Accept(tokens, ref_uiCurToken, ezTokenType::Float, &uiValueToken))
    {
      ezString sValue = tokens[uiValueToken]->m_DataView;

      double fValue = 0;
      ezConversionUtils::StringToFloat(sValue, fValue).IgnoreResult();

      return ezVariant(bValueIsNegative ? -fValue : fValue);
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
      auto functions = pType->GetFunctions();
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
        if (ezConversionUtils::StringToInt(tokens[uiValueToken]->m_DataView, iValue).Succeeded() && iValue >= 0)
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

ezResult ezShaderParser::PreprocessSection(ezStreamReader& inout_stream, ezShaderHelper::ezShaderSections::Enum section, ezArrayPtr<ezString> customDefines, ezStringBuilder& out_sResult)
{
  ezString sContent;
  sContent.ReadAll(inout_stream);

  ezShaderHelper::ezTextSectionizer sections;
  ezShaderHelper::GetShaderSections(sContent, sections);

  ezUInt32 uiFirstLine = 0;
  ezStringView sSectionContent = sections.GetSectionContent(section, uiFirstLine);

  ezPreprocessor pp;
  pp.SetPassThroughPragma(false);
  pp.SetPassThroughLine(false);

  // setup defines
  {
    EZ_SUCCEED_OR_RETURN(pp.AddCustomDefine("TRUE 1"));
    EZ_SUCCEED_OR_RETURN(pp.AddCustomDefine("FALSE 0"));
    EZ_SUCCEED_OR_RETURN(pp.AddCustomDefine("PLATFORM_SHADER ="));

    for (auto& sDefine : customDefines)
    {
      EZ_SUCCEED_OR_RETURN(pp.AddCustomDefine(sDefine));
    }
  }

  pp.SetFileOpenFunction([&](ezStringView sAbsoluteFile, ezDynamicArray<ezUInt8>& out_fileContent, ezTimestamp& out_fileModification)
    {
        if (sAbsoluteFile == "SectionContent")
        {
          out_fileContent.PushBackRange(ezMakeArrayPtr((const ezUInt8*)sSectionContent.GetStartPointer(), sSectionContent.GetElementCount()));
          return EZ_SUCCESS;
        }

        ezFileReader r;
        if (r.Open(sAbsoluteFile).Failed())
        {
          ezLog::Error("Could not find include file '{0}'", sAbsoluteFile);
          return EZ_FAILURE;
        }

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
        ezFileStats stats;
        if (ezFileSystem::GetFileStats(sAbsoluteFile, stats).Succeeded())
        {
          out_fileModification = stats.m_LastModificationTime;
        }
#endif

        ezUInt8 Temp[4096];
        while (ezUInt64 uiRead = r.ReadBytes(Temp, 4096))
        {
          out_fileContent.PushBackRange(ezArrayPtr<ezUInt8>(Temp, (ezUInt32)uiRead));
        }

        return EZ_SUCCESS; });

  bool bFoundUndefinedVars = false;
  pp.m_ProcessingEvents.AddEventHandler([&bFoundUndefinedVars](const ezPreprocessor::ProcessingEvent& e)
    {
        if (e.m_Type == ezPreprocessor::ProcessingEvent::EvaluateUnknown)
        {
          bFoundUndefinedVars = true;

          ezLog::Error("Undefined variable is evaluated: '{0}' (File: '{1}', Line: {2}. Only material permutation variables are allowed in material config sections.", e.m_pToken->m_DataView, e.m_pToken->m_File, e.m_pToken->m_uiLine);
        } });

  if (pp.Process("SectionContent", out_sResult, false).Failed() || bFoundUndefinedVars)
  {
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

// static
void ezShaderParser::ParseMaterialParameterSection(ezStreamReader& inout_stream, ezDynamicArray<ParameterDefinition>& out_parameter, ezDynamicArray<EnumDefinition>& out_enumDefinitions)
{
  ezStringBuilder sContent;
  if (PreprocessSection(inout_stream, ezShaderHelper::ezShaderSections::MATERIALPARAMETER, ezArrayPtr<ezString>(), sContent).Failed())
  {
    ezLog::Error("Failed to preprocess material parameter section");
    return;
  }

  ezTokenizer tokenizer;
  tokenizer.Tokenize(ezMakeArrayPtr((const ezUInt8*)sContent.GetData(), sContent.GetElementCount()), ezLog::GetThreadLocalLogSystem());

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
void ezShaderParser::ParsePermutationSection(ezStringView s, ezDynamicArray<ezHashedString>& out_permVars, ezDynamicArray<ezPermutationVar>& out_fixedPermVars)
{
  out_permVars.Clear();
  out_fixedPermVars.Clear();

  ezTokenizer tokenizer;
  tokenizer.Tokenize(ezArrayPtr<const ezUInt8>((const ezUInt8*)s.GetStartPointer(), s.GetElementCount()), ezLog::GetThreadLocalLogSystem(), false);

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

    if (token.m_iType == ezTokenType::String1 || token.m_iType == ezTokenType::String2 || token.m_iType == ezTokenType::RawString1)
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

ezStatus ParseShaderConstant(const TokenStream& tokens, ezUInt32& ref_uiCurToken, ezShaderConstantBufferLayout& ref_materialConstantBufferLayout)
{
  ezHybridArray<ezUInt32, 8> acceptedTokens;
  TokenMatch constantPattern[] = {ezTokenType::Identifier, "("_ezsv, ezTokenType::Identifier, ")"_ezsv, ";"_ezsv};
  TokenMatch packedhalf2Pattern[] = {ezTokenType::Identifier, "("_ezsv, ezTokenType::Identifier, ","_ezsv, ezTokenType::Identifier, ","_ezsv, ezTokenType::Identifier, ")"_ezsv, ";"_ezsv};
  const ezUInt32 uiStartToken = ref_uiCurToken;
  if (Accept(tokens, ref_uiCurToken, constantPattern, &acceptedTokens))
  {
    const ezUInt32 uiNameToken = acceptedTokens[2];
    const ezUInt32 uiTypeToken = acceptedTokens[0];

    ezEnum<ezShaderConstant::Type> type;
    if (s_NameToShaderConstantTable.TryGetValue(tokens[uiTypeToken]->m_DataView, type))
    {
      ezShaderConstant& out_shaderConstant = ref_materialConstantBufferLayout.m_Constants.ExpandAndGetRef();
      out_shaderConstant.m_Type = type;
      out_shaderConstant.m_sName.Assign(tokens[uiNameToken]->m_DataView);
      out_shaderConstant.m_uiArrayElements = 1;
      out_shaderConstant.m_uiOffset = 0;
      return ezStatus(EZ_SUCCESS);
    }
    else if (tokens[uiNameToken]->m_DataView == "PACKEDCOLOR4H")
    {
      {
        ezShaderConstant& out_shaderConstant = ref_materialConstantBufferLayout.m_Constants.ExpandAndGetRef();
        out_shaderConstant.m_Type = ezShaderConstant::Type::UInt1;
        ezStringBuilder sNameRG(tokens[uiNameToken]->m_DataView, "RG"_ezsv);
        out_shaderConstant.m_sName.Assign(sNameRG.GetView());
        out_shaderConstant.m_uiArrayElements = 1;
        out_shaderConstant.m_uiOffset = 0;
      }
      {
        ezShaderConstant& out_shaderConstant = ref_materialConstantBufferLayout.m_Constants.ExpandAndGetRef();
        out_shaderConstant.m_Type = ezShaderConstant::Type::UInt1;
        ezStringBuilder sNameRG(tokens[uiNameToken]->m_DataView, "GB"_ezsv);
        out_shaderConstant.m_sName.Assign(sNameRG.GetView());
        out_shaderConstant.m_uiArrayElements = 1;
        out_shaderConstant.m_uiOffset = 0;
      }
      return ezStatus(EZ_SUCCESS);
    }

    return ezStatus(ezFmt("Unknown shader constant type: {}", tokens[uiTypeToken]->m_DataView));
  }
  else if (Accept(tokens, ref_uiCurToken, packedhalf2Pattern, &acceptedTokens))
  {
    const ezUInt32 uiNameToken = acceptedTokens[6];

    ezShaderConstant& out_shaderConstant = ref_materialConstantBufferLayout.m_Constants.ExpandAndGetRef();
    out_shaderConstant.m_Type = ezShaderConstant::Type::UInt1;
    out_shaderConstant.m_sName.Assign(tokens[uiNameToken]->m_DataView);
    out_shaderConstant.m_uiArrayElements = 1;
    out_shaderConstant.m_uiOffset = 0;
    return ezStatus(EZ_SUCCESS);
  }
  return ezStatus(ezFmt("Unknown shader constant"));
}

ezUInt32 AlignSize(ezUInt32 uiValue, ezUInt32 uiAlignment)
{
  const ezUInt32 uiRemainder = uiValue % uiAlignment;
  return uiRemainder == 0 ? uiValue : uiValue + uiAlignment - uiRemainder;
}

void AlignConstantBufferDX(ezShaderConstantBufferLayout& ref_materialConstantBufferLayout)
{
  ezUInt32 uiCurrentOffset = 0;
  for (ezShaderConstant& constant : ref_materialConstantBufferLayout.m_Constants)
  {
    const ezUInt32 uiScalarSize = s_ShaderConstantScalarSize[constant.m_Type];
    const ezUInt32 uiConstantSize = s_ShaderConstantSize[constant.m_Type];
    uiCurrentOffset = AlignSize(uiCurrentOffset, uiScalarSize);
    const ezUInt32 uiStartBucket = uiCurrentOffset / 16;
    const ezUInt32 uiEndBucket = (uiCurrentOffset + uiConstantSize - 1) / 16;
    // Check if the constant is crossing a 16 byte boundary
    if (uiStartBucket != uiEndBucket)
    {
      uiCurrentOffset = AlignSize(uiCurrentOffset, 16);
    }
    constant.m_uiOffset = uiCurrentOffset;
    uiCurrentOffset += uiConstantSize;
  }

  uiCurrentOffset = AlignSize(uiCurrentOffset, 16);
  ref_materialConstantBufferLayout.m_uiTotalSize = uiCurrentOffset;
}

void AlignStructuredBufferDX(ezShaderConstantBufferLayout& ref_materialConstantBufferLayout)
{
  ezUInt32 uiCurrentOffset = 0;
  for (ezShaderConstant& constant : ref_materialConstantBufferLayout.m_Constants)
  {
    const ezUInt32 uiScalarSize = s_ShaderConstantScalarSize[constant.m_Type];
    const ezUInt32 uiConstantSize = s_ShaderConstantSize[constant.m_Type];
    uiCurrentOffset = AlignSize(uiCurrentOffset, uiScalarSize);
    constant.m_uiOffset = uiCurrentOffset;
    uiCurrentOffset += uiConstantSize;
  }
  ref_materialConstantBufferLayout.m_uiTotalSize = uiCurrentOffset;
}

void AlignStructuredBufferStd430Relaxed(ezShaderConstantBufferLayout& ref_materialConstantBufferLayout)
{
  ezUInt32 uiCurrentOffset = 0;
  for (ezShaderConstant& constant : ref_materialConstantBufferLayout.m_Constants)
  {
    const ezUInt32 uiScalarSize = s_ShaderConstantScalarSize[constant.m_Type];
    const ezUInt32 uiConstantSize = s_ShaderConstantSize[constant.m_Type];
    uiCurrentOffset = AlignSize(uiCurrentOffset, uiScalarSize);
    const ezUInt32 uiStartBucket = uiCurrentOffset / 16;
    const ezUInt32 uiEndBucket = (uiCurrentOffset + uiConstantSize - 1) / 16;
    // Check if the constant is crossing a 16 byte boundary
    if (uiStartBucket != uiEndBucket)
    {
      uiCurrentOffset = AlignSize(uiCurrentOffset, 16);
    }
    constant.m_uiOffset = uiCurrentOffset;
    uiCurrentOffset += uiConstantSize;
  }
  ref_materialConstantBufferLayout.m_uiTotalSize = uiCurrentOffset;
}

ezStatus ParseMaterialConstants(const TokenStream& tokens, ezUInt32& ref_uiCurToken, ezShaderConstantBufferLayout& ref_materialConstantBufferLayout)
{
  while (!Accept(tokens, ref_uiCurToken, ezTokenType::EndOfFile))
  {
    ezStatus parseResult = ParseShaderConstant(tokens, ref_uiCurToken, ref_materialConstantBufferLayout);
    if (!parseResult.Succeeded())
    {
      return parseResult;
    }
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezShaderParser::ParseMaterialConstantsSection(ezStringView sMaterialConstantsSection, ezSharedPtr<ezShaderConstantBufferLayout>& out_pMaterialConstantBufferLayout)
{
  InitializeTables();

  ezTokenizer tokenizer;
  tokenizer.Tokenize(ezArrayPtr<const ezUInt8>((const ezUInt8*)sMaterialConstantsSection.GetStartPointer(), sMaterialConstantsSection.GetElementCount()), ezLog::GetThreadLocalLogSystem(), false);

  TokenStream tokens;
  tokenizer.GetAllLines(tokens);

  ezUInt32 uiCurToken = 0;
  ezHybridArray<ezUInt32, 8> acceptedTokens;

  out_pMaterialConstantBufferLayout = EZ_DEFAULT_NEW(ezShaderConstantBufferLayout);
  const ezStatus res = ParseMaterialConstants(tokens, uiCurToken, *out_pMaterialConstantBufferLayout);
  if (res.Failed() || out_pMaterialConstantBufferLayout->m_Constants.IsEmpty())
  {
    out_pMaterialConstantBufferLayout = nullptr;
    return res;
  }

  return ezStatus(EZ_SUCCESS);
}

void ezShaderParser::LayoutMaterialConstants(ezShaderConstantBufferLayout& ref_materialConstantBufferLayout, ezEnum<ezGALBufferLayout> layout)
{
  switch (layout)
  {
    case ezGALBufferLayout::Vulkan_Std430_relaxed:
      AlignStructuredBufferStd430Relaxed(ref_materialConstantBufferLayout);
      break;
    case ezGALBufferLayout::DirectX_StructuredButter:
      AlignStructuredBufferDX(ref_materialConstantBufferLayout);
      break;
    case ezGALBufferLayout::DirectX_ConstantButter:
      AlignConstantBufferDX(ref_materialConstantBufferLayout);
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
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

ezResult ParseResource(const TokenStream& tokens, ezUInt32& ref_uiCurToken, ezShaderResourceDefinition& out_resourceDefinition)
{
  // Match type
  ezUInt32 uiTypeToken = ref_uiCurToken;
  if (!Accept(tokens, ref_uiCurToken, ezTokenType::Identifier, &uiTypeToken))
  {
    return EZ_FAILURE;
  }
  if (!s_NameToDescriptorTable.TryGetValue(tokens[uiTypeToken]->m_DataView, out_resourceDefinition.m_Binding.m_ResourceType))
    return EZ_FAILURE;
  s_NameToTextureTable.TryGetValue(tokens[uiTypeToken]->m_DataView, out_resourceDefinition.m_Binding.m_TextureType);

  // Skip optional template
  TokenMatch templatePattern[] = {"<"_ezsv, ezTokenType::Identifier, ">"_ezsv};
  ezHybridArray<ezUInt32, 8> acceptedTokens;
  Accept(tokens, ref_uiCurToken, templatePattern, &acceptedTokens);

  // Match name
  ezUInt32 uiNameToken = ref_uiCurToken;
  if (!Accept(tokens, ref_uiCurToken, ezTokenType::Identifier, &uiNameToken))
  {
    return EZ_FAILURE;
  }
  out_resourceDefinition.m_Binding.m_sName.Assign(tokens[uiNameToken]->m_DataView);
  ezUInt32 uiEndToken = uiNameToken;

  // Match optional array
  TokenMatch arrayPattern[] = {"["_ezsv, ezTokenType::Integer, "]"_ezsv};
  TokenMatch bindlessPattern[] = {"["_ezsv, "]"_ezsv};
  if (Accept(tokens, ref_uiCurToken, arrayPattern, &acceptedTokens))
  {
    ezConversionUtils::StringToUInt(tokens[acceptedTokens[1]]->m_DataView, out_resourceDefinition.m_Binding.m_uiArraySize).AssertSuccess("Tokenizer error");
    uiEndToken = acceptedTokens.PeekBack();
  }
  else if (Accept(tokens, ref_uiCurToken, bindlessPattern, &acceptedTokens))
  {
    out_resourceDefinition.m_Binding.m_uiArraySize = 0;
    uiEndToken = acceptedTokens.PeekBack();
  }
  out_resourceDefinition.m_sDeclaration = ezStringView(tokens[uiTypeToken]->m_DataView.GetStartPointer(), tokens[uiEndToken]->m_DataView.GetEndPointer());

  // Match optional register
  TokenMatch slotPattern[] = {":"_ezsv, "register"_ezsv, "("_ezsv, ezTokenType::Identifier, ")"_ezsv};
  TokenMatch slotAndSetPattern[] = {":"_ezsv, "register"_ezsv, "("_ezsv, ezTokenType::Identifier, ","_ezsv, ezTokenType::Identifier, ")"_ezsv};
  if (Accept(tokens, ref_uiCurToken, slotPattern, &acceptedTokens))
  {
    ezStringView sSlot = tokens[acceptedTokens[3]]->m_DataView;
    sSlot.Trim("tsubx");
    if (sSlot.IsEqual_NoCase("AUTO")) // See shader macros in StandardMacros.h
    {
      out_resourceDefinition.m_Binding.m_iSlot = -1;
    }
    else
    {
      ezInt32 iSlot;
      ezConversionUtils::StringToInt(sSlot, iSlot).AssertSuccess("Failed to parse slot index of shader resource");
      out_resourceDefinition.m_Binding.m_iSlot = static_cast<ezInt16>(iSlot);
    }
    uiEndToken = acceptedTokens.PeekBack();
  }
  else if (Accept(tokens, ref_uiCurToken, slotAndSetPattern, &acceptedTokens))
  {
    ezStringView sSlot = tokens[acceptedTokens[3]]->m_DataView;
    sSlot.Trim("tsubx");
    if (sSlot.IsEqual_NoCase("AUTO")) // See shader macros in StandardMacros.h
    {
      out_resourceDefinition.m_Binding.m_iSlot = -1;
    }
    else
    {
      ezInt32 iSlot;
      ezConversionUtils::StringToInt(sSlot, iSlot).AssertSuccess("Failed to parse slot index of shader resource");
      out_resourceDefinition.m_Binding.m_iSlot = static_cast<ezInt16>(iSlot);
    }
    ezStringView sSet = tokens[acceptedTokens[5]]->m_DataView;
    sSet.TrimWordStart("space"_ezsv);
    ezInt32 iSet;
    ezConversionUtils::StringToInt(sSet, iSet).AssertSuccess("Failed to parse set index of shader resource");
    out_resourceDefinition.m_Binding.m_iSet = static_cast<ezInt16>(iSet);
    uiEndToken = acceptedTokens.PeekBack();
  }

  out_resourceDefinition.m_sDeclarationAndRegister = ezStringView(tokens[uiTypeToken]->m_DataView.GetStartPointer(), tokens[uiEndToken]->m_DataView.GetEndPointer());
  // Match ; (resource declaration done) or { (constant buffer member declaration starts)
  if (!Accept(tokens, ref_uiCurToken, ";"_ezsv) && !Accept(tokens, ref_uiCurToken, "{"_ezsv))
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

void ezShaderParser::ParseShaderResources(ezStringView sShaderStageSource, ezDynamicArray<ezShaderResourceDefinition>& out_resources)
{
  if (sShaderStageSource.IsEmpty())
  {
    out_resources.Clear();
    return;
  }

  InitializeTables();

  ezTokenizer tokenizer;
  tokenizer.SetTreatHashSignAsLineComment(true);
  tokenizer.Tokenize(ezArrayPtr<const ezUInt8>((const ezUInt8*)sShaderStageSource.GetStartPointer(), sShaderStageSource.GetElementCount()), ezLog::GetThreadLocalLogSystem(), false);

  TokenStream tokens;
  tokenizer.GetAllLines(tokens);

  ezUInt32 uiCurToken = 0;

  while (!Accept(tokens, uiCurToken, ezTokenType::EndOfFile))
  {
    ezShaderResourceDefinition resourceDef;
    if (ParseResource(tokens, uiCurToken, resourceDef).Succeeded())
    {
      out_resources.PushBack(std::move(resourceDef));
      continue;
    }
    ++uiCurToken;
  }
}

ezResult ezShaderParser::MergeShaderResourceBindings(const ezShaderProgramData& spd, ezHashTable<ezHashedString, ezShaderResourceBinding>& out_bindings, ezLogInterface* pLog)
{
  ezUInt32 uiSize = 0;
  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    uiSize += spd.m_Resources[stage].GetCount();
  }

  out_bindings.Clear();
  out_bindings.Reserve(uiSize);

  ezMap<ezHashedString, const ezShaderResourceDefinition*> resourceFirstOccurence;

  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    for (const ezShaderResourceDefinition& res : spd.m_Resources[stage])
    {
      ezHashedString sName = res.m_Binding.m_sName;
      auto it = out_bindings.Find(sName);
      if (it.IsValid())
      {
        ezShaderResourceBinding& current = it.Value();
        if (current.m_ResourceType != res.m_Binding.m_ResourceType || current.m_TextureType != res.m_Binding.m_TextureType || current.m_uiArraySize != res.m_Binding.m_uiArraySize)
        {
          ezLog::Error(pLog, "A shared shader resource '{}' has a mismatching signatures between stages: '{}' vs '{}'", sName, resourceFirstOccurence.Find(sName).Value()->m_sDeclarationAndRegister, res.m_sDeclarationAndRegister);
          return EZ_FAILURE;
        }

        current.m_Stages |= ezGALShaderStageFlags::MakeFromShaderStage((ezGALShaderStage::Enum)stage);
      }
      else
      {
        out_bindings.Insert(sName, res.m_Binding);
        resourceFirstOccurence.Insert(sName, &res);
        out_bindings.Find(sName).Value().m_Stages |= ezGALShaderStageFlags::MakeFromShaderStage((ezGALShaderStage::Enum)stage);
      }
    }
  }
  return EZ_SUCCESS;
}

ezResult ezShaderParser::SanityCheckShaderResourceBindings(const ezHashTable<ezHashedString, ezShaderResourceBinding>& bindings, ezLogInterface* pLog)
{
  for (auto it : bindings)
  {
    if (it.Value().m_iSet < 0)
    {
      ezLog::Error(pLog, "Shader resource '{}' does not have a set defined.", it.Key());
      return EZ_FAILURE;
    }
    if (it.Value().m_iSlot < 0)
    {
      ezLog::Error(pLog, "Shader resource '{}' does not have a slot defined.", it.Key());
      return EZ_FAILURE;
    }
  }
  return EZ_SUCCESS;
}

void ezShaderParser::ApplyShaderResourceBindings(ezStringView sPlatform, ezStringView sShaderStageSource, const ezDynamicArray<ezShaderResourceDefinition>& resources, const ezHashTable<ezHashedString, ezShaderResourceBinding>& bindings, const CreateResourceDeclaration& createDeclaration, ezStringBuilder& out_sShaderStageSource)
{
  ezDeque<ezString> partStorage;
  ezHybridArray<ezStringView, 16> parts;

  ezStringBuilder sDeclaration;
  const char* szStart = sShaderStageSource.GetStartPointer();
  for (ezUInt32 i = 0; i < resources.GetCount(); ++i)
  {
    parts.PushBack(ezStringView(szStart, resources[i].m_sDeclarationAndRegister.GetStartPointer()));

    ezShaderResourceBinding* pBinding = nullptr;
    bindings.TryGetValue(resources[i].m_Binding.m_sName, pBinding);

    EZ_ASSERT_DEV(pBinding != nullptr, "Every resource should be present in the map.");
    createDeclaration(sPlatform, resources[i].m_sDeclaration, *pBinding, sDeclaration);
    ezString& sStorage = partStorage.ExpandAndGetRef();
    sStorage = sDeclaration;
    parts.PushBack(sStorage);
    szStart = resources[i].m_sDeclarationAndRegister.GetEndPointer();
  }
  parts.PushBack(ezStringView(szStart, sShaderStageSource.GetEndPointer()));

  ezUInt32 uiSize = 0;
  for (const ezStringView& sPart : parts)
    uiSize += sPart.GetElementCount();

  out_sShaderStageSource.Clear();
  out_sShaderStageSource.Reserve(uiSize);

  for (const ezStringView& sPart : parts)
  {
    out_sShaderStageSource.Append(sPart);
  }
}
