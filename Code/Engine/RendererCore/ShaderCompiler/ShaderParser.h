#pragma once

#include <RendererCore/Declarations.h>
#include <Foundation/Strings/String.h>

class ezPropertyAttribute;

class EZ_RENDERERCORE_DLL ezShaderParser
{
public:

  struct ParameterDefinition
  {
    const ezRTTI* m_pType;
    ezString m_sType;
    ezString m_sName;

    ezHybridArray<ezPropertyAttribute*, 4> m_Attributes;
  };

  static void ParseMaterialParameterSection(ezStreamReader& stream, ezHybridArray<ParameterDefinition, 16>& out_Parameter);

  static void ParsePermutationSection(ezStreamReader& stream, ezHybridArray<ezHashedString, 16>& out_PermVars);
  static void ParsePermutationSection(ezStringView sPermutationSection, ezHybridArray<ezHashedString, 16>& out_PermVars);

  static void ParsePermutationVarConfig(ezStringView sPermutationVarConfig, ezVariant& out_DefaultValue, ezHybridArray<ezHashedString, 16>& out_EnumValues);
};
