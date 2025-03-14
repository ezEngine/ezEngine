#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <SharedPluginScene/Common/Messages.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLayerDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezLayerDocument::ezLayerDocument(ezStringView sDocumentPath, ezScene2Document* pParentScene)
  : ezSceneDocument(sDocumentPath, ezSceneDocument::DocumentType::Layer)
{
  m_pHostDocument = pParentScene;
}

ezLayerDocument::~ezLayerDocument() = default;

void ezLayerDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
}

ezVariant ezLayerDocument::GetCreateEngineMetaData() const
{
  return m_pHostDocument->GetGuid();
}
