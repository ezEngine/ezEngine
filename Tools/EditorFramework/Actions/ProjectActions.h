#pragma once

#include <EditorFramework/Plugin.h>
#include <ToolsFoundation/Project/EditorProject.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class EZ_EDITORFRAMEWORK_DLL ezProjectActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping);

  static ezActionDescriptorHandle s_hEditorMenu;
  static ezActionDescriptorHandle s_hCreateDocument;
  static ezActionDescriptorHandle s_hOpenDocument;
  static ezActionDescriptorHandle s_hRecentDocuments;
  static ezActionDescriptorHandle s_hCreateProject;
  static ezActionDescriptorHandle s_hOpenProject;
  static ezActionDescriptorHandle s_hRecentProjects;
  static ezActionDescriptorHandle s_hCloseProject;
  static ezActionDescriptorHandle s_hProjectSettings;
};

///
class EZ_EDITORFRAMEWORK_DLL ezRecentDocumentsMenuAction : public ezLRUMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRecentDocumentsMenuAction);
public:
  ezRecentDocumentsMenuAction(const ezActionContext& context, const char* szName) : ezLRUMenuAction(context, szName) {}
  virtual void GetEntries(ezHybridArray<std::pair<ezString, ezVariant>, 16>& out_Entries) override;
  virtual void Execute(const ezVariant& value) override;
};

///
class EZ_EDITORFRAMEWORK_DLL ezRecentProjectsMenuAction : public ezLRUMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRecentProjectsMenuAction);
public:
  ezRecentProjectsMenuAction(const ezActionContext& context, const char* szName) : ezLRUMenuAction(context, szName) {}
  virtual void GetEntries(ezHybridArray<std::pair<ezString, ezVariant>, 16>& out_Entries) override;
  virtual void Execute(const ezVariant& value) override;
};

///
class EZ_EDITORFRAMEWORK_DLL ezProjectAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProjectAction);
public:
  enum class ButtonType
  {
    CreateDocument,
    OpenDocument,
    CreateProject,
    OpenProject,
    CloseProject,
    ProjectSettings
  };

  ezProjectAction(const ezActionContext& context, const char* szName, ButtonType button);
  ~ezProjectAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void ProjectEventHandler(const ezToolsProject::Event& e);

  ButtonType m_ButtonType;
};