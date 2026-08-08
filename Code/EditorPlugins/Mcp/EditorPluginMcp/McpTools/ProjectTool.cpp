#include <EditorPluginMcp/EditorPluginMcpPCH.h>

#include <EditorPluginMcp/McpTools/ProjectTool.h>
#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>

#include <Foundation/Configuration/Plugin.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Project/ProjectExport.h>
#include <ToolsFoundation/Project/ToolsProject.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpProjectTool, 1, ezRTTIDefaultAllocator<ezMcpProjectTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezMcpProjectTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
  desc.m_sName = "project_info";
  desc.m_sDescription = "Returns which project the editor currently has open: its name, the project directory, the configured "
                        "data directories, the asset profiles and the loaded plugins. Call this first - paths returned by other "
                        "tools are relative to these data directories, asset transform states depend on the active profile, and "
                        "the loaded plugins determine which reflected types exist.";
  desc.m_sInputSchema = R"({"type":"object","properties":{}})";

  ezMcpToolDesc& exp = out_tools.ExpandAndGetRef();
  exp.m_sName = "project_export";
  exp.m_sDescription = "Exports the open project into a standalone, runnable directory - the same operation as the editor's "
                       "'Export Project' dialog, which cannot be used through action_execute. Builds the project's C++ plugin, "
                       "transforms all assets and then writes the runtime binaries, the transformed assets and launch scripts "
                       "to the target directory. Both preparation steps can be turned off, but only do that when you know they "
                       "have just been done - otherwise the export contains stale or missing assets.\n"
                       "IMPORTANT - the target directory is DELETED before the export, so never point this at a directory that "
                       "holds anything else, in particular not at the project directory itself.\n"
                       "This runs for a long time: transforming the assets of a project that was never transformed takes many "
                       "minutes, during which the editor answers nothing else. Use a generous timeout - 30 minutes rather than "
                       "30 seconds - and see app_ping for telling a slow export apart from a hung editor. The export log is "
                       "returned and also written to 'ExportLog.txt' in the target directory.";
  exp.m_sInputSchema = R"({"type":"object","properties":{)"
                       R"("targetDirectory":{"type":"string","description":"Absolute path of the directory to export into. It is deleted and recreated. Must not be inside the project directory."},)"
                       R"("compileCppPlugin":{"type":"boolean","description":"Build the project's C++ plugin first, so the exported binaries match the current code. Default true. Has no effect on a project without C++ code."},)"
                       R"("transformAssets":{"type":"boolean","description":"Transform all assets first. Default true. With this off, assets modified since the last transform are exported in their old state, and assets never transformed for the active profile are missing entirely."},)"
                       R"("createLaunchScripts":{"type":"boolean","description":"Write .bat files for starting the exported project. Default true."}},)"
                       R"("required":["targetDirectory"]})";
}

void ezMcpProjectTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  if (sToolName == "project_info")
  {
    ExecuteProjectInfo(arguments, out_result);
  }
  else if (sToolName == "project_export")
  {
    ExecuteProjectExport(arguments, out_result);
  }
}

void ezMcpProjectTool::ExecuteProjectInfo(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezMcpJsonWriter writer;
  writer.BeginObject();

  // Reported explicitly rather than as an error, because "no project open" is a legitimate state that
  // the agent has to be able to observe - it explains why every other tool returns nothing.
  const bool bProjectOpen = ezToolsProject::IsProjectOpen();
  writer.AddVariableBool("projectOpen", bProjectOpen);

  if (!bProjectOpen)
  {
    writer.EndObject();
    out_result.m_sText = writer.GetResult();
    return;
  }

  const ezToolsProject* pProject = ezToolsProject::GetSingleton();

  writer.AddVariableString("projectName", pProject->GetProjectName(false));
  writer.AddVariableString("projectDirectory", pProject->GetProjectDirectory());
  writer.AddVariableString("projectFile", pProject->GetProjectFile());
  writer.AddVariableString("projectDataFolder", pProject->GetProjectDataFolder());

  // The configured directories, not the mounted ones: these are what asset paths are expressed
  // relative to, and the root name is the '>rootname/...' prefix that shows up in those paths.
  writer.BeginArray("dataDirectories");
  for (const auto& dd : ezQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs)
  {
    writer.BeginObject();
    writer.AddVariableString("path", dd.m_sDataDirSpecialPath);
    writer.AddVariableString("rootName", dd.m_sRootName);
    writer.AddVariableBool("writable", dd.m_bWritable);
    // set for directories the project depends on, which the user cannot remove in the settings dialog
    writer.AddVariableBool("hardCodedDependency", dd.m_bHardCodedDependency);
    writer.EndObject();
  }
  writer.EndArray();

  if (const ezAssetCurator* pCurator = ezAssetCurator::GetSingleton())
  {
    if (const ezPlatformProfile* pActive = pCurator->GetActiveAssetProfile())
    {
      writer.AddVariableString("activeAssetProfile", pActive->GetConfigName());
    }

    writer.BeginArray("assetProfiles");
    for (ezUInt32 i = 0; i < pCurator->GetNumAssetProfiles(); ++i)
    {
      if (const ezPlatformProfile* pProfile = pCurator->GetAssetProfile(i))
      {
        writer.WriteString(pProfile->GetConfigName());
      }
    }
    writer.EndArray();
  }

  // Which plugins are loaded determines which reflected types exist at all, so this is the companion
  // to the 'plugin' field that rtti_type_info returns for a type.
  ezDynamicArray<ezPlugin::PluginInfo> pluginInfos;
  ezPlugin::GetAllPluginInfos(pluginInfos);

  writer.BeginArray("loadedPlugins");
  for (const ezPlugin::PluginInfo& info : pluginInfos)
  {
    writer.BeginObject();
    writer.AddVariableString("name", info.m_sName);

    writer.BeginArray("dependencies");
    for (const ezString& sDep : info.m_sDependencies)
    {
      writer.WriteString(sDep);
    }
    writer.EndArray();

    writer.EndObject();
  }
  writer.EndArray();

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpProjectTool::ExecuteProjectExport(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  if (!ezToolsProject::IsProjectOpen())
  {
    out_result.SetError("No project is open, so there is nothing to export.");
    return;
  }

  ezStringBuilder sTargetDir = ezMcpJson::GetString(arguments, "targetDirectory");
  sTargetDir.MakeCleanPath();
  sTargetDir.Trim("", "/");

  if (sTargetDir.IsEmpty())
  {
    out_result.SetError("No 'targetDirectory' given.");
    return;
  }

  if (!sTargetDir.IsAbsolutePath())
  {
    out_result.SetError(ezStringBuilder("'", sTargetDir, "' is not an absolute path. The export target is not resolved against "
                                                         "the project or any data directory, so it has to be a full path."));
    return;
  }

  // The export clears the target directory first, so an unlucky path would delete the project. Checked
  // here rather than deeper down because this is the only caller whose argument comes from a model that
  // has never seen the folder it is naming.
  ezStringBuilder sProjectDir = ezToolsProject::GetSingleton()->GetProjectDirectory();
  sProjectDir.MakeCleanPath();
  sProjectDir.Trim("", "/");

  if (ezPathUtils::IsSubPath_NoCase(sTargetDir, sProjectDir) || ezPathUtils::IsSubPath_NoCase(sProjectDir, sTargetDir))
  {
    out_result.SetError(ezStringBuilder("Refusing to export to '", sTargetDir, "', because it overlaps with the project directory '",
      sProjectDir, "'. The target directory is deleted before the export, which would destroy the project. Pick a directory outside of it."));
    return;
  }

  ezProjectExportOptions options;
  options.m_bCompileCppPlugin = ezMcpJson::GetBool(arguments, "compileCppPlugin", true);
  options.m_bTransformAssets = ezMcpJson::GetBool(arguments, "transformAssets", true);
  options.m_bCreateLaunchScripts = ezMcpJson::GetBool(arguments, "createLaunchScripts", true);

  ezStringBuilder sLog;
  const ezStatus status = ezProjectExport::ExportProjectComplete(sTargetDir, options, &sLog);

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableBool("exported", status.Succeeded());
  writer.AddVariableString("targetDirectory", sTargetDir);

  if (status.Failed())
  {
    writer.AddVariableString("error", status.GetMessageString());
  }

  // The whole log, not a summary: it is the only record of what was written and which assets failed, and
  // an export that "succeeded" can still have skipped things that only show up here.
  writer.AddVariableString("log", sLog);

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
  out_result.m_bIsError = status.Failed();
}
