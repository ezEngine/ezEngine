#pragma once

#include <RendererCore/Basics.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Set.h>
#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <CoreUtils/CodeUtils/Preprocessor.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>

class EZ_RENDERERCORE_DLL ezShaderProgramCompiler : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderProgramCompiler);

public:

  struct ezShaderProgramData
  {
    ezShaderProgramData()
    {
      m_szPlatform = nullptr;
      m_szSourceFile = nullptr;

      for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
      {
        m_bWriteToDisk[stage] = true;
        m_szShaderSource[stage] = nullptr;
      }
    }

    const char* m_szPlatform;
    const char* m_szSourceFile;
    const char* m_szShaderSource[ezGALShaderStage::ENUM_COUNT];
    ezShaderStageBinary m_StageBinary[ezGALShaderStage::ENUM_COUNT];
    bool m_bWriteToDisk[ezGALShaderStage::ENUM_COUNT];
  };

  virtual void GetSupportedPlatforms(ezHybridArray<ezString, 4>& Platforms) = 0;

  virtual ezResult Compile(ezShaderProgramData& inout_Data, ezLogInterface* pLog) = 0;

};

class EZ_RENDERERCORE_DLL ezShaderCompiler
{
public:

  ezResult CompileShaderPermutationsForPlatforms(const char* szFile, const ezPermutationGenerator& Generator, const char* szPlatform = "ALL");

private:

  void RunShaderCompilerForPermutations(const char* szFile, const ezPermutationGenerator& Generator, const char* szPlatform, ezShaderProgramCompiler* pCompiler);

  bool PassThroughUnknownCommandCB(const char* szCmd)
  {
    return ezStringUtils::IsEqual(szCmd, "version");
  }

  struct ezShaderData
  {
    ezString m_Platforms;
    ezString m_Permutations;
    ezString m_StateSource;
    ezString m_ShaderStageSource[ezGALShaderStage::ENUM_COUNT];
  };

  ezResult FileOpen(const char* szAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent, ezTimestamp& out_FileModification);

  ezStringBuilder m_StageSourceFile[ezGALShaderStage::ENUM_COUNT];

  ezHybridArray<ezPermutationGenerator::PermutationVar, 16> m_PermVars;
  ezTokenizedFileCache m_FileCache;
  ezShaderData m_ShaderData;

  ezSet<ezString> m_IncludeFiles;
};


