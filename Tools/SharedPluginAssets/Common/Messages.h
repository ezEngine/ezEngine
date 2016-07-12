#pragma once

#include <SharedPluginAssets/Plugin.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>


class EZ_SHAREDPLUGINASSETS_DLL ezEditorEngineRestoreResourceMsg : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineRestoreResourceMsg, ezEditorEngineDocumentMsg);

public:

};
