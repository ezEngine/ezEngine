#include <EditorPluginScenePCH.h>

#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScene2Document, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezScene2Document::ezScene2Document(const char* szDocumentPath)
  : ezSceneDocument(szDocumentPath, ezSceneDocument::DocumentType::Scene)
{
  m_LayerSelection.SetOwner(this);
}

ezScene2Document::~ezScene2Document() = default;

void ezScene2Document::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  //#TODO: Test code to add some layers
  auto pRoot = this->GetObjectManager()->GetObject(GetSettingsObject()->GetGuid());
  if (pRoot->GetChildren().IsEmpty())
  {
    ezDocumentObject* pObject = this->GetObjectManager()->CreateObject(ezGetStaticRTTI<ezSceneLayer>());
    this->GetObjectManager()->AddObject(pObject, pRoot, "Layers", 0);
    ezDocumentObject* pObject2 = this->GetObjectManager()->CreateObject(ezGetStaticRTTI<ezSceneLayer>());
    this->GetObjectManager()->AddObject(pObject2, pRoot, "Layers", 1);
  }
}
