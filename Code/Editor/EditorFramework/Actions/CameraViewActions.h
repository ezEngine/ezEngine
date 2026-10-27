#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

/// Actions that rotate the camera of a game object view (scene, prefab) to look along a world axis, or orbit it in fixed steps.
///
/// The default shortcuts follow Blender's numpad layout. The actions affect the view under the mouse, or the focused view, if the mouse isn't over a view.
class EZ_EDITORFRAMEWORK_DLL ezCameraViewActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(ezStringView sMapping);

  static ezActionDescriptorHandle s_hCameraViewMenu;
  static ezActionDescriptorHandle s_hViewFront;
  static ezActionDescriptorHandle s_hViewBack;
  static ezActionDescriptorHandle s_hViewRight;
  static ezActionDescriptorHandle s_hViewLeft;
  static ezActionDescriptorHandle s_hViewTop;
  static ezActionDescriptorHandle s_hViewBottom;
  static ezActionDescriptorHandle s_hFlipView;
  static ezActionDescriptorHandle s_hOrbitLeft;
  static ezActionDescriptorHandle s_hOrbitRight;
  static ezActionDescriptorHandle s_hOrbitUp;
  static ezActionDescriptorHandle s_hOrbitDown;
};

///
class EZ_EDITORFRAMEWORK_DLL ezCameraViewAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCameraViewAction, ezButtonAction);

public:
  enum class ActionType
  {
    ViewFront,
    ViewBack,
    ViewRight,
    ViewLeft,
    ViewTop,
    ViewBottom,
    FlipView,
    OrbitLeft,
    OrbitRight,
    OrbitUp,
    OrbitDown,
  };

  ezCameraViewAction(const ezActionContext& context, const char* szName, ActionType type);

  virtual void Execute(const ezVariant& value) override;

private:
  ActionType m_Type;
};
