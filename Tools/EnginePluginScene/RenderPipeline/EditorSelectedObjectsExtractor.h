#pragma once

#include <RendererCore/Pipeline/Extractor.h>

class ezSceneContext;

class ezEditorSelectedObjectsExtractor : public ezSelectedObjectsExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorSelectedObjectsExtractor, ezSelectedObjectsExtractor);
public:
  ezEditorSelectedObjectsExtractor();

  virtual const ezDeque<ezGameObjectHandle>* GetSelection() override;

  void SetSceneContext(ezSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  ezSceneContext* GetSceneContext() const { return m_pSceneContext; }

private:
  ezSceneContext* m_pSceneContext;
};
