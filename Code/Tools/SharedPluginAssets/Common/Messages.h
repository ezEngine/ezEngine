#pragma once

#include <SharedPluginAssets/SharedPluginAssetsDLL.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>


class EZ_SHAREDPLUGINASSETS_DLL ezEditorEngineRestoreResourceMsg : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineRestoreResourceMsg, ezEditorEngineDocumentMsg);

public:

};

class EZ_SHAREDPLUGINASSETS_DLL ezEditorEngineResourceUpdateMsg : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineResourceUpdateMsg, ezEditorEngineDocumentMsg);

public:
  ezEditorEngineResourceUpdateMsg()
  {
  }

  ezString m_sResourceType;
  ezDataBuffer m_Data;
};

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
