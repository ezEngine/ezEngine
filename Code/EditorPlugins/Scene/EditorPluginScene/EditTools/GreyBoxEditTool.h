#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/Gizmos/DrawBoxGizmo.h>

struct ezGameObjectEvent;
struct ezManipulatorManagerEvent;

class EZ_EDITORPLUGINSCENE_DLL ezGreyBoxEditTool : public ezGameObjectEditTool
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGreyBoxEditTool, ezGameObjectEditTool);

public:
  ezGreyBoxEditTool();
  ~ezGreyBoxEditTool();

  virtual ezEditorInputContext* GetEditorInputContextOverride();
  virtual ezEditToolSupportedSpaces GetSupportedSpaces() const override;
  virtual bool GetSupportsMoveParentOnly() const override;
  virtual void GetGridSettings(ezGridSettingsMsgToEngine& outGridSettings) override;

protected:
  virtual void OnConfigured() override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  void UpdateGizmoState();
  void GameObjectEventHandler(const ezGameObjectEvent& e);
  void ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e);
  void GizmoEventHandler(const ezGizmoEvent& e);

  ezDrawBoxGizmo m_DrawBoxGizmo;
};

