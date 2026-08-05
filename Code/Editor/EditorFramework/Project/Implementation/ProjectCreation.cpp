#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/Project/ProjectCreation.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

namespace
{
  ezString GetProjectTemplatesFolder()
  {
    ezStringBuilder sFolder = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
    sFolder.AppendPath("ProjectTemplates");
    return sFolder;
  }
} // namespace

void ezProjectCreation::FindProjectTemplates(ezDynamicArray<ezString>& out_templateNames)
{
  out_templateNames.Clear();

  ezFileSystemIterator fsIt;
  fsIt.StartSearch(GetProjectTemplatesFolder(), ezFileSystemIteratorFlags::ReportFolders);

  ezStringBuilder path;

  while (fsIt.IsValid())
  {
    fsIt.GetStats().GetFullPath(path);
    path.AppendPath("ezProject");

    if (ezOSFile::ExistsFile(path))
    {
      out_templateNames.PushBack(fsIt.GetStats().m_sName);
    }

    fsIt.Next();
  }
}

ezResult ezProjectCreation::FindProjectTemplate(ezStringView sTemplateName, ezStringBuilder& out_sProjectFile)
{
  if (sTemplateName.IsEmpty())
    return EZ_FAILURE;

  out_sProjectFile = GetProjectTemplatesFolder();
  out_sProjectFile.AppendPath(sTemplateName);
  out_sProjectFile.AppendPath("ezProject");

  if (!ezOSFile::ExistsFile(out_sProjectFile))
  {
    out_sProjectFile.Clear();
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezProjectCreation::FindPluginTemplates(const ezPluginBundleSet& pluginBundles, ezDynamicArray<ezString>& out_templateNames)
{
  out_templateNames.Clear();

  for (auto it : pluginBundles.m_Plugins)
  {
    for (const ezString& sTemplate : it.Value().m_EnabledInTemplates)
    {
      if (!out_templateNames.Contains(sTemplate))
      {
        out_templateNames.PushBack(sTemplate);
      }
    }
  }
}

ezStatus ezProjectCreation::CreateProject(const ezProjectCreationOptions& options, const ezPluginBundleSet& pluginBundles)
{
  ezStringBuilder sTargetDir = options.m_sTargetDirectory;
  sTargetDir.MakeCleanPath();

  if (sTargetDir.IsEmpty() || !ezPathUtils::IsAbsolutePath(sTargetDir))
    return ezStatus(ezFmt("The project path '{}' is not an absolute path.", sTargetDir));

  if (ezOSFile::ExistsDirectory(sTargetDir))
  {
    // an existing but empty folder is fine - a folder with files in it might be a project, and creating
    // one on top of it would mix two projects into one
    ezFileSystemIterator fsIt;
    fsIt.StartSearch(sTargetDir, ezFileSystemIteratorFlags::ReportFilesAndFoldersRecursive);

    if (fsIt.IsValid())
      return ezStatus(ezFmt("The directory '{}' already exists and is not empty.", sTargetDir));
  }

  // resolve the template before anything is written, so that a bad name does not leave a folder behind
  ezStringBuilder sTemplateProjectFile;

  if (!options.m_sProjectTemplate.IsEmpty() && FindProjectTemplate(options.m_sProjectTemplate, sTemplateProjectFile).Failed())
  {
    ezStringBuilder sAvailable;
    ezDynamicArray<ezString> templates;
    FindProjectTemplates(templates);

    for (const ezString& sName : templates)
    {
      sAvailable.AppendWithSeparator(", ", "'", sName, "'");
    }

    if (sAvailable.IsEmpty())
      sAvailable = "<none>";

    return ezStatus(ezFmt("There is no project template called '{}'. Available templates: {}", options.m_sProjectTemplate, sAvailable));
  }

  if (ezOSFile::CreateDirectoryStructure(sTargetDir).Failed())
    return ezStatus(ezFmt("Failed to create the directory '{}'.", sTargetDir));

  if (options.m_sProjectTemplate.IsEmpty())
  {
    ezPluginBundleSet localSet = pluginBundles;

    if (!options.m_sPluginTemplate.IsEmpty())
    {
      localSet.SetFromTemplate(options.m_sPluginTemplate);
    }

    ezStringBuilder sPluginSelection = sTargetDir;
    sPluginSelection.AppendPath("Editor/PluginSelection.ddl");

    ezFileWriter file;
    if (file.Open(sPluginSelection).Failed())
      return ezStatus(ezFmt("Failed to write the plugin selection to '{}'.", sPluginSelection));

    ezOpenDdlWriter ddl;
    ddl.SetOutputStream(&file);

    localSet.WriteStateToDDL(ddl);
  }
  else
  {
    ezStringBuilder sSrcFolder = sTemplateProjectFile;
    sSrcFolder.PathParentDirectory();

    if (ezOSFile::CopyFolder(sSrcFolder, sTargetDir).Failed())
      return ezStatus(ezFmt("Failed to copy the project template from '{}' to '{}'.", sSrcFolder, sTargetDir));

    // in case the template folder contained an AssetCache, delete it, so that the new project starts
    // out with no transformed assets, rather than with outputs of unknown age
    ezStringBuilder sAssetCache(sTargetDir, "/AssetCache");
    if (ezOSFile::ExistsDirectory(sAssetCache) && ezOSFile::DeleteFolder(sAssetCache).Failed())
      return ezStatus(ezFmt("Failed to delete the copied asset cache '{}'.", sAssetCache));
  }

  return ezStatus(EZ_SUCCESS);
}
