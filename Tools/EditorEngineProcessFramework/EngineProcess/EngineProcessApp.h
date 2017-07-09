#pragma once

#include <EditorEngineProcessFramework/Plugin.h>
#include <Foundation/Configuration/Singleton.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <System/Window/Window.h>
#include <Foundation/Types/UniquePtr.h>

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

  ezViewHandle CreateRemoteWindowAndView();
  void DestroyRemoteWindow();

private:
  void CreateRemoteWindow();

  ezEditorEngineProcessMode m_Mode = ezEditorEngineProcessMode::Primary;

  ezUniquePtr<ezRemoteProcessWindow> m_pRemoteWindow;
  ezViewHandle m_hRemoteView;
};