#include "Main.h"
#include <Foundation/Logging/Log.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/Lock.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <GameUtils/Components/RotorComponent.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameUtils/Components/TimedDeathComponent.h>
#include <GameUtils/Components/SpawnComponent.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <RendererCore/Components/CameraComponent.h>
#include <GameUtils/Components/SliderComponent.h>
#include <GameUtils/Components/InputComponent.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>

ezShaderCompilerApplication::ezShaderCompilerApplication()
  : ezGameApplication("ezShaderCompiler", ezGameApplicationType::StandAlone, nullptr)
{
}

void ezShaderCompilerApplication::BeforeCoreStartup()
{
  // only print important messages
  ezGlobalLog::GetOrCreateInstance()->SetLogLevel(ezLogMsgType::InfoMsg);

  ezGameApplication::BeforeCoreStartup();

  m_sShaderFiles = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-shader", 0, "");
  EZ_ASSERT_ALWAYS(!m_sShaderFiles.IsEmpty(), "Shader file has not been specified. Use the -shader command followed by a path");

  m_sAppProjectPath = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-project", 0, "");
  EZ_ASSERT_ALWAYS(!m_sAppProjectPath.IsEmpty(), "Project directory has not been specified. Use the -project command followed by a path");

  m_sPlatforms = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-platform", 0, "");

  if (m_sPlatforms.IsEmpty())
    m_sPlatforms = "DX11_SM50";// "ALL";
}


void ezShaderCompilerApplication::AfterCoreStartup()
{
  DoProjectSetup();

  ezStartup::StartupEngine();
}

void ezShaderCompilerApplication::BeforeCoreShutdown()
{
  ezGameApplication::BeforeCoreShutdown();
}

void ezShaderCompilerApplication::DoLoadCustomPlugins()
{
  EZ_VERIFY(ezPlugin::LoadPlugin("ezShaderCompilerHLSL").Succeeded(), "Compiler Plugin not found");
}

ezResult ezShaderCompilerApplication::CompileShader(const char* szShaderFile)
{
  EZ_LOG_BLOCK("Compiling Shader", szShaderFile);

  if (ExtractPermutationVarValues(szShaderFile).Failed())
    return EZ_FAILURE;

  ezHybridArray<ezPermutationVar, 16> PermVars;

  const ezUInt32 uiMaxPerms = m_PermutationGenerator.GetPermutationCount();

  ezLog::Info("Shader has {0} permutations", uiMaxPerms);

  for (ezUInt32 perm = 0; perm < uiMaxPerms; ++perm)
  {
    EZ_LOG_BLOCK("Compiling Permutation");

    m_PermutationGenerator.GetPermutation(perm, PermVars);
    ezShaderCompiler sc;
    if (sc.CompileShaderPermutationForPlatforms(szShaderFile, PermVars, m_sPlatforms).Failed())
    {
      ezLog::Error("Failed to compile shader permutation {0}", perm);
      return EZ_FAILURE;
    }
  }

  ezLog::Success("Compiled Shader '{0}'", szShaderFile);
  return EZ_SUCCESS;
}

ezResult ezShaderCompilerApplication::ExtractPermutationVarValues(const char* szShaderFile)
{
  m_PermVarValues.Clear();
  m_PermutationGenerator.Clear();

  ezFileReader shaderFile;
  if (shaderFile.Open(szShaderFile).Failed())
  {
    ezLog::Error("Could not open file '{0}'", szShaderFile);
    return EZ_FAILURE;
  }

  ezHybridArray<ezHashedString, 16> permVars;
  ezShaderParser::ParsePermutationSection(shaderFile, permVars);

  {
    EZ_LOG_BLOCK("Permutation Vars");
    for (const auto& s : permVars)
    {
      ezLog::Dev(s.GetData());
    }
  }

  {
    EZ_LOG_BLOCK("Permutation Var Values");
    for (const auto& s : permVars)
    {
      ezHybridArray<ezHashedString, 4> values;
      ezShaderManager::GetPermutationValues(s, values);

      for (const auto& val : values)
      {
        m_PermVarValues[s.GetData()].PushBack(val.GetData());

        ezLog::Info("PV: {0} = {1}", s.GetData(), val.GetData());

        m_PermutationGenerator.AddPermutation(s, val);
      }
    }
  }

  return EZ_SUCCESS;
}

void ezShaderCompilerApplication::PrintConfig()
{
  EZ_LOG_BLOCK("ShaderCompiler Config");

  ezLog::Info("Project: '{0}'", m_sAppProjectPath);
  ezLog::Info("Shader: '{0}'", m_sShaderFiles);
  ezLog::Info("Platform: '{0}'", m_sPlatforms);
}

ezApplication::ApplicationExecution ezShaderCompilerApplication::Run()
{
  PrintConfig();

  ezStringBuilder files = m_sShaderFiles;

  ezDynamicArray<ezStringView> allFiles;
  files.Split(false, allFiles, ";");

  for (const ezStringView& shader : allFiles)
  {
    ezStringBuilder file = shader;
    ezStringBuilder relPath;

    if (ezFileSystem::ResolvePath(file, nullptr, &relPath).Failed())
    {
      ezLog::Error("Could not resolve path to shader '{0}'", file);
    }

    if (CompileShader(relPath).Failed())
    {
      ezLog::Error("Failed to compile shader '{0}'", relPath);
      return ezApplication::ApplicationExecution::Quit;
    }
  }

  return ezApplication::ApplicationExecution::Quit;
}

EZ_APPLICATION_ENTRY_POINT(ezShaderCompilerApplication);



