#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

class EZ_SHAREDPLUGINASSETS_DLL ezEditorEngineRestartSimulationMsg : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineRestartSimulationMsg, ezEditorEngineDocumentMsg);

public:
};

class EZ_SHAREDPLUGINASSETS_DLL ezEditorEngineLoopAnimationMsg : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineLoopAnimationMsg, ezEditorEngineDocumentMsg);

public:
  bool m_bLoop;
};
