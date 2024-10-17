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

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData) override;
  virtual void PostSortAndBatch(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData) override {}

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  void SetSceneContext(ezSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  ezSceneContext* GetSceneContext() const { return m_pSceneContext; }

private:
  void ExtractShapeIcon(const ezGameObject* pObject, const ezView& view, ezExtractedRenderData& extractedRenderData, ezRenderData::Category category);
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
    ezColor m_FallbackColor = ezColor::White;
  };

  ezHashTable<const ezRTTI*, ShapeIconInfo> m_ShapeIconInfos;
};
