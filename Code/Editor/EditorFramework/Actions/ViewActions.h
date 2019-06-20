#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class EZ_EDITORFRAMEWORK_DLL ezViewActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  enum Flags
  {
    PerspectiveMode = EZ_BIT(0),
    RenderMode = EZ_BIT(1),
    UsageHint = EZ_BIT(2),
    ActivateRemoteProcess = EZ_BIT(3),
  };

  static void MapActions(const char* szMapping, const char* szPath, ezUInt32 flags);

  static ezActionDescriptorHandle s_hRenderMode;
  static ezActionDescriptorHandle s_hPerspective;
  static ezActionDescriptorHandle s_hCameraUsageHint;
  static ezActionDescriptorHandle s_hActivateRemoteProcess;
  static ezActionDescriptorHandle s_hLinkDeviceCamera;
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

///
class EZ_EDITORFRAMEWORK_DLL ezCameraUsageHintAction : public ezEnumerationMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCameraUsageHintAction, ezEnumerationMenuAction);
public:
  ezCameraUsageHintAction(const ezActionContext& context, const char* szName, const char* szIconPath);
  virtual ezInt64 GetValue() const override;
  virtual void Execute(const ezVariant& value) override;
};

class EZ_EDITORFRAMEWORK_DLL ezViewAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewAction, ezButtonAction);
public:
  enum class ButtonType
  {
    ActivateRemoteProcess,
    LinkDeviceCamera,
  };

  ezViewAction(const ezActionContext& context, const char* szName, ButtonType button);
  ~ezViewAction();

  virtual void Execute(const ezVariant& value) override;

private:
  ButtonType m_ButtonType;
};
