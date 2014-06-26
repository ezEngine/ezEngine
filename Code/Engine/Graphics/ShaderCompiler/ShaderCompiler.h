#pragma once

#include <Graphics/Basics.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Set.h>
#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <CoreUtils/CodeUtils/Preprocessor.h>
#include <Graphics/ShaderCompiler/PermutationGenerator.h>
#include <Graphics/Shader/ShaderPermutationBinary.h>

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

class EZ_GRAPHICS_DLL ezShaderProgramCompiler : public ezEnumerable<ezShaderProgramCompiler>
{
  EZ_DECLARE_ENUMERABLE_CLASS(ezShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(ezHybridArray<ezString, 4>& Platforms) = 0;

  virtual ezResult Compile(ezShaderProgramData& inout_Data, ezLogInterface* pLog) = 0;

};

struct ezShaderData
{
  ezString m_Platforms;
  ezString m_Permutations;
  ezString m_ShaderStageSource[ezGALShaderStage::ENUM_COUNT];
};

class EZ_GRAPHICS_DLL ezShaderCompiler
{
public:

  ezResult CompileShader(const char* szFile, const ezPermutationGenerator& Generator, const char* szPlatform = "ALL");

  struct ShaderSection
  {
    ShaderSection(const char* szName)
    {
      m_sName = szName;
      m_szSectionStart = nullptr;
    }

    ezString m_sName;
    const char* m_szSectionStart;
    ezStringIterator m_Content;
  };

  enum ShaderSections
  {
    PLATFORMS,
    PERMUTATIONS,
    VERTEXSHADER,
    HULLSHADER,
    DOMAINSHADER,
    GEOMETRYSHADER,
    PIXELSHADER,
    COMPUTESHADER,
  };

  static void GetShaderSections(const ezStringBuilder& sContent, ezHybridArray<ShaderSection, 16>& out_Sections);

private:
  static void SplitIntoSections(const ezStringBuilder& sContent, ezHybridArray<ShaderSection, 16>& inout_Sections);

  ezResult FileOpen(const char* szAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent);

  ezStringBuilder m_StageSourceFile[ezGALShaderStage::ENUM_COUNT];

  ezShaderData m_ShaderData;
};


