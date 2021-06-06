#include <EditorPluginScenePCH.h>

#include <EditorPluginScene/Scene/Scene2Document.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScene2Document, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezScene2Document::ezScene2Document(const char* szDocumentPath)
  : ezSceneDocument(szDocumentPath, ezSceneDocument::DocumentType::Scene)
{
}

ezScene2Document::~ezScene2Document() = default;
