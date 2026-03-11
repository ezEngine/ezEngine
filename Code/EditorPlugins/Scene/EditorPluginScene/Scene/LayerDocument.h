#pragma once

#include <EditorPluginScene/Scene/SceneDocument.h>

class ezScene2Document;

class EZ_EDITORPLUGINSCENE_DLL ezLayerDocument : public ezSceneDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLayerDocument, ezSceneDocument);

public:
  ezLayerDocument(ezStringView sDocumentPath, ezScene2Document* pParentScene);
  ~ezLayerDocument();

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual ezVariant GetCreateEngineMetaData() const override;
};
