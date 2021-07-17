#pragma once

#include <EditorPluginScene/Scene/SceneDocument.h>

class ezScene2Document;

class ezLayerDocument : public ezSceneDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLayerDocument, ezSceneDocument);

public:
  ezLayerDocument(const char* szDocumentPath, ezScene2Document* pParentScene);
  ~ezLayerDocument();

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void InitializeAfterLoadingAndSaving() override;

  virtual ezVariant GetCreateEngineMetaData() const override;
};
