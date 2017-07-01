#pragma once

#include <EditorEngineProcessFramework/Plugin.h>
#include <Foundation/Configuration/Singleton.h>

enum class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEditorEngineProcessMode
{
  Primary,
  PrimaryOwnWindow,
  Remote,
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEditorEngineProcessApp
{
  EZ_DECLARE_SINGLETON(ezEditorEngineProcessApp);

public:
  ezEditorEngineProcessApp();
  ~ezEditorEngineProcessApp();

  ezEditorEngineProcessMode m_Mode = ezEditorEngineProcessMode::Primary;


};