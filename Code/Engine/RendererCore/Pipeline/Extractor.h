#pragma once

#include <RendererCore/Pipeline/RenderData.h>

class ezExtractedRenderData;

class EZ_RENDERERCORE_DLL ezExtractor : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExtractor, ezReflectedClass);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezExtractor);

public:
  ezExtractor() {}
  virtual ~ezExtractor() {}

  /// \brief Sets the name of the extractor.
  void SetName(const char* szName);

  /// \brief returns the name of the extractor.
  const char* GetName() const;

  virtual void Extract(const ezView& view, ezExtractedRenderData* pExtractedRenderData) = 0;

private:
  friend class ezRenderPipeline;

  bool m_bActive;

  ezHashedString m_sName;
};


class EZ_RENDERERCORE_DLL ezVisibleObjectsExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisibleObjectsExtractor, ezExtractor);
public:
  ezVisibleObjectsExtractor() {}

  virtual void Extract(const ezView& view, ezExtractedRenderData* pExtractedRenderData) override;
};

class EZ_RENDERERCORE_DLL ezSelectedObjectsExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSelectedObjectsExtractor, ezExtractor);
public:
  ezSelectedObjectsExtractor();

  virtual void Extract(const ezView& view, ezExtractedRenderData* pExtractedRenderData) override;
  virtual const ezDeque<ezGameObjectHandle>* GetSelection() = 0;
  ezRenderData::Category m_OverrideCategory;
};
