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
  ezLog::SetLogLevel(ezLogMsgType::InfoMsg);

  ezGameApplication::BeforeCoreStartup();

  auto cmd = ezCommandLineUtils::GetGlobalInstance();

  m_sShaderFiles = cmd->GetStringOption("-shader", 0, "");
  EZ_ASSERT_ALWAYS(!m_sShaderFiles.IsEmpty(), "Shader file has not been specified. Use the -shader command followed by a path");

  m_sAppProjectPath = cmd->GetStringOption("-project", 0, "");
  EZ_ASSERT_ALWAYS(!m_sAppProjectPath.IsEmpty(), "Project directory has not been specified. Use the -project command followed by a path");

  m_sPlatforms = cmd->GetStringOption("-platform", 0, "");

  if (m_sPlatforms.IsEmpty())
    m_sPlatforms = "DX11_SM50";// "ALL";

  const ezUInt32 pvs = cmd->GetStringOptionArguments("-perm");

  for (ezUInt32 pv = 0; pv < pvs; ++pv)
  {
    ezStringBuilder var = cmd->GetStringOption("-perm", pv);

    const char* szEqual = var.FindSubString("=");

    if (szEqual == nullptr)
    {
      ezLog::Error("Permutation Variable declaration contains no equal sign: '{0}'", var);
      continue;
    }

    ezStringBuilder val = szEqual + 1;
    var.SetSubString_FromTo(var.GetData(), szEqual);

    val.Trim(" \t");
    var.Trim(" \t");

    ezLog::Dev("Fixed permutation variable: {0} = {1}", var, val);
    m_FixedPermVars[var].PushBack(val);
  }
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
    if (sc.CompileShaderPermutationForPlatforms(szShaderFile, PermVars, ezGlobalLog::GetOrCreateInstance(), m_sPlatforms).Failed())
      return EZ_FAILURE;
  }

  ezLog::Success("Compiled Shader '{0}'", szShaderFile);
  return EZ_SUCCESS;
}

ezResult ezShaderCompilerApplication::ExtractPermutationVarValues(const char* szShaderFile)
{
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
    for (const auto& s : permVars)
    {
      ezHybridArray<ezHashedString, 4> values;
      ezShaderManager::GetPermutationValues(s, values);

      for (const auto& val : values)
      {
        m_PermutationGenerator.AddPermutation(s, val);
      }
    }
  }

  {
    for (auto it = m_FixedPermVars.GetIterator(); it.IsValid(); ++it)
    {
      ezHashedString hsname, hsvalue;
      hsname.Assign(it.Key().GetData());
      m_PermutationGenerator.RemovePermutations(hsname);

      for (const auto& val : it.Value())
      {
        hsvalue.Assign(val.GetData());

        m_PermutationGenerator.AddPermutation(hsname, hsvalue);
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
      return ezApplication::ApplicationExecution::Quit;
  }

  return ezApplication::ApplicationExecution::Quit;
}

EZ_APPLICATION_ENTRY_POINT(ezShaderCompilerApplication);



