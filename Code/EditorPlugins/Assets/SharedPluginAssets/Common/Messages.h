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

class EZ_SHAREDPLUGINASSETS_DLL ezEditorEngineSetMaterialsMsg : public ezEditorEngineDocumentMsg
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineSetMaterialsMsg, ezEditorEngineDocumentMsg);

public:
  ezHybridArray<ezString, 16> m_Materials;

  /// \brief Human-readable display name for each material slot (e.g. the material asset filename).
  /// Used by asset previews to show which material is under the cursor.
  ezHybridArray<ezString, 16> m_SlotNames;
};
