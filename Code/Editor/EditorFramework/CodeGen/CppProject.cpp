#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/System/Process.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  include <Shlobj.h>
#endif

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezIDE, 1)
  EZ_ENUM_CONSTANT(ezIDE::VisualStudioCode),
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  EZ_ENUM_CONSTANT(ezIDE::VisualStudio),
#endif
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezCompiler, 1)
  EZ_ENUM_CONSTANT(ezCompiler::Clang),
#if EZ_ENABLED(EZ_PLATFORM_LINUX)
  EZ_ENUM_CONSTANT(ezCompiler::Gcc),
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  EZ_ENUM_CONSTANT(ezCompiler::Vs2022),
#endif
EZ_END_STATIC_REFLECTED_ENUM;

#if EZ_ENABLED(EZ_PLATFORM_LINUX)
#define CPP_COMPILER_DEFAULT "g++"
#define C_COMPILER_DEFAULT "gcc"
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#define CPP_COMPILER_DEFAULT ""
#define C_COMPILER_DEFAULT ""
#else
#error Platform not implemented
#endif

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezCompilerPreferences, ezNoBase, 1, ezRTTIDefaultAllocator<ezCompilerPreferences>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Compiler", ezCompiler, m_Compiler)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("CustomCompiler", m_bCustomCompiler)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("CppCompiler", m_sCppCompiler)->AddAttributes(new ezDefaultValueAttribute(CPP_COMPILER_DEFAULT)),
    EZ_MEMBER_PROPERTY("CCompiler", m_sCCompiler)->AddAttributes(new ezDefaultValueAttribute(C_COMPILER_DEFAULT)),
    EZ_MEMBER_PROPERTY("RcCompiler", m_sRcCompiler),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezCodeEditorPreferences, ezNoBase, 1, ezRTTIDefaultAllocator<ezCodeEditorPreferences>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("CodeEditorPath", m_sEditorPath)->AddAttributes(new ezExternalFileBrowserAttribute("Select Editor", "*.exe"_ezsv)),
    EZ_MEMBER_PROPERTY("CodeEditorArgs", m_sEditorArgs)->AddAttributes(new ezDefaultValueAttribute("{file} {line}")),
    EZ_MEMBER_PROPERTY("IsVisualStudio", m_bIsVisualStudio)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCppProject, 1, ezRTTIDefaultAllocator<ezCppProject>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("CppIDE", ezIDE, m_Ide),
    EZ_MEMBER_PROPERTY("CompilerPreferences", m_CompilerPreferences),
    EZ_MEMBER_PROPERTY("CodeEditorPreferences", m_CodeEditorPreferences),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezEvent<const ezCppSettings&> ezCppProject::s_ChangeEvents;

ezDynamicArray<ezCppProject::MachineSpecificCompilerPaths> ezCppProject::s_MachineSpecificCompilers;

namespace
{
  static constexpr ezUInt32 minGccVersion = 10;
  static constexpr ezUInt32 maxGccVersion = 20;
  static constexpr ezUInt32 minClangVersion = 10;
  static constexpr ezUInt32 maxClangVersion = 20;

  ezResult TestCompilerExecutable(ezStringView sName, ezString* out_pVersion = nullptr)
  {
    ezStringBuilder sStdout;
    ezProcessOptions po;
    po.AddArgument("--version");
    po.m_sProcess = sName;
    po.m_onStdOut = [&sStdout](ezStringView out)
    { sStdout.Append(out); };

    if (ezProcess::Execute(po).Failed())
      return EZ_FAILURE;

    ezHybridArray<ezStringView, 8> lines;
    sStdout.Split(false, lines, "\r", "\n");
    if (lines.IsEmpty())
      return EZ_FAILURE;

    ezHybridArray<ezStringView, 4> splitResult;
    lines[0].Split(false, splitResult, " ");

    if (splitResult.IsEmpty())
      return EZ_FAILURE;

    ezStringView version = splitResult.PeekBack();
    splitResult.Clear();
    version.Split(false, splitResult, ".");
    if (splitResult.GetCount() < 3)
    {
      return EZ_FAILURE;
    }

    if (out_pVersion)
    {
      *out_pVersion = version;
    }

    return EZ_SUCCESS;
  }

  void AddCompilerVersions(ezDynamicArray<ezCppProject::MachineSpecificCompilerPaths>& inout_compilers, ezCompiler::Enum compiler, ezStringView sRequiredMajorVersion)
  {
    ezStringView compilerBaseName;
    ezStringView compilerBaseNameCpp;
    switch (compiler)
    {
      case ezCompiler::Clang:
        compilerBaseName = "clang";
        compilerBaseNameCpp = "clang++";
        break;
#if EZ_ENABLED(EZ_PLATFORM_LINUX)
      case ezCompiler::Gcc:
        compilerBaseName = "gcc";
        compilerBaseNameCpp = "g++";
        break;
#endif
      default:
        EZ_ASSERT_NOT_IMPLEMENTED
    }


    ezString compilerVersion;
    ezStringBuilder requiredVersion = sRequiredMajorVersion;
    requiredVersion.Append('.');
    ezStringBuilder fmt;
    if (TestCompilerExecutable(compilerBaseName, &compilerVersion).Succeeded() && TestCompilerExecutable(compilerBaseNameCpp).Succeeded() && compilerVersion.StartsWith(requiredVersion))
    {
      fmt.SetFormat("{} (system default = {})", compilerBaseName, compilerVersion);
      inout_compilers.PushBack({fmt.GetView(), compiler, compilerBaseName, compilerBaseNameCpp, false});
    }

    ezStringBuilder compilerExecutable;
    ezStringBuilder compilerExecutableCpp;
    compilerExecutable.SetFormat("{}-{}", compilerBaseName, sRequiredMajorVersion);
    compilerExecutableCpp.SetFormat("{}-{}", compilerBaseNameCpp, sRequiredMajorVersion);
    if (TestCompilerExecutable(compilerExecutable, &compilerVersion).Succeeded() && TestCompilerExecutable(compilerExecutableCpp).Succeeded() && compilerVersion.StartsWith(requiredVersion))
    {
      fmt.SetFormat("{} (version {})", compilerBaseName, compilerVersion);
      inout_compilers.PushBack({fmt.GetView(), compiler, compilerExecutable, compilerExecutableCpp, false});
    }
  }
} // namespace

ezString ezCppProject::GetTargetSourceDir(ezStringView sProjectDirectory /*= {}*/)
{
  ezStringBuilder sTargetDir = sProjectDirectory;

  if (sTargetDir.IsEmpty())
  {
    sTargetDir = ezToolsProject::GetSingleton()->GetProjectDirectory();
  }

  sTargetDir.AppendPath("CppSource");
  return sTargetDir;
}

ezString ezCppProject::GetGeneratorFolderName(const ezCppSettings& cfg)
{
  const ezCppProject* preferences = ezPreferences::QueryPreferences<ezCppProject>();

  switch (preferences->m_CompilerPreferences.m_Compiler.GetValue())
  {
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    case ezCompiler::Vs2022:
      return "Vs2022x64";
#endif
    case ezCompiler::Clang:
      return "Clangx64";
#if EZ_ENABLED(EZ_PLATFORM_LINUX)
    case ezCompiler::Gcc:
      return "Gccx64";
#endif
  }
  EZ_ASSERT_NOT_IMPLEMENTED;
  return "";
}

ezString ezCppProject::GetCMakeGeneratorName(const ezCppSettings& cfg)
{

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  const ezCppProject* preferences = ezPreferences::QueryPreferences<ezCppProject>();

  switch (preferences->m_CompilerPreferences.m_Compiler.GetValue())
  {
    case ezCompiler::Vs2022:
      return "Visual Studio 17 2022";
    case ezCompiler::Clang:
      return "Ninja";
  }
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
  return "Ninja";
#else
#  error Platform not implemented
#endif
  EZ_ASSERT_NOT_IMPLEMENTED;
  return "";
}

ezString ezCppProject::GetPluginSourceDir(const ezCppSettings& cfg, ezStringView sProjectDirectory /*= {}*/)
{
  ezStringBuilder sDir = GetTargetSourceDir(sProjectDirectory);
  sDir.AppendPath(cfg.m_sPluginName);
  sDir.Append("Plugin");
  return sDir;
}

ezString ezCppProject::GetBuildDir(const ezCppSettings& cfg)
{
  ezStringBuilder sBuildDir;
  sBuildDir.SetFormat("{}/Build/{}", GetTargetSourceDir(), GetGeneratorFolderName(cfg));
  return sBuildDir;
}

ezString ezCppProject::GetSolutionPath(const ezCppSettings& cfg)
{
  ezStringBuilder sSolutionFile;
  sSolutionFile = GetBuildDir(cfg);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  const ezCppProject* preferences = ezPreferences::QueryPreferences<ezCppProject>();
  if (preferences->m_CompilerPreferences.m_Compiler == ezCompiler::Vs2022)
  {
    sSolutionFile.AppendPath(cfg.m_sPluginName);
    sSolutionFile.Append(".sln");
    return sSolutionFile;
  }
#endif

  sSolutionFile.AppendPath("build.ninja");
  return sSolutionFile;
}

ezStatus ezCppProject::OpenSolution(const ezCppSettings& cfg)
{
  const ezCppProject* preferences = ezPreferences::QueryPreferences<ezCppProject>();

  switch (preferences->m_Ide.GetValue())
  {
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    case ezIDE::VisualStudio:
      if (!ezQtUiServices::OpenFileInDefaultProgram(ezCppProject::GetSolutionPath(cfg)))
      {
        return ezStatus("Opening the solution in Visual Studio failed.");
      }
      break;
#endif
    case ezIDE::VisualStudioCode:
    {
      auto solutionPath = ezCppProject::GetTargetSourceDir();
      QStringList args;
      args.push_back(QString::fromUtf8(solutionPath.GetData(), solutionPath.GetElementCount()));
      if (ezStatus status = ezQtUiServices::OpenInVsCode(args); status.Failed())
      {
        return ezStatus(ezFmt("Opening Visual Studio Code failed: {}", status.m_sMessage));
      }
    }
    break;
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCppProject::OpenInCodeEditor(const ezStringView& sFileName, ezInt32 iLineNumber)
{
  if (!ezOSFile::ExistsFile(sFileName))
  {
    return ezStatus("Failed finding filename");
  }

  ezStringBuilder sLineNumber;
  ezConversionUtils::ToString(iLineNumber, sLineNumber);

  const ezCppProject* preferences = ezPreferences::QueryPreferences<ezCppProject>();

  // Visual Studio does not expose a CLI command to open a file/line in all use-cases directly
  // therefore run a custom .vbs script which controls VS and performs the needed actions for us. This avoids pulling COM interfacing into the project.
  if (preferences->m_CodeEditorPreferences.m_bIsVisualStudio)
  {
    ezStringBuilder dir;
    if (ezFileSystem::ResolveSpecialDirectory(">sdk/Utilities/Scripts/open-in-msvs.vbs", dir).Failed())
    {
      return ezStatus("Failed resolving path to \">sdk/Utilities/Scripts/open-in-msvs.vbs\"");
    }

    if (!ezOSFile::ExistsFile(dir))
    {
      return ezStatus(ezFmt("File does not exist '{0}'", dir));
    }

    QStringList args;
    args.append("/B");
    args.append(QString::fromUtf8(dir.GetData()));
    args.append(QString::fromUtf8(sFileName.GetStartPointer(), sFileName.GetElementCount()));
    args.append(QString::fromUtf8(sLineNumber.GetData()));

    QProcess proc;
    if (proc.startDetached("cscript", args) == false)
    {
      return ezStatus("Failed to launch code editor");
    }

    return ezStatus(EZ_SUCCESS);
  }


  ezStringBuilder sFormatString = preferences->m_CodeEditorPreferences.m_sEditorArgs;
  if (sFormatString.IsEmpty())
  {
    return ezStatus("Code editor is not configured");
  }

  sFormatString.ReplaceAll("{line}", sLineNumber);
  sFormatString.ReplaceAll("{file}", sFileName);

  const QStringList args = QProcess::splitCommand(QString::fromUtf8(sFormatString.GetData()));
  const QString sProgramPath = QString::fromUtf8(preferences->m_CodeEditorPreferences.m_sEditorPath.GetData());

  QProcess proc;
  if (proc.startDetached(sProgramPath, args) == false)
  {
    return ezStatus("Failed to launch code editor");
  }
  return ezStatus(EZ_SUCCESS);
}

ezStringView ezCppProject::CompilerToString(ezCompiler::Enum compiler)
{
  switch (compiler)
  {
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    case ezCompiler::Vs2022:
      return "Vs2022";
#endif
    case ezCompiler::Clang:
      return "Clang";
#if EZ_ENABLED(EZ_PLATFORM_LINUX)
    case ezCompiler::Gcc:
      return "Gcc";
#endif
    default:
      break;
  }
  EZ_ASSERT_NOT_IMPLEMENTED;
  return "<not implemented>";
}

ezCompiler::Enum ezCppProject::GetSdkCompiler()
{
#if EZ_ENABLED(EZ_COMPILER_CLANG)
  return ezCompiler::Clang;
#elif EZ_ENABLED(EZ_COMPILER_GCC)
  return ezCompiler::Gcc;
#elif EZ_ENABLED(EZ_COMPILER_MSVC)
  return ezCompiler::Vs2022;
#else
#  error Unknown compiler
#endif
}

ezString ezCppProject::GetSdkCompilerMajorVersion()
{
#if EZ_ENABLED(EZ_COMPILER_MSVC)
  ezStringBuilder fmt;
  fmt.SetFormat("{}.{}", _MSC_VER / 100, _MSC_VER % 100);
  return fmt;
#elif EZ_ENABLED(EZ_COMPILER_CLANG)
  return EZ_PP_STRINGIFY(__clang_major__);
#elif EZ_ENABLED(EZ_COMPILER_GCC)
  return EZ_PP_STRINGIFY(__GNUC__);
#else
#  error Unsupported compiler
#endif
}

ezStatus ezCppProject::TestCompiler()
{
  const ezCppProject* preferences = ezPreferences::QueryPreferences<ezCppProject>();
  if (preferences->m_CompilerPreferences.m_Compiler != GetSdkCompiler())
  {
    return ezStatus(ezFmt("The currently configured compiler is incompatible with this SDK. The SDK was built with '{}' but the currently configured compiler is '{}'.",
      CompilerToString(GetSdkCompiler()),
      CompilerToString(preferences->m_CompilerPreferences.m_Compiler)));
  }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  // As CMake is selecting the compiler it is hard to do a version check, for now just assume they are compatible.
  if (GetSdkCompiler() == ezCompiler::Vs2022)
  {
    return ezStatus(EZ_SUCCESS);
  }

  if (GetSdkCompiler() == ezCompiler::Clang)
  {
    if (!ezOSFile::ExistsFile(preferences->m_CompilerPreferences.m_sRcCompiler))
    {
      return ezStatus(ezFmt("The selected RC compiler '{}' does not exist on disk.", preferences->m_CompilerPreferences.m_sRcCompiler));
    }
  }
#endif

  ezString cCompilerVersion, cppCompilerVersion;
  if (TestCompilerExecutable(preferences->m_CompilerPreferences.m_sCCompiler, &cCompilerVersion).Failed())
  {
    return ezStatus("The selected C Compiler doesn't work or doesn't exist.");
  }
  if (TestCompilerExecutable(preferences->m_CompilerPreferences.m_sCppCompiler, &cppCompilerVersion).Failed())
  {
    return ezStatus("The selected C++ Compiler doesn't work or doesn't exist.");
  }

  ezStringBuilder sdkCompilerMajorVersion = GetSdkCompilerMajorVersion();
  sdkCompilerMajorVersion.Append('.');
  if (!cCompilerVersion.StartsWith(sdkCompilerMajorVersion))
  {
    return ezStatus(ezFmt("The selected C Compiler has an incompatible version. The SDK was built with version {} but the compiler has version {}.", GetSdkCompilerMajorVersion(), cCompilerVersion));
  }
  if (!cppCompilerVersion.StartsWith(sdkCompilerMajorVersion))
  {
    return ezStatus(ezFmt("The selected C++ Compiler has an incompatible version. The SDK was built with version {} but the compiler has version {}.", GetSdkCompilerMajorVersion(), cppCompilerVersion));
  }

  return ezStatus(EZ_SUCCESS);
}

const char* ezCppProject::GetCMakePath()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  return "cmake/bin/cmake";
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
  return "cmake";
#else
#  error Platform not implemented
#endif
}

ezResult ezCppProject::CheckCMakeCache(const ezCppSettings& cfg)
{
  ezStringBuilder sCacheFile;
  sCacheFile = GetBuildDir(cfg);
  sCacheFile.AppendPath("CMakeCache.txt");

  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(sCacheFile));

  ezStringBuilder content;
  content.ReadAll(file);

  const ezStringView sSearchFor = "CMAKE_CONFIGURATION_TYPES:STRING="_ezsv;

  const char* pConfig = content.FindSubString(sSearchFor);
  if (pConfig == nullptr)
    return EZ_FAILURE;

  pConfig += sSearchFor.GetElementCount();

  const char* pEndConfig = content.FindSubString("\n", pConfig);
  if (pEndConfig == nullptr)
    return EZ_FAILURE;

  ezStringBuilder sUsedCfg;
  sUsedCfg.SetSubString_FromTo(pConfig, pEndConfig);
  sUsedCfg.Trim("\t\n\r ");

  if (sUsedCfg != BUILDSYSTEM_BUILDTYPE)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezCppProject::ModifyResult ezCppProject::CheckCMakeUserPresets(const ezCppSettings& cfg, bool bWriteResult)
{
  ezStringBuilder configureJsonPath = ezCppProject::GetPluginSourceDir(cfg).GetFileDirectory();
  configureJsonPath.AppendPath("CMakeUserPresets.json");

  if (ezOSFile::ExistsFile(configureJsonPath))
  {
    ezFileReader fileReader;
    if (fileReader.Open(configureJsonPath).Failed())
    {
      ezLog::Error("Failed to open '{}' for reading", configureJsonPath);
      return ModifyResult::FAILURE;
    }
    ezJSONReader reader;
    if (reader.Parse(fileReader).Failed())
    {
      ezLog::Error("Failed to parse JSON of '{}'", configureJsonPath);
      return ModifyResult::FAILURE;
    }
    fileReader.Close();

    if (reader.GetTopLevelElementType() != ezJSONReader::ElementType::Dictionary)
    {
      ezLog::Error("Top level element of '{}' is expected to be a dictionary. Please manually fix, rename or delete the file.", configureJsonPath);
      return ModifyResult::FAILURE;
    }

    ezVariantDictionary json = reader.GetTopLevelObject();
    auto modifyResult = ModifyCMakeUserPresetsJson(cfg, json);
    if (modifyResult == ModifyResult::FAILURE)
    {
      ezLog::Error("Failed to modify '{}' in place. Please manually fix, rename or delete the file.", configureJsonPath);
      return ModifyResult::FAILURE;
    }

    if (bWriteResult && modifyResult == ModifyResult::MODIFIED)
    {
      ezStandardJSONWriter jsonWriter;
      ezDeferredFileWriter fileWriter;
      fileWriter.SetOutput(configureJsonPath);
      jsonWriter.SetOutputStream(&fileWriter);

      jsonWriter.WriteVariant(ezVariant(json));
      if (fileWriter.Close().Failed())
      {
        ezLog::Error("Failed to write CMakeUserPresets.json to '{}'", configureJsonPath);
        return ModifyResult::FAILURE;
      }
    }

    return modifyResult;
  }
  else
  {
    if (bWriteResult)
    {
      ezStandardJSONWriter jsonWriter;
      ezDeferredFileWriter fileWriter;
      fileWriter.SetOutput(configureJsonPath);
      jsonWriter.SetOutputStream(&fileWriter);

      jsonWriter.WriteVariant(ezVariant(CreateEmptyCMakeUserPresetsJson(cfg)));
      if (fileWriter.Close().Failed())
      {
        ezLog::Error("Failed to write CMakeUserPresets.json to '{}'", configureJsonPath);
        return ModifyResult::FAILURE;
      }
    }
  }

  return ModifyResult::MODIFIED;
}

bool ezCppProject::ExistsSolution(const ezCppSettings& cfg)
{
  return ezOSFile::ExistsFile(GetSolutionPath(cfg));
}

bool ezCppProject::ExistsProjectCMakeListsTxt()
{
  if (!ezToolsProject::IsProjectOpen())
    return false;

  ezStringBuilder sPath = GetTargetSourceDir();
  sPath.AppendPath("CMakeLists.txt");
  return ezOSFile::ExistsFile(sPath);
}

ezResult ezCppProject::PopulateWithDefaultSources(const ezCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  EZ_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  const ezString sProjectName = cfg.m_sPluginName;

  ezStringBuilder sProjectNameUpper = cfg.m_sPluginName;
  sProjectNameUpper.ToUpper();

  const ezStringBuilder sTargetDir = ezToolsProject::GetSingleton()->GetProjectDirectory();

  ezStringBuilder sSourceDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSourceDir.AppendPath("CppProject");

  ezDynamicArray<ezFileStats> items;
  ezOSFile::GatherAllItemsInFolder(items, sSourceDir, ezFileSystemIteratorFlags::ReportFilesRecursive);

  struct FileToCopy
  {
    ezString m_sSource;
    ezString m_sDestination;
  };

  ezHybridArray<FileToCopy, 32> filesCopied;

  // gather files
  {
    for (const auto& item : items)
    {
      ezStringBuilder srcPath, dstPath;
      item.GetFullPath(srcPath);

      dstPath = srcPath;
      dstPath.MakeRelativeTo(sSourceDir).IgnoreResult();

      dstPath.ReplaceAll("CppProject", sProjectName);
      dstPath.Prepend(sTargetDir, "/");
      dstPath.MakeCleanPath();

      // don't copy files over that already exist (and may have edits)
      if (ezOSFile::ExistsFile(dstPath))
      {
        // if any file already exists, don't copy non-existing (user might have deleted unwanted sample files)
        filesCopied.Clear();
        break;
      }

      auto& ftc = filesCopied.ExpandAndGetRef();
      ftc.m_sSource = srcPath;
      ftc.m_sDestination = dstPath;
    }
  }

  // Copy files
  {
    for (const auto& ftc : filesCopied)
    {
      if (ezOSFile::CopyFile(ftc.m_sSource, ftc.m_sDestination).Failed())
      {
        ezLog::Error("Failed to copy a file.\nSource: '{}'\nDestination: '{}'\n", ftc.m_sSource, ftc.m_sDestination);
        return EZ_FAILURE;
      }
    }
  }

  // Modify sources
  {
    for (const auto& filePath : filesCopied)
    {
      ezStringBuilder content;

      {
        ezFileReader file;
        if (file.Open(filePath.m_sDestination).Failed())
        {
          ezLog::Error("Failed to open C++ project file for reading.\nSource: '{}'\n", filePath.m_sDestination);
          return EZ_FAILURE;
        }

        content.ReadAll(file);
      }

      content.ReplaceAll("CppProject", sProjectName);
      content.ReplaceAll("CPPPROJECT", sProjectNameUpper);

      {
        ezFileWriter file;
        if (file.Open(filePath.m_sDestination).Failed())
        {
          ezLog::Error("Failed to open C++ project file for writing.\nSource: '{}'\n", filePath.m_sDestination);
          return EZ_FAILURE;
        }

        file.WriteBytes(content.GetData(), content.GetElementCount()).IgnoreResult();
      }
    }
  }

  s_ChangeEvents.Broadcast(cfg);
  return EZ_SUCCESS;
}

ezResult ezCppProject::CleanBuildDir(const ezCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  EZ_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  const ezString sBuildDir = GetBuildDir(cfg);

  if (!ezOSFile::ExistsDirectory(sBuildDir))
    return EZ_SUCCESS;

  return ezOSFile::DeleteFolder(sBuildDir);
}

ezResult ezCppProject::RunCMake(const ezCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  EZ_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  if (!ExistsProjectCMakeListsTxt())
  {
    ezLog::Error("No CMakeLists.txt exists in target source directory '{}'", GetTargetSourceDir());
    return EZ_FAILURE;
  }

  if (auto compilerWorking = TestCompiler(); compilerWorking.Failed())
  {
    compilerWorking.LogFailure();
    return EZ_FAILURE;
  }

  if (CheckCMakeUserPresets(cfg, true) == ModifyResult::FAILURE)
  {
    return EZ_FAILURE;
  }

  ezStringBuilder tmp;

  QStringList args;
  args << "--preset";
  args << "ezEngine";

  ezLogSystemToBuffer log;


  const ezString sTargetSourceDir = ezCppProject::GetTargetSourceDir();

  ezStatus res = ezQtEditorApp::GetSingleton()->ExecuteTool(GetCMakePath(), args, 120, &log, ezLogMsgType::InfoMsg, sTargetSourceDir);

  if (res.Failed())
  {
    ezLog::Error("CMake generation failed:\n\n{}\n{}\n", log.m_sBuffer, res.m_sMessage);
    return EZ_FAILURE;
  }

  if (!ExistsSolution(cfg))
  {
    ezLog::Error("CMake did not generate the expected output. Did you attempt to rename it? If so, you may need to delete the top-level CMakeLists.txt file and set up the C++ project again.");
    return EZ_FAILURE;
  }

  ezLog::Success("CMake generation successful.\n\n{}\n", log.m_sBuffer);
  s_ChangeEvents.Broadcast(cfg);
  return EZ_SUCCESS;
}

ezResult ezCppProject::RunCMakeIfNecessary(const ezCppSettings& cfg)
{
  if (!ezCppProject::ExistsProjectCMakeListsTxt())
    return EZ_SUCCESS;

  auto userPresetResult = CheckCMakeUserPresets(cfg, false);
  if (userPresetResult == ModifyResult::FAILURE)
  {
    return EZ_FAILURE;
  }

  if (ezCppProject::ExistsSolution(cfg) && ezCppProject::CheckCMakeCache(cfg).Succeeded() && userPresetResult == ModifyResult::NOT_MODIFIED)
    return EZ_SUCCESS;

  return ezCppProject::RunCMake(cfg);
}

ezResult ezCppProject::CompileSolution(const ezCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  EZ_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  EZ_LOG_BLOCK("Compile C++ Plugin");

  ezHybridArray<ezString, 32> errors;
  ezInt32 iReturnCode = 0;
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  if (ezSystemInformation::IsDebuggerAttached())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("When a debugger is attached, MSBuild usually fails to compile the project.\n\nDetach the debugger now, then press OK to continue.");
  }
#endif

  ezProcessOptions po;

  ezString cmakePath = ezQtEditorApp::GetSingleton()->FindToolApplication(ezCppProject::GetCMakePath());

  po.m_sProcess = cmakePath;
  po.AddArgument("--build");
  po.AddArgument(GetBuildDir(cfg));
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  const ezCppProject* preferences = ezPreferences::QueryPreferences<ezCppProject>();

  if (preferences->m_CompilerPreferences.m_Compiler == ezCompiler::Vs2022)
  {
    po.AddArgument("--config");
    po.AddArgument(BUILDSYSTEM_BUILDTYPE);
  }
#endif
  po.m_sWorkingDirectory = GetBuildDir(cfg);
  po.m_bHideConsoleWindow = true;
  po.m_onStdOut = [&](ezStringView sText)
  {
    if (sText.FindSubString_NoCase("error") != nullptr)
      errors.PushBack(sText);
  };
  po.m_onStdError = [&](ezStringView sText)
  {
    if (sText.FindSubString_NoCase("error") != nullptr)
      errors.PushBack(sText);
  };

  ezStringBuilder sCMakeBuildCmd;
  po.BuildCommandLineString(sCMakeBuildCmd);
  ezLog::Dev("Running {} {}", cmakePath, sCMakeBuildCmd);
  if (ezProcess::Execute(po, &iReturnCode).Failed())
  {
    ezLog::Error("Failed to start CMake.");
    return EZ_FAILURE;
  }

  if (iReturnCode == 0)
  {
    ezLog::Success("Compiled C++ code.");
    return EZ_SUCCESS;
  }

  ezLog::Error("CMake --build failed with return code {}", iReturnCode);

  for (const auto& err : errors)
  {
    ezLog::Error(err);
  }

  return EZ_FAILURE;
}

ezResult ezCppProject::BuildCodeIfNecessary(const ezCppSettings& cfg)
{
  if (!ezCppProject::ExistsProjectCMakeListsTxt())
    return EZ_SUCCESS;

  if (!ezCppProject::ExistsSolution(cfg) || ezCppProject::CheckCMakeCache(cfg).Failed())
  {
    EZ_SUCCEED_OR_RETURN(ezCppProject::RunCMake(cfg));
  }

  return CompileSolution(cfg);
}

ezVariantDictionary ezCppProject::CreateEmptyCMakeUserPresetsJson(const ezCppSettings& cfg)
{
  ezVariantDictionary json;
  json.Insert("version", 3);

  {
    ezVariantDictionary cmakeMinimumRequired;
    cmakeMinimumRequired.Insert("major", 3);
    cmakeMinimumRequired.Insert("minor", 21);
    cmakeMinimumRequired.Insert("patch", 0);

    json.Insert("cmakeMinimumRequired", std::move(cmakeMinimumRequired));
  }

  {
    ezVariantArray configurePresets;
    ezVariantDictionary ezEnginePreset;
    ezEnginePreset.Insert("name", "ezEngine");
    ezEnginePreset.Insert("displayName", "Build the ezEngine Plugin");

    {
      ezVariantDictionary cacheVariables;
      ezEnginePreset.Insert("cacheVariables", std::move(cacheVariables));
    }

    configurePresets.PushBack(std::move(ezEnginePreset));
    json.Insert("configurePresets", std::move(configurePresets));
  }

  {
    ezVariantArray buildPresets;
    {
      ezVariantDictionary ezEngineBuildPreset;
      ezEngineBuildPreset.Insert("name", "ezEngine");
      ezEngineBuildPreset.Insert("configurePreset", "ezEngine");
      buildPresets.PushBack(std::move(ezEngineBuildPreset));
    }
    json.Insert("buildPresets", std::move(buildPresets));
  }

  EZ_VERIFY(ModifyCMakeUserPresetsJson(cfg, json) == ModifyResult::MODIFIED, "Freshly created user presets file should always be modified");

  return json;
}

void ezCppProject::UpdatePluginConfig(const ezCppSettings& cfg)
{
  const ezStringBuilder sPluginName(cfg.m_sPluginName, "Plugin");

  ezPluginBundleSet& bundles = ezQtEditorApp::GetSingleton()->GetPluginBundles();

  ezStringBuilder txt;
  bundles.m_Plugins.Remove(sPluginName);
  ezPluginBundle& plugin = bundles.m_Plugins[sPluginName];
  plugin.m_bLoadCopy = true;
  plugin.m_bSelected = true;
  plugin.m_bMissing = true;
  plugin.m_LastModificationTime = ezTimestamp::MakeInvalid();
  plugin.m_ExclusiveFeatures.PushBack("ProjectPlugin");
  txt.Set("'", cfg.m_sPluginName, "' project plugin");
  plugin.m_sDisplayName = txt;
  txt.Set("C++ code for the '", cfg.m_sPluginName, "' project.");
  plugin.m_sDescription = txt;
  plugin.m_RuntimePlugins.PushBack(sPluginName);

  ezQtEditorApp::GetSingleton()->WritePluginSelectionStateDDL();
}

ezResult ezCppProject::EnsureCppPluginReady()
{
  if (!ExistsProjectCMakeListsTxt())
    return EZ_SUCCESS;

  ezCppSettings cppSettings;
  if (cppSettings.Load().Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to load the C++ plugin settings."));
    return EZ_FAILURE;
  }

  if (ezCppProject::BuildCodeIfNecessary(cppSettings).Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to build the C++ code. See log for details."));
    return EZ_FAILURE;
  }

  ezQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);
  return EZ_SUCCESS;
}

bool ezCppProject::IsBuildRequired()
{
  if (!ExistsProjectCMakeListsTxt())
    return false;

  ezCppSettings cfg;
  if (cfg.Load().Failed())
    return false;

  if (!ezCppProject::ExistsSolution(cfg))
    return true;

  if (ezCppProject::CheckCMakeCache(cfg).Failed())
    return true;

  ezStringBuilder sPath = ezOSFile::GetApplicationDirectory();
  sPath.AppendPath(cfg.m_sPluginName);

  sPath.Append("Plugin");

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  sPath.Append(".dll");
#else
  sPath.Append(".so");
#endif

  if (!ezOSFile::ExistsFile(sPath))
    return true;

  return false;
}

namespace
{
  template <typename T>
  T* Expect(ezVariantDictionary& inout_json, ezStringView sName)
  {
    ezVariant* var = nullptr;
    if (inout_json.TryGetValue(sName, var) && var->IsA<T>())
    {
      return &var->GetWritable<T>();
    }
    return nullptr;
  }

  void Modify(ezVariantDictionary& inout_json, ezStringView sName, ezStringView sValue, ezCppProject::ModifyResult& inout_modified)
  {
    ezVariant* currentValue = nullptr;
    if (inout_json.TryGetValue(sName, currentValue) && currentValue->IsA<ezString>() && currentValue->Get<ezString>() == sValue)
      return;

    inout_json[sName] = sValue;
    inout_modified = ezCppProject::ModifyResult::MODIFIED;
  }

  void Remove(ezVariantDictionary& inout_json, ezStringView sName, ezCppProject::ModifyResult& inout_modified)
  {
    if (inout_json.Remove(sName))
    {
      inout_modified = ezCppProject::ModifyResult::MODIFIED;
    }
  }

} // namespace

ezCppProject::ModifyResult ezCppProject::ModifyCMakeUserPresetsJson(const ezCppSettings& cfg, ezVariantDictionary& inout_json)
{
  auto result = ModifyResult::NOT_MODIFIED;
  auto configurePresets = Expect<ezVariantArray>(inout_json, "configurePresets");
  if (!configurePresets)
    return ModifyResult::FAILURE;

  const ezCppProject* preferences = ezPreferences::QueryPreferences<ezCppProject>();

  for (auto& preset : *configurePresets)
  {
    if (!preset.IsA<ezVariantDictionary>())
      continue;

    auto& presetDict = preset.GetWritable<ezVariantDictionary>();

    auto name = Expect<ezString>(presetDict, "name");
    if (!name || *name != "ezEngine")
    {
      continue;
    }

    auto cacheVariables = Expect<ezVariantDictionary>(presetDict, "cacheVariables");
    if (!cacheVariables)
      return ModifyResult::FAILURE;

    Modify(*cacheVariables, "EZ_SDK_DIR", ezFileSystem::GetSdkRootDirectory(), result);
    Modify(*cacheVariables, "EZ_BUILDTYPE_ONLY", BUILDSYSTEM_BUILDTYPE, result);
    Modify(*cacheVariables, "CMAKE_BUILD_TYPE", BUILDSYSTEM_BUILDTYPE, result);

    bool needsCompilerPaths = true;
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    if (preferences->m_CompilerPreferences.m_Compiler == ezCompiler::Vs2022)
    {
      needsCompilerPaths = false;
    }
#endif

    if (needsCompilerPaths)
    {
      Modify(*cacheVariables, "CMAKE_C_COMPILER", preferences->m_CompilerPreferences.m_sCCompiler, result);
      Modify(*cacheVariables, "CMAKE_CXX_COMPILER", preferences->m_CompilerPreferences.m_sCppCompiler, result);
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
      Modify(*cacheVariables, "CMAKE_RC_COMPILER", preferences->m_CompilerPreferences.m_sRcCompiler, result);
      Modify(*cacheVariables, "CMAKE_RC_COMPILER_INIT", "rc", result);
#endif
    }
    else
    {
      cacheVariables->Remove("CMAKE_C_COMPILER");
      cacheVariables->Remove("CMAKE_CXX_COMPILER");
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
      cacheVariables->Remove("CMAKE_RC_COMPILER");
      cacheVariables->Remove("CMAKE_RC_COMPILER_INIT");
#endif
    }

    Modify(presetDict, "generator", GetCMakeGeneratorName(cfg), result);
    Modify(presetDict, "binaryDir", GetBuildDir(cfg), result);
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    if (preferences->m_CompilerPreferences.m_Compiler == ezCompiler::Vs2022)
    {
      Modify(presetDict, "architecture", "x64", result);
    }
    else
#endif
    {
      Remove(presetDict, "architecture", result);
    }
  }

  return result;
}

ezCppProject::ezCppProject()
  : ezPreferences(ezPreferences::Domain::Application, "C++ Projects")
{
}
void ezCppProject::LoadPreferences()
{
  EZ_PROFILE_SCOPE("Preferences");
  auto preferences = ezPreferences::QueryPreferences<ezCppProject>();

  ezCompiler::Enum sdkCompiler = GetSdkCompiler();

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (sdkCompiler == ezCompiler::Vs2022)
  {
    s_MachineSpecificCompilers.PushBack({"Visual Studio 2022 (system default)", ezCompiler::Vs2022, "", "", false});
  }

#  if EZ_ENABLED(EZ_COMPILER_CLANG)
  // if the rcCompiler path is empty or points to a non existant file, try to autodetect it
  if ((preferences->m_CompilerPreferences.m_sRcCompiler.IsEmpty() || !ezOSFile::ExistsFile(preferences->m_CompilerPreferences.m_sRcCompiler)))
  {
    ezStringBuilder rcPath;
    HKEY hInstalledRoots = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots", 0, KEY_READ, &hInstalledRoots) == ERROR_SUCCESS)
    {
      EZ_SCOPE_EXIT(RegCloseKey(hInstalledRoots));
      DWORD pathLengthInBytes = 0;
      ezDynamicArray<wchar_t> path;
      if (RegGetValueW(hInstalledRoots, nullptr, L"KitsRoot10", RRF_RT_REG_SZ, nullptr, nullptr, &pathLengthInBytes) == ERROR_SUCCESS)
      {
        path.SetCount(pathLengthInBytes / sizeof(wchar_t));
        if (RegGetValueW(hInstalledRoots, nullptr, L"KitsRoot10", RRF_RT_REG_SZ, nullptr, path.GetData(), &pathLengthInBytes) == ERROR_SUCCESS)
        {
          ezStringBuilder windowsSdkBinPath;
          windowsSdkBinPath = ezStringWChar(path.GetData());
          windowsSdkBinPath.MakeCleanPath();
          windowsSdkBinPath.AppendPath("bin");

          ezDynamicArray<ezFileStats> folders;
          ezOSFile::GatherAllItemsInFolder(folders, windowsSdkBinPath, ezFileSystemIteratorFlags::ReportFolders);

          folders.Sort([](const ezFileStats& a, const ezFileStats& b)
            { return a.m_sName > b.m_sName; });

          for (const ezFileStats& folder : folders)
          {
            if (!folder.m_sName.StartsWith("10."))
            {
              continue;
            }
            rcPath = windowsSdkBinPath;
            rcPath.AppendPath(folder.m_sName);
            rcPath.AppendPath("x64/rc.exe");
            if (ezOSFile::ExistsFile(rcPath))
            {
              break;
            }
            rcPath.Clear();
          }
        }
      }
    }
    if (!rcPath.IsEmpty())
    {
      preferences->m_CompilerPreferences.m_sRcCompiler = rcPath;
    }
  }

  ezString clangVersion;

  wchar_t* pProgramFiles = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, nullptr, &pProgramFiles)))
  {
    ezStringBuilder clangDefaultPath;
    clangDefaultPath = ezStringWChar(pProgramFiles);
    CoTaskMemFree(pProgramFiles);
    pProgramFiles = nullptr;

    clangDefaultPath.AppendPath("LLVM/bin/clang.exe");
    clangDefaultPath.MakeCleanPath();
    ezStringBuilder clangCppDefaultPath = clangDefaultPath;
    clangCppDefaultPath.ReplaceLast(".exe", "++.exe");

    ezStringView clangMajorSdkVersion = EZ_PP_STRINGIFY(__clang_major__) ".";
    if (TestCompilerExecutable(clangDefaultPath, &clangVersion).Succeeded() && TestCompilerExecutable(clangCppDefaultPath).Succeeded() && clangVersion.StartsWith(clangMajorSdkVersion))
    {
      ezStringBuilder clangNiceName;
      clangNiceName.SetFormat("Clang (system default = {})", clangVersion);
      s_MachineSpecificCompilers.PushBack({clangNiceName, ezCompiler::Clang, clangDefaultPath, clangCppDefaultPath, false});
    }
  }
#  endif
#endif


#if EZ_ENABLED(EZ_PLATFORM_LINUX)
  AddCompilerVersions(s_MachineSpecificCompilers, ezCppProject::GetSdkCompiler(), ezCppProject::GetSdkCompilerMajorVersion());
#endif

#if EZ_ENABLED(EZ_COMPILER_CLANG)
  s_MachineSpecificCompilers.PushBack({"Clang (Custom)", ezCompiler::Clang, "", "", true});
#endif

#if EZ_ENABLED(EZ_PLATFORM_LINUX) && EZ_ENABLED(EZ_COMPILER_GCC)
  s_MachineSpecificCompilers.PushBack({"Gcc (Custom)", ezCompiler::Gcc, "", "", true});
#endif

  if (preferences->m_CompilerPreferences.m_Compiler != sdkCompiler)
  {
    ezStringBuilder incompatibleCompilerName = u8"⚠ ";
    incompatibleCompilerName.SetFormat(u8"⚠ {} (incompatible)", ezCppProject::CompilerToString(preferences->m_CompilerPreferences.m_Compiler));
    s_MachineSpecificCompilers.PushBack(
      {incompatibleCompilerName,
        preferences->m_CompilerPreferences.m_Compiler,
        preferences->m_CompilerPreferences.m_sCCompiler,
        preferences->m_CompilerPreferences.m_sCppCompiler,
        preferences->m_CompilerPreferences.m_bCustomCompiler});
  }
}

ezResult ezCppProject::ForceSdkCompatibleCompiler()
{
  ezCppProject* preferences = ezPreferences::QueryPreferences<ezCppProject>();

  ezCompiler::Enum sdkCompiler = GetSdkCompiler();
  for (auto& compiler : s_MachineSpecificCompilers)
  {
    if (!compiler.m_bIsCustom && compiler.m_Compiler == sdkCompiler)
    {
      preferences->m_CompilerPreferences.m_Compiler = sdkCompiler;
      preferences->m_CompilerPreferences.m_sCCompiler = compiler.m_sCCompiler;
      preferences->m_CompilerPreferences.m_sCppCompiler = compiler.m_sCppCompiler;
      preferences->m_CompilerPreferences.m_bCustomCompiler = compiler.m_bIsCustom;

      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

ezCppProject::~ezCppProject() = default;
