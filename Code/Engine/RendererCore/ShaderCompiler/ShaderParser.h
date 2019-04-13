#pragma once

#include <Foundation/Strings/String.h>
#include <RendererCore/Declarations.h>

class ezPropertyAttribute;

class EZ_RENDERERCORE_DLL ezShaderParser
{
public:
  struct AttributeDefinition
  {
    ezString m_sType;
    ezHybridArray<ezVariant, 8> m_Values;
  };

  struct ParameterDefinition
  {
    const ezRTTI* m_pType = nullptr;
    ezString m_sType;
    ezString m_sName;

    ezHybridArray<AttributeDefinition, 4> m_Attributes;
  };

  struct EnumDefinition
  {
    ezString m_sName;
    ezVariant m_DefaultValue;
    ezHybridArray<ezHashedString, 16> m_Values;
  };

  static void ParseMaterialParameterSection(
    ezStreamReader& stream, ezHybridArray<ParameterDefinition, 16>& out_Parameter, ezHybridArray<EnumDefinition, 4>& out_EnumDefinitions);

  static void ParsePermutationSection(
    ezStreamReader& stream, ezHybridArray<ezHashedString, 16>& out_PermVars, ezHybridArray<ezPermutationVar, 16>& out_FixedPermVars);
  static void ParsePermutationSection(ezStringView sPermutationSection, ezHybridArray<ezHashedString, 16>& out_PermVars,
    ezHybridArray<ezPermutationVar, 16>& out_FixedPermVars);

  static void ParsePermutationVarConfig(
    ezStringView sPermutationVarConfig, ezVariant& out_DefaultValue, EnumDefinition& out_EnumDefinition);
};
