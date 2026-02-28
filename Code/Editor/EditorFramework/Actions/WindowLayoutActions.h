#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

/// Manages window layout save/restore using ADS perspective management.
class EZ_EDITORFRAMEWORK_DLL ezWindowLayoutActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();
  static void MapActions(ezStringView sMapping);

  /// Automatically restores the default layout at startup.
  static void RestoreUserLayout();

  /// Automatically saves the current layout as default at shutdown.
  static void SaveUserLayout();

  static ezActionDescriptorHandle s_hCatWindowLayout;
  static ezActionDescriptorHandle s_hSetToDefaultPinned;
  static ezActionDescriptorHandle s_hSetToDefaultUnpinned;
  static ezActionDescriptorHandle s_hSetToAbBottom;
  static ezActionDescriptorHandle s_hSaveLayout;
  static ezActionDescriptorHandle s_hLoadLayout;
};

class EZ_EDITORFRAMEWORK_DLL ezWindowLayoutAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezWindowLayoutAction, ezButtonAction);

public:
  enum class ButtonType
  {
    SetToDefaultPinned,
    SetToDefaultUnpinned,
    SetToAbBottom,
  };

  ezWindowLayoutAction(const ezActionContext& context, const char* szName, ButtonType button);
  virtual void Execute(const ezVariant& value) override;

private:
  ButtonType m_ButtonType;
};

/// Dynamic menu listing the 3 user layout slots for saving the current panel arrangement.
class EZ_EDITORFRAMEWORK_DLL ezSaveLayoutMenuAction : public ezDynamicMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSaveLayoutMenuAction, ezDynamicMenuAction);

public:
  ezSaveLayoutMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath);
  virtual void GetEntries(ezDynamicArray<Item>& out_entries) override;
  virtual void Execute(const ezVariant& value) override;
};

/// Dynamic menu listing the 3 user layout slots for restoring a previously saved panel arrangement.
class EZ_EDITORFRAMEWORK_DLL ezLoadLayoutMenuAction : public ezDynamicMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLoadLayoutMenuAction, ezDynamicMenuAction);

public:
  ezLoadLayoutMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath);
  virtual void GetEntries(ezDynamicArray<Item>& out_entries) override;
  virtual void Execute(const ezVariant& value) override;
};
