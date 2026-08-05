#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>

struct ezPluginBundleSet;

/// What to create, as gathered by the 'Create Project' wizard or from the command line.
///
/// The two template kinds are unrelated and must not be confused: a *project* template is an entire
/// project folder that gets copied, a *plugin* template only decides which plugin bundles are enabled.
/// A project template brings its own 'Editor/PluginSelection.ddl', so m_sPluginTemplate is ignored
/// when m_sProjectTemplate is set - which is also what the wizard does (it skips its plugin page).
struct EZ_EDITORFRAMEWORK_DLL ezProjectCreationOptions
{
  /// Absolute path to the folder to create. It must not exist yet, or must be empty.
  ezString m_sTargetDirectory;

  /// Name of a project template as returned by ezProjectCreation::FindProjectTemplates(), or empty for a blank project.
  ezString m_sProjectTemplate;

  /// Name of a plugin template (see ezPluginBundle::m_EnabledInTemplates), e.g. 'General3D'. Only used for blank projects.
  ///
  /// When this is empty, the passed in ezPluginBundleSet is written out unchanged - which is what the wizard needs,
  /// since its plugin page has already put the user's selection into that set.
  ezString m_sPluginTemplate;
};

/// Creating a project without the wizard.
///
/// This is what ezQtCreateProjectDlg executes once its pages are filled in, and what the '-createProject'
/// command line option calls. It only writes the project's files - opening the result is the caller's job.
struct EZ_EDITORFRAMEWORK_DLL ezProjectCreation
{
  /// Names of the project templates found in '<appdata>/ProjectTemplates', ie. every sub-folder containing an 'ezProject' file.
  ///
  /// The built-in 'blank project' is not part of this list, it is represented by an empty template name.
  static void FindProjectTemplates(ezDynamicArray<ezString>& out_templateNames);

  /// Returns the path to the 'ezProject' file of the named project template, or EZ_FAILURE if there is no such template.
  static ezResult FindProjectTemplate(ezStringView sTemplateName, ezStringBuilder& out_sProjectFile);

  /// Names of the plugin templates that the available plugin bundles reference.
  ///
  /// Requires the plugin bundles to have been detected already (ezQtEditorApp::DetectAvailablePluginBundles()),
  /// otherwise the list comes back empty.
  static void FindPluginTemplates(const ezPluginBundleSet& pluginBundles, ezDynamicArray<ezString>& out_templateNames);

  /// Creates the project folder and everything the chosen template implies.
  ///
  /// For a blank project that is only 'Editor/PluginSelection.ddl'; the 'ezProject' file itself is written when the
  /// project is opened for the first time (ezToolsProject::CreateProject()). For a project template the template folder
  /// is copied as it is, minus its 'AssetCache', so that the new project starts out with no transformed assets.
  ///
  /// Fails when the target directory already contains files, so that an existing project cannot be overwritten.
  static ezStatus CreateProject(const ezProjectCreationOptions& options, const ezPluginBundleSet& pluginBundles);
};
