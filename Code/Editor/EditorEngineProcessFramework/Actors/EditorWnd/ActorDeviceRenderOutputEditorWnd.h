#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Core/ActorDevices/ActorDeviceRenderOutput.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>

class ezWindowOutputTargetGAL;

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezActorDeviceRenderOutputEditorWnd : public ezActorDeviceRenderOutput
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorDeviceRenderOutputEditorWnd, ezActorDeviceRenderOutput);

public:
  ezActorDeviceRenderOutputEditorWnd(ezWindowOutputTargetGAL* pOutputTarget);
  ~ezActorDeviceRenderOutputEditorWnd();

  virtual ezWindowOutputTargetGAL* GetWindowOutputTarget() const override;

  virtual void Present() override;

protected:
  ezWindowOutputTargetGAL* m_pOutputTarget = nullptr;
};
