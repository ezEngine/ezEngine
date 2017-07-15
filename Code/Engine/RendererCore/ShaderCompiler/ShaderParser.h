#pragma once

#include <RendererCore/Declarations.h>
#include <Foundation/Strings/String.h>

class ezPropertyAttribute;

class EZ_RENDERERCORE_DLL ezShaderParser
{
public:

  struct AttributeDefinition
  {
    ezString m_sName;
    ezString m_sValue;
  };

  struct ParameterDefinition
  {
    ezString m_sType;
    ezString m_sName;

    ezHybridArray<AttributeDefinition, 4> m_Attributes;
  };

  static void ParseMaterialParameterSection(ezStreamReader& stream, ezHybridArray<ParameterDefinition, 16>& out_Parameter);

  static void ParsePermutationSection(ezStreamReader& stream, ezHybridArray<ezHashedString, 16>& out_PermVars, ezHybridArray<ezPermutationVar, 16>& out_FixedPermVars);
  static void ParsePermutationSection(ezStringView sPermutationSection, ezHybridArray<ezHashedString, 16>& out_PermVars, ezHybridArray<ezPermutationVar, 16>& out_FixedPermVars);

  static void ParsePermutationVarConfig(const char* szPermVarName, ezStringView sPermutationVarConfig, ezVariant& out_DefaultValue, ezHybridArray<ezHashedString, 16>& out_EnumValues);
};
