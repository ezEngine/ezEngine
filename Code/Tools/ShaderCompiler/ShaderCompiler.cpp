#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <ShaderCompiler/ShaderCompiler.h>

ezCommandLineOptionString opt_Shader("_ShaderCompiler", "-shader", "\
One or multiple paths to shader files or folders containing shaders.\n\
Paths are separated with semicolons.\n\
Paths may be absolute or relative to the -project directory.\n\
If a path to a folder is specified, all .ezShader files in that folder are compiled.\n\
\n\
This option has to be specified.",
  "");

ezCommandLineOptionPath opt_Project("_ShaderCompiler", "-project", "\
Path to the folder of the project, for which shaders should be compiled.",
  "");

ezCommandLineOptionString opt_Platform("_ShaderCompiler", "-platform", "The name of the platform for which to compile the shaders.\n\
Examples:\n\
  -platform DX11_SM50\n\
  -platform VULKAN\n\
  -platform ALL",
  "DX11_SM50");

ezCommandLineOptionBool opt_IgnoreErrors("_ShaderCompiler", "-IgnoreErrors", "If set, a compile error won't stop other shaders from being compiled.", false);

ezCommandLineOptionDoc opt_Perm("_ShaderCompiler", "-perm", "<string list>", "List of permutation variables to set to fixed values.\n\
Spaces are used to separate multiple arguments, therefore each argument mustn't use spaces.\n\
In the form of 'SOME_VAR=VALUE'\n\
Examples:\n\
  -perm BLEND_MODE=BLEND_MODE_OPAQUE\n\
  -perm TWO_SIDED=FALSE MSAA=TRUE\n\
\n\
If a permutation variable is not set to a fixed value, all shader permutations for that variable will generated and compiled.\n\
",
  "");

ezShaderCompilerApplication::ezShaderCompilerApplication()
  : ezGameApplication("ezShaderCompiler", nullptr)
{
}

ezResult ezShaderCompilerApplication::BeforeCoreSystemsStartup()
{
  {
    ezStringBuilder cmdHelp;
    if (ezCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, ezCommandLineOption::LogAvailableModes::IfHelpRequested, "_ShaderCompiler"))
    {
      ezLog::Print(cmdHelp);
      return EZ_FAILURE;
    }
  }

  ezStartup::AddApplicationTag("tool");
  ezStartup::AddApplicationTag("shadercompiler");

  // only print important messages
  ezLog::SetDefaultLogLevel(ezLogMsgType::InfoMsg);

  EZ_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  auto cmd = ezCommandLineUtils::GetGlobalInstance();

  m_sShaderFiles = opt_Shader.GetOptionValue(ezCommandLineOption::LogMode::Always);
  EZ_ASSERT_ALWAYS(!m_sShaderFiles.IsEmpty(), "Shader file has not been specified. Use the -shader command followed by a path");

  m_sAppProjectPath = opt_Project.GetOptionValue(ezCommandLineOption::LogMode::Always);
  EZ_ASSERT_ALWAYS(!m_sAppProjectPath.IsEmpty(), "Project directory has not been specified. Use the -project command followed by a path");

  m_sPlatforms = opt_Platform.GetOptionValue(ezCommandLineOption::LogMode::Always);

  m_bIgnoreErrors = opt_IgnoreErrors.GetOptionValue(ezCommandLineOption::LogMode::Always);

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

  return EZ_SUCCESS;
}


void ezShaderCompilerApplication::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  ezStartup::StartupHighLevelSystems();
}

void ezShaderCompilerApplication::Init_LoadRequiredPlugins()
{
  ezPlugin::InitializeStaticallyLinkedPlugins();

#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
  ezShaderManager::Configure("VULKAN", true);
  EZ_VERIFY(ezPlugin::LoadPlugin("ezShaderCompilerDXC").Succeeded(), "DXC compiler plugin not found");
#else
  ezShaderManager::Configure("DX11_SM50", true);
  EZ_VERIFY(ezPlugin::LoadPlugin("ezShaderCompilerHLSL").Succeeded(), "HLSL compiler plugin not found");
#endif
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
    if (sc.CompileShaderPermutationForPlatforms(szShaderFile, PermVars, ezLog::GetThreadLocalLogSystem(), m_sPlatforms).Failed())
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
  ezHybridArray<ezPermutationVar, 16> fixedPermVars;
  ezShaderParser::ParsePermutationSection(shaderFile, permVars, fixedPermVars);

  {
    EZ_LOG_BLOCK("Permutation Vars");
    for (const auto& s : permVars)
    {
      ezLog::Dev(s.GetData());
    }
  }

  // regular permutation variables
  {
    for (const auto& s : permVars)
    {
      ezHybridArray<ezHashedString, 16> values;
      ezShaderManager::GetPermutationValues(s, values);

      for (const auto& val : values)
      {
        m_PermutationGenerator.AddPermutation(s, val);
      }
    }
  }

  // permutation variables that have fixed values
  {
    for (const auto& s : fixedPermVars)
    {
      m_PermutationGenerator.AddPermutation(s.m_sName, s.m_sValue);
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

ezApplication::Execution ezShaderCompilerApplication::Run()
{
  PrintConfig();

  ezStringBuilder files = m_sShaderFiles;

  ezDynamicArray<ezString> shadersToCompile;

  ezDynamicArray<ezStringView> allFiles;
  files.Split(false, allFiles, ";");

  for (const ezStringView& shader : allFiles)
  {
    ezStringBuilder file = shader;
    ezStringBuilder relPath;

    if (ezFileSystem::ResolvePath(file, nullptr, &relPath).Succeeded())
    {
      shadersToCompile.PushBack(relPath);
    }
    else
    {
      if (ezPathUtils::IsRelativePath(file))
      {
        file.Prepend(m_sAppProjectPath, "/");
      }

      file.TrimWordEnd("*");
      file.MakeCleanPath();

      if (ezOSFile::ExistsDirectory(file))
      {
        ezFileSystemIterator fsIt;
        for (fsIt.StartSearch(file, ezFileSystemIteratorFlags::ReportFilesRecursive); fsIt.IsValid(); fsIt.Next())
        {
          if (ezPathUtils::HasExtension(fsIt.GetStats().m_sName, "ezShader"))
          {
            fsIt.GetStats().GetFullPath(relPath);

            if (relPath.MakeRelativeTo(m_sAppProjectPath).Succeeded())
            {
              shadersToCompile.PushBack(relPath);
            }
          }
        }
      }
      else
      {
        ezLog::Error("Could not resolve path to shader '{0}'", file);
      }
    }
  }

  for (const auto& shader : shadersToCompile)
  {
    if (CompileShader(shader).Failed())
    {
      if (!m_bIgnoreErrors)
      {
        return ezApplication::Execution::Quit;
      }
    }
  }

  return ezApplication::Execution::Quit;
}

EZ_CONSOLEAPP_ENTRY_POINT(ezShaderCompilerApplication);
