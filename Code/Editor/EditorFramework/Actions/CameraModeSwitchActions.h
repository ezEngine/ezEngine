#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

class EZ_EDITORFRAMEWORK_DLL ezCameraModeSwitchActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  /// Maps the camera mode dropdown to the toolbar identified by \a szMapping.
  static void MapToolbarActions(const char* szMapping);

  static ezActionDescriptorHandle s_hCameraMode;
};

class EZ_EDITORFRAMEWORK_DLL ezCameraModeSwitchAction : public ezDynamicMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCameraModeSwitchAction, ezDynamicMenuAction);

public:
  ezCameraModeSwitchAction(const ezActionContext& context, const char* szName, const char* szIconPath);
  virtual void GetEntries(ezDynamicArray<Item>& out_entries) override;
  virtual void Execute(const ezVariant& value) override;
};
