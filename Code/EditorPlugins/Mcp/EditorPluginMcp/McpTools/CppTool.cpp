#include <EditorPluginMcp/EditorPluginMcpPCH.h>

#include <EditorPluginMcp/McpTools/CppTool.h>
#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpCppTool, 1, ezRTTIDefaultAllocator<ezMcpCppTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezMcpCppTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  ezMcpToolDesc& status = out_tools.ExpandAndGetRef();
  status.m_sName = "cpp_status";
  status.m_sDescription = "Returns the state of the project's C++ plugin: whether the project has C++ code at all, the plugin "
                          "name, where its sources, its build directory, its solution and its compiled library are, whether a "
                          "build is needed, and whether the configured compiler can be used. Call this before cpp_generate or "
                          "cpp_build - it is the only cheap call of the three and it says which of them is the one to make. A "
                          "project without C++ code reports 'hasCppProject' false; cpp_generate creates it.";
  status.m_sInputSchema = R"({"type":"object","properties":{}})";

  ezMcpToolDesc& generate = out_tools.ExpandAndGetRef();
  generate.m_sName = "cpp_generate";
  generate.m_sDescription = "Creates or regenerates the C++ plugin of the open project: writes the default sources into the "
                            "project directory if they are not there yet, updates the generated files that belong to the SDK, "
                            "runs CMake and (unless turned off) compiles the result. This is what the editor's 'C++ Project' "
                            "dialog does, which cannot be used through action_execute.\n"
                            "Existing source files are never overwritten - only files that are missing are written, plus "
                            "SDK-owned files such as CMakeLists.txt when the SDK has a newer version of them. So calling this "
                            "on a project that already has C++ code is safe and is the way to regenerate a broken solution.\n"
                            "The plugin name can only be chosen when the project has no C++ settings yet; renaming later is "
                            "refused, because the existing sources and CMake files carry the old name and are not touched.\n"
                            "This runs for minutes - CMake generation plus a full build of the plugin - during which the editor "
                            "answers nothing else. Use a generous timeout and see app_ping for telling a slow build apart from "
                            "a hung editor. The full log is returned; on failure it is the only place the compiler errors are.";
  generate.m_sInputSchema = R"({"type":"object","properties":{)"
                            R"("pluginName":{"type":"string","description":"Name of the plugin to create, without the 'Plugin' suffix, which is added automatically. Only used when the project has no C++ settings yet, otherwise it has to match the existing name. Defaults to the project name."},)"
                            R"("compile":{"type":"boolean","description":"Compile the plugin after generating the solution. Default true. With this off the solution exists but no library is produced, so the editor still cannot load the plugin."},)"
                            R"("cleanBuildDirectory":{"type":"boolean","description":"Delete the build directory before running CMake. Default false. Turns an incremental build into a full one - use it when CMake or the build fails in ways that look stale. Fails harmlessly if the solution is open in an IDE."},)"
                            R"("useSdkCompiler":{"type":"boolean","description":"Switch the editor's compiler preference to the one this SDK was built with before generating. Default false. Set it when cpp_status reports a compiler mismatch, which otherwise makes CMake refuse to run - a plugin built with a different compiler cannot be loaded."}})"
                            R"(})";

  ezMcpToolDesc& build = out_tools.ExpandAndGetRef();
  build.m_sName = "cpp_build";
  build.m_sDescription = "Compiles the C++ plugin of the open project and restarts the engine process so the editor picks up the "
                         "new library. Call this after editing the project's C++ sources. Requires that the C++ project already "
                         "exists - see cpp_status, and cpp_generate to create it.\n"
                         "By default this compiles whenever anything might have changed, which for unchanged sources is a cheap "
                         "no-op inside the build system; 'force' additionally re-runs CMake first, which is what to use after "
                         "adding, removing or renaming source files.\n"
                         "This runs for minutes, during which the editor answers nothing else. Use a generous timeout and see "
                         "app_ping for telling a slow build apart from a hung editor. The full log is returned; on failure it "
                         "is the only place the compiler errors are.";
  build.m_sInputSchema = R"({"type":"object","properties":{)"
                         R"("force":{"type":"boolean","description":"Re-run CMake before compiling, instead of only when the build files are missing or out of date. Default false. Needed after files were added to or removed from the source directory, because CMake collects them at generation time."}})"
                         R"(})";
}

void ezMcpCppTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  if (sToolName == "cpp_status")
  {
    ExecuteStatus(arguments, out_result);
  }
  else if (sToolName == "cpp_generate")
  {
    ExecuteGenerate(arguments, out_result);
  }
  else if (sToolName == "cpp_build")
  {
    ExecuteBuild(arguments, out_result);
  }
}

bool ezMcpCppTool::PrepareSettings(ezCppSettings& out_settings, ezMcpToolResult& out_result)
{
  if (!ezToolsProject::IsProjectOpen())
  {
    out_result.SetError("No project is open, so there is no C++ plugin to work with.");
    return false;
  }

  // A project that never had C++ code has no CppProject.ddl, so a failed load is the normal state here
  // and not an error: it leaves the settings at their defaults, which is what creating one starts from.
  out_settings.Load().IgnoreResult();
  return true;
}

ezString ezMcpCppTool::GetEffectivePluginName(const ezCppSettings& settings)
{
  if (!settings.m_sPluginName.IsEmpty())
    return settings.m_sPluginName;

  // Same fallback as the C++ project dialog, where the project name is the placeholder text.
  return ezToolsProject::GetSingleton()->GetProjectName(true);
}

ezString ezMcpCppTool::GetPluginBinaryPath(const ezCppSettings& settings)
{
  ezStringBuilder sPath = ezOSFile::GetApplicationDirectory();
  sPath.AppendPath(GetEffectivePluginName(settings));
  sPath.Append("Plugin");

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  sPath.Append(".dll");
#else
  sPath.Append(".so");
#endif

  sPath.MakeCleanPath();
  return sPath;
}

void ezMcpCppTool::ExecuteStatus(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezCppSettings cfg;
  if (!PrepareSettings(cfg, out_result))
    return;

  const bool bHasCppProject = ezCppProject::ExistsProjectCMakeListsTxt();
  const ezStatus compilerStatus = ezCppProject::TestCompiler();
  const ezString sPluginBinary = GetPluginBinaryPath(cfg);

  ezMcpJsonWriter writer;
  writer.BeginObject();

  // Whether the CMakeLists.txt exists is the distinction that decides which tool to call next: without
  // it every other operation does nothing, quietly, since ezCppProject treats "no C++ code" as success.
  writer.AddVariableBool("hasCppProject", bHasCppProject);
  writer.AddVariableBool("pluginNameConfigured", !cfg.m_sPluginName.IsEmpty());
  writer.AddVariableString("pluginName", GetEffectivePluginName(cfg));

  writer.AddVariableString("sourceDirectory", ezCppProject::GetTargetSourceDir());
  writer.AddVariableString("pluginSourceDirectory", ezCppProject::GetPluginSourceDir(cfg));
  writer.AddVariableString("buildDirectory", ezCppProject::GetBuildDir(cfg));
  writer.AddVariableString("solutionPath", ezCppProject::GetSolutionPath(cfg));
  writer.AddVariableBool("solutionExists", ezCppProject::ExistsSolution(cfg));

  // The library the editor actually loads. Reported because a successful build and a loadable plugin
  // are not the same thing: a mismatched name or a build into a different directory shows up here.
  writer.AddVariableString("pluginBinary", sPluginBinary);
  writer.AddVariableBool("pluginBinaryExists", ezOSFile::ExistsFile(sPluginBinary));

  // Only tests for missing outputs and an outdated CMake cache - it does not compare source timestamps,
  // so false does not mean the binary is up to date with the sources. Build anyway after editing code.
  writer.AddVariableBool("buildRequired", ezCppProject::IsBuildRequired());

  writer.AddVariableBool("compilerUsable", compilerStatus.Succeeded());
  if (compilerStatus.Failed())
  {
    writer.AddVariableString("compilerError", compilerStatus.GetMessageString());
  }

  // A plugin has to be built with the compiler the SDK was built with, which is the first thing
  // TestCompiler() checks - so a usable compiler is this one, and 'compilerError' names the configured
  // one when it is not. cpp_generate's 'useSdkCompiler' switches the preference over.
  writer.AddVariableString("sdkCompiler", ezCppProject::CompilerToString(ezCppProject::GetSdkCompiler()));
  writer.AddVariableString("sdkCompilerVersion", ezCppProject::GetSdkCompilerMajorVersion());

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}

void ezMcpCppTool::ExecuteGenerate(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezCppSettings cfg;
  if (!PrepareSettings(cfg, out_result))
    return;

  const bool bCompile = ezMcpJson::GetBool(arguments, "compile", true);
  const bool bCleanBuildDir = ezMcpJson::GetBool(arguments, "cleanBuildDirectory", false);
  const bool bUseSdkCompiler = ezMcpJson::GetBool(arguments, "useSdkCompiler", false);

  {
    ezStringBuilder sRequestedName = ezMcpJson::GetString(arguments, "pluginName");
    sRequestedName.Trim(" \t");

    // The dialog stores the name without the suffix and appends it everywhere, so a caller that spells
    // out 'FooPlugin' - the name of the resulting library - would otherwise create 'FooPluginPlugin'.
    if (sRequestedName.EndsWith_NoCase("Plugin"))
    {
      sRequestedName.Shrink(0, 6);
    }

    if (!cfg.m_sPluginName.IsEmpty() && !sRequestedName.IsEmpty() && cfg.m_sPluginName != sRequestedName)
    {
      // Renaming is refused rather than performed: the sources, the CMake files and the file names on
      // disk all contain the old name and are deliberately not touched, so the rename would produce a
      // project that no longer builds. Deleting those files by hand is the only way through, which is
      // not something to do behind the caller's back.
      out_result.SetError(ezStringBuilder("This project's C++ plugin is called '", cfg.m_sPluginName, "', it cannot be renamed to '",
        sRequestedName, "'. The existing sources and CMake files were written with the old name and are not modified, so the "
                        "rename would leave a project that does not build. Omit 'pluginName' to keep the existing one."));
      return;
    }

    if (cfg.m_sPluginName.IsEmpty())
    {
      cfg.m_sPluginName = sRequestedName.IsEmpty() ? GetEffectivePluginName(cfg) : ezString(sRequestedName);
    }
  }

  if (cfg.Save().Failed())
  {
    out_result.SetError("Failed to save the C++ project settings to ':project/Editor/CppProject.ddl'.");
    return;
  }

  ezLogSystemToBuffer logBuffer;
  ezStatus result = ezStatus(EZ_SUCCESS);
  bool bCompiled = false;

  {
    ezLogSystemScope logScope(&logBuffer);

    if (bUseSdkCompiler && ezCppProject::ForceSdkCompatibleCompiler().Failed())
    {
      ezLog::Warning("No compiler compatible with this SDK was found on this machine, keeping the configured one.");
    }

    if (bCleanBuildDir && ezCppProject::CleanBuildDir(cfg).Failed())
    {
      // Not fatal: the usual cause is the solution being open in an IDE, and CMake can regenerate into
      // an existing directory. Reported so a later stale-looking failure has an explanation.
      ezLog::Warning("Could not delete the build directory '{}'. It is probably open in an IDE.", ezCppProject::GetBuildDir(cfg));
    }

    if (ezCppProject::PopulateWithDefaultSources(cfg).Failed())
    {
      result = ezStatus("Writing the default C++ source files into the project directory failed.");
    }

    if (result.Succeeded() && ezCppProject::RunCMake(cfg).Failed())
    {
      result = ezStatus("Generating the C++ solution with CMake failed.");
    }

    if (result.Succeeded() && bCompile)
    {
      if (ezCppProject::CompileSolution(cfg).Failed())
      {
        result = ezStatus("Compiling the generated C++ solution failed.");
      }
      else
      {
        bCompiled = true;
      }
    }
  }

  if (result.Succeeded())
  {
    // Writes the plugin's config header and reloads the engine process, so that the plugin that was
    // just built is the one in use. Without this the editor keeps running the previous library.
    ezCppProject::UpdatePluginConfig(cfg);
    ezQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableBool("generated", result.Succeeded());
  writer.AddVariableString("pluginName", cfg.m_sPluginName);
  writer.AddVariableString("sourceDirectory", ezCppProject::GetTargetSourceDir());
  writer.AddVariableString("solutionPath", ezCppProject::GetSolutionPath(cfg));
  writer.AddVariableBool("compiled", bCompiled);

  if (bCompiled)
  {
    writer.AddVariableString("pluginBinary", GetPluginBinaryPath(cfg));
  }

  if (result.Failed())
  {
    writer.AddVariableString("error", result.GetMessageString());
  }

  writer.AddVariableString("log", logBuffer.m_sBuffer);

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
  out_result.m_bIsError = result.Failed();
}

void ezMcpCppTool::ExecuteBuild(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezCppSettings cfg;
  if (!PrepareSettings(cfg, out_result))
    return;

  if (!ezCppProject::ExistsProjectCMakeListsTxt())
  {
    // ezCppProject reports success for a project without C++ code, which as a tool result would read as
    // "built" and leave a caller waiting for a plugin that is never going to appear.
    out_result.SetError(ezStringBuilder("This project has no C++ code: there is no CMakeLists.txt in '",
      ezCppProject::GetTargetSourceDir(), "'. Call cpp_generate to create the C++ plugin first."));
    return;
  }

  const bool bForce = ezMcpJson::GetBool(arguments, "force", false);

  ezLogSystemToBuffer logBuffer;
  ezStatus result = ezStatus(EZ_SUCCESS);

  {
    ezLogSystemScope logScope(&logBuffer);

    // Keeps the SDK-owned files in the project's source directory current - the same step the editor
    // performs before it builds. It leaves the user's own sources alone.
    if (ezCppProject::PopulateWithDefaultSources(cfg).Failed())
    {
      result = ezStatus("Updating the default C++ source files in the project directory failed.");
    }

    if (result.Succeeded() && bForce && ezCppProject::RunCMake(cfg).Failed())
    {
      result = ezStatus("Re-generating the C++ solution with CMake failed.");
    }

    if (result.Succeeded())
    {
      // BuildCodeIfNecessary() runs CMake only when the solution is missing or its cache is outdated,
      // then compiles unconditionally - the build system decides what is actually out of date. With
      // 'force' CMake has already run above.
      if (ezCppProject::BuildCodeIfNecessary(cfg).Failed())
      {
        result = ezStatus("Compiling the C++ code failed.");
      }
    }
  }

  if (result.Succeeded())
  {
    ezQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableBool("built", result.Succeeded());
  writer.AddVariableString("pluginName", GetEffectivePluginName(cfg));
  writer.AddVariableString("pluginBinary", GetPluginBinaryPath(cfg));

  if (result.Failed())
  {
    writer.AddVariableString("error", result.GetMessageString());
  }

  writer.AddVariableString("log", logBuffer.m_sBuffer);

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
  out_result.m_bIsError = result.Failed();
}
