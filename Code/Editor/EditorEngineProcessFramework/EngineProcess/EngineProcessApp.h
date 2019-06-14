#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <System/Window/Window.h>

class ezActor;

typedef ezTypedResourceHandle<class ezRenderPipelineResource> ezRenderPipelineResourceHandle;

enum class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEditorEngineProcessMode
{
  Primary,
  Remote,
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezRemoteProcessWindow : public ezWindow
{
public:
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEditorEngineProcessApp
{
  EZ_DECLARE_SINGLETON(ezEditorEngineProcessApp);

public:
  ezEditorEngineProcessApp();
  ~ezEditorEngineProcessApp();

  void SetRemoteMode();

  bool IsRemoteMode() const { return m_Mode == ezEditorEngineProcessMode::Remote; }

  virtual ezViewHandle CreateRemoteWindowAndView(ezCamera* pCamera);
  virtual void DestroyRemoteWindow();

  virtual ezRenderPipelineResourceHandle CreateDefaultMainRenderPipeline();
  virtual ezRenderPipelineResourceHandle CreateDefaultDebugRenderPipeline();

protected:
  virtual void CreateRemoteWindow();

  ezEditorEngineProcessMode m_Mode = ezEditorEngineProcessMode::Primary;

  ezActor* m_pActor = nullptr;
  ezViewHandle m_hRemoteView;
};
