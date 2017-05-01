#pragma once

#include <RendererCore/Pipeline/RenderData.h>
#include <Foundation/Strings/HashedString.h>

class ezExtractedRenderData;

class EZ_RENDERERCORE_DLL ezExtractor : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExtractor, ezReflectedClass);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezExtractor);

public:
  ezExtractor(const char* szName);
  virtual ~ezExtractor();

  /// \brief Sets the name of the extractor.
  void SetName(const char* szName);

  /// \brief returns the name of the extractor.
  const char* GetName() const;

  /// \brief returns true if the given object should be filtered by view tags.
  bool FilterByViewTags(const ezView& view, const ezGameObject* pObject) const;

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
    ezExtractedRenderData* pExtractedRenderData);

  virtual void PostSortAndBatch(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
    ezExtractedRenderData* pExtractedRenderData);

private:
  friend class ezRenderPipeline;

  bool m_bActive;

  ezHashedString m_sName;

protected:
  ezHybridArray<ezHashedString, 4> m_DependsOn;
};


class EZ_RENDERERCORE_DLL ezVisibleObjectsExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisibleObjectsExtractor, ezExtractor);
public:
  ezVisibleObjectsExtractor(const char* szName = "VisibleObjectsExtractor");

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
    ezExtractedRenderData* pExtractedRenderData) override;
};

class EZ_RENDERERCORE_DLL ezSelectedObjectsExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSelectedObjectsExtractor, ezExtractor);
public:
  ezSelectedObjectsExtractor(const char* szName = "SelectedObjectsExtractor");

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
    ezExtractedRenderData* pExtractedRenderData) override;

  virtual const ezDeque<ezGameObjectHandle>* GetSelection() = 0;

  ezRenderData::Category m_OverrideCategory;
};
