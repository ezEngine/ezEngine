#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>

struct ezGameObjectEvent;
struct ezManipulatorManagerEvent;

class EZ_EDITORPLUGINSCENE_DLL ezGreyBoxEditTool : public ezGameObjectEditTool
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGreyBoxEditTool, ezGameObjectEditTool);

public:
  ezGreyBoxEditTool();
  ~ezGreyBoxEditTool();

  virtual ezEditToolSupportedSpaces GetSupportedSpaces() const override;
  virtual bool GetSupportsMoveParentOnly() const override;

protected:
  virtual void OnConfigured() override;

private:
  void UpdateGizmoState();
  void GameObjectEventHandler(const ezGameObjectEvent& e);
  void ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e);

  ezTranslateGizmo m_TranslateGizmo;
};

