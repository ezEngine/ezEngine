#pragma once

#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Textures/TextureResource.h>

class ezSceneContext;

class ezEditorShapeIconsExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorShapeIconsExtractor, ezExtractor);
public:
  ezEditorShapeIconsExtractor();
  ~ezEditorShapeIconsExtractor();

  virtual void Extract(const ezView& view, ezExtractedRenderData* pExtractedRenderData) override;

  void SetSceneContext(ezSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  ezSceneContext* GetSceneContext() const { return m_pSceneContext; }

private:
  void ExtractShapeIcon(const ezGameObject* pObject, const ezView& view, ezExtractedRenderData* pExtractedRenderData, ezRenderData::Category category);
  void LoadShapeIconTextures();

  float m_fSize;
  float m_fMaxScreenSize;
  ezSceneContext* m_pSceneContext;

  ezHashTable<const ezRTTI*, ezTextureResourceHandle> m_ShapeIcons;
};
