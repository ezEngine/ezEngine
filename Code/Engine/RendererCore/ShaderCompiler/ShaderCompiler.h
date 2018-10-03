#pragma once

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

// \brief Flags that affect the compilation process of a shader
struct ezShaderCompilerFlags
{
  typedef ezUInt8 StorageType;
  enum Enum
  {
    Debug = EZ_BIT(0),
    Default = 0,
  };

  struct Bits
  {
    StorageType Debug : 1;
  };
};
EZ_DECLARE_FLAGS_OPERATORS(ezShaderCompilerFlags);

class EZ_RENDERERCORE_DLL ezShaderProgramCompiler : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderProgramCompiler, ezReflectedClass);

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

    ezBitflags<ezShaderCompilerFlags> m_Flags;
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
  ezResult CompileShaderPermutationForPlatforms(const char* szFile, const ezArrayPtr<const ezPermutationVar>& permutationVars,
                                                ezLogInterface* pLog, const char* szPlatform = "ALL");

private:
  ezResult RunShaderCompiler(const char* szFile, const char* szPlatform, ezShaderProgramCompiler* pCompiler, ezLogInterface* pLog);

  void WriteFailedShaderSource(ezShaderProgramCompiler::ezShaderProgramData& spd, ezLogInterface* pLog);

  bool PassThroughUnknownCommandCB(const char* szCmd) { return ezStringUtils::IsEqual(szCmd, "version"); }

  struct ezShaderData
  {
    ezString m_Platforms;
    ezHybridArray<ezPermutationVar, 16> m_Permutations;
    ezHybridArray<ezPermutationVar, 16> m_FixedPermVars;
    ezString m_StateSource;
    ezString m_ShaderStageSource[ezGALShaderStage::ENUM_COUNT];
  };

  ezResult FileOpen(const char* szAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent, ezTimestamp& out_FileModification);

  ezStringBuilder m_StageSourceFile[ezGALShaderStage::ENUM_COUNT];

  ezTokenizedFileCache m_FileCache;
  ezShaderData m_ShaderData;

  ezSet<ezString> m_IncludeFiles;
};
