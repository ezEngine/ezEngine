#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <EditorEngineProcessFramework/Actors/EditorWnd/ActorDeviceRenderOutputEditorWnd.h>
#include <Core/Actor/Actor.h>

class ezWindow;
class ezEditorProcessViewWindow;

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezActorEditorWnd : public ezActor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorEditorWnd, ezActor);

public:
  ezEditorProcessViewWindow* GetWindow();

private: // functions called directly by ezActorManagerEditorWnd
  friend class ezActorManagerEditorWnd;

  ezActorEditorWnd(const char* szActorName, const char* szGroupName, ezUniquePtr<ezEditorProcessViewWindow>&& pWindow);

protected:
  virtual void Activate() override;
  virtual void Deactivate() override;
  virtual void Update() override;

  ezUniquePtr<ezEditorProcessViewWindow> m_pWindow;
  ezUniquePtr<ezWindowOutputTargetGAL> m_OutputTarget;
};
