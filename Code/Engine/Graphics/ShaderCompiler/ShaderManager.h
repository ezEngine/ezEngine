#pragma once

#include <Graphics/Basics.h>
#include <Graphics/ShaderCompiler/PermutationGenerator.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Map.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Device/Device.h>

class EZ_GRAPHICS_DLL ezShaderManager
{
public:

  static void SetPlatform(const char* szPlatform, ezGALDevice* pDevice);

  static void SetPermutationVariable(const char* szVariable, const char* szValue);

  static void BindShader(const char* szShaderFile);

  static bool IsShaderValid() { return s_bShaderValid; }

private:

  struct ShaderConfig
  {
    ezString m_PermutationVarsUsed;
    bool m_bShaderValid;
  };

  struct ShaderProgramData
  {
    ezGALShaderHandle m_hShader;
    ezGALVertexDeclarationHandle m_hVertexDeclaration;
  };

  static ezMap<ezString, ShaderConfig> s_ShaderConfigs;

  static ezPermutationGenerator s_AllowedPermutations;
  static ezMap<ezString, ezString> s_PermutationVariables;
  static ezString s_sPlatform;
  static bool s_bShaderValid;
  static ezMap<ezString, ShaderProgramData> s_ShaderPrograms;
  static ezGALDevice* s_pDevice;
};


