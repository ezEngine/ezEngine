#pragma once

#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Textures/Texture2DResource.h>

class ezSceneContext;

class ezEditorShapeIconsExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorShapeIconsExtractor, ezExtractor);
public:
  ezEditorShapeIconsExtractor(const char* szName = "EditorShapeIconsExtractor");
  ~ezEditorShapeIconsExtractor();

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
    ezExtractedRenderData* pExtractedRenderData) override;

  void SetSceneContext(ezSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  ezSceneContext* GetSceneContext() const { return m_pSceneContext; }

private:
  void ExtractShapeIcon(const ezGameObject* pObject, const ezView& view, ezExtractedRenderData* pExtractedRenderData, ezRenderData::Category category);
  const ezTypedMemberProperty<ezColor>* FindColorProperty(const ezRTTI* pRtti) const;
  const ezTypedMemberProperty<ezColorGammaUB>* FindColorGammaProperty(const ezRTTI* pRtti) const;
  void FillShapeIconInfo();

  float m_fSize;
  float m_fMaxScreenSize;
  ezSceneContext* m_pSceneContext;

  struct ShapeIconInfo
  {
    ezTexture2DResourceHandle m_hTexture;
    const ezTypedMemberProperty<ezColor>* m_pColorProperty;
    const ezTypedMemberProperty<ezColorGammaUB>* m_pColorGammaProperty;
  };

  ezHashTable<const ezRTTI*, ShapeIconInfo> m_ShapeIconInfos;
};
