#pragma once

#include <EditorFramework/Plugin.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class EZ_EDITORFRAMEWORK_DLL ezViewActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hRenderMode;
  static ezActionDescriptorHandle s_hPerspective;
  static ezActionDescriptorHandle s_hRenderPipeline;
};

///
class EZ_EDITORFRAMEWORK_DLL ezRenderModeAction : public ezEnumerationMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderModeAction, ezEnumerationMenuAction);
public:
  ezRenderModeAction(const ezActionContext& context, const char* szName, const char* szIconPath);
  virtual ezInt64 GetValue() const override;
  virtual void Execute(const ezVariant& value) override;
};

///
class EZ_EDITORFRAMEWORK_DLL ezPerspectiveAction : public ezEnumerationMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPerspectiveAction, ezEnumerationMenuAction);
public:
  ezPerspectiveAction(const ezActionContext& context, const char* szName, const char* szIconPath);
  virtual ezInt64 GetValue() const override;
  virtual void Execute(const ezVariant& value) override;
};

class EZ_EDITORFRAMEWORK_DLL ezRenderPipelineMenuAction : public ezLRUMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelineMenuAction, ezLRUMenuAction);
public:
  ezRenderPipelineMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath);
  virtual void GetEntries(ezHybridArray<ezLRUMenuAction::Item, 16>& out_Entries) override;
  virtual void Execute(const ezVariant& value) override;

private:
  enum Actions
  {
    Browse,
    Default
  };
  void GetRecentRenderPipelines(ezHybridArray<ezString, 10>& list);
  void AddToRecentRenderPipelines(const ezString& entry);
};