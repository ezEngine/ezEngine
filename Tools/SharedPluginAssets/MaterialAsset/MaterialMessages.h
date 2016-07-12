#pragma once

#include <SharedPluginAssets/Plugin.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>


class EZ_SHAREDPLUGINASSETS_DLL ezEditorEngineMaterialUpdateMsg : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineMaterialUpdateMsg, ezEditorEngineDocumentMsg);

public:
  ezEditorEngineMaterialUpdateMsg()
  {
  }

  ezDataBuffer m_Data;
};
