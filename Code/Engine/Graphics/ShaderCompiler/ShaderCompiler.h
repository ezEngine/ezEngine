#pragma once

#include <Graphics/Basics.h>
#include <Graphics/Shader/Helper.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Set.h>
#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <CoreUtils/CodeUtils/Preprocessor.h>
#include <Graphics/ShaderCompiler/PermutationGenerator.h>
#include <Graphics/Shader/ShaderPermutationBinary.h>

class ezShaderProgramCompiler;
EZ_DECLARE_REFLECTABLE_TYPE(EZ_GRAPHICS_DLL, ezShaderProgramCompiler);

class EZ_GRAPHICS_DLL ezShaderProgramCompiler : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderProgramCompiler);

public:

  struct ezShaderProgramData
  {
    ezShaderProgramData()
    {
      m_szPlatform = nullptr;

      for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
        m_szShaderSource[stage] = nullptr;
    }

    const char* m_szPlatform;
    const char* m_szShaderSource[ezGALShaderStage::ENUM_COUNT];
    ezShaderStageBinary m_StageBinary[ezGALShaderStage::ENUM_COUNT];
  };

  virtual void GetSupportedPlatforms(ezHybridArray<ezString, 4>& Platforms) = 0;

  virtual ezResult Compile(ezShaderProgramData& inout_Data, ezLogInterface* pLog) = 0;

};

class EZ_GRAPHICS_DLL ezShaderCompiler
{
public:

  ezResult CompileShader(const char* szFile, const ezPermutationGenerator& Generator, const char* szPlatform = "ALL");

private:

  ezResult CompileShader(const char* szFile, const ezPermutationGenerator& Generator, const char* szPlatform, ezShaderProgramCompiler* pCompiler);

  struct ezShaderData
  {
    ezString m_Platforms;
    ezString m_Permutations;
    ezString m_ShaderStageSource[ezGALShaderStage::ENUM_COUNT];
  };

  ezResult FileOpen(const char* szAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent);

  ezStringBuilder m_StageSourceFile[ezGALShaderStage::ENUM_COUNT];

  ezDeque<ezPermutationGenerator::PermutationVar> m_PermVars;
  ezTokenizedFileCache m_FileCache;
  ezShaderData m_ShaderData;
};


