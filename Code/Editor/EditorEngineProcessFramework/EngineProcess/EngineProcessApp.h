#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/System/Window.h>
#include <Core/System/WindowManager.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/Declarations.h>


using ezRenderPipelineResourceHandle = ezTypedResourceHandle<class ezRenderPipelineResource>;

enum class ezEditorEngineProcessMode
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
  void DestroyRemoteWindow();

  virtual ezRenderPipelineResourceHandle CreateDefaultMainRenderPipeline();
  virtual ezRenderPipelineResourceHandle CreateDefaultDebugRenderPipeline();

protected:
  virtual void CreateRemoteWindow();

  ezEditorEngineProcessMode m_Mode = ezEditorEngineProcessMode::Primary;

  ezRegisteredWndHandle m_hWindow;
  ezViewHandle m_hRemoteView;
};
