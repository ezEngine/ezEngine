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
