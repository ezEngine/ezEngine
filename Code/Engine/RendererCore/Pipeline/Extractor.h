#pragma once

#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Pipeline/RenderData.h>

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

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& extractedRenderData);

  virtual void PostSortAndBatch(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& extractedRenderData);

protected:
  /// \brief returns true if the given object should be filtered by view tags.
  bool FilterByViewTags(const ezView& view, const ezGameObject* pObject) const;

  /// \brief extracts the render data for the given object.
  void ExtractRenderData(const ezView& view, const ezGameObject* pObject, ezMsgExtractRenderData& msg, ezExtractedRenderData& extractedRenderData) const;

private:
  friend class ezRenderPipeline;

  bool m_bActive;

  ezHashedString m_sName;

protected:
  ezHybridArray<ezHashedString, 4> m_DependsOn;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  mutable ezUInt32 m_uiNumCachedRenderData;
  mutable ezUInt32 m_uiNumUncachedRenderData;
#endif
};


class EZ_RENDERERCORE_DLL ezVisibleObjectsExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisibleObjectsExtractor, ezExtractor);

public:
  ezVisibleObjectsExtractor(const char* szName = "VisibleObjectsExtractor");
  ~ezVisibleObjectsExtractor();

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& extractedRenderData) override;
};

class EZ_RENDERERCORE_DLL ezSelectedObjectsExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSelectedObjectsExtractor, ezExtractor);

public:
  ezSelectedObjectsExtractor(const char* szName = "SelectedObjectsExtractor");
  ~ezSelectedObjectsExtractor();

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& extractedRenderData) override;

  virtual const ezDeque<ezGameObjectHandle>* GetSelection() = 0;

  ezRenderData::Category m_OverrideCategory;
};

/// \brief Stores a list of game objects that should get highlighted by the renderer.
///
/// Store an instance somewhere in your game code:
/// ezExplicitObjectSelection m_SelectedObjects;
/// Add handles to game object that should be get the highlighting outline (as the editor uses for selected objects).
/// On an ezView call:
/// ezView::SetExtractorProperty("HighlightObjects", "SelectionContext", &m_SelectedObjects);
/// The first name must be the name of an ezExplicitlySelectedObjectsExtractor that is instantiated by the render pipeline.
///
/// As long as there is also an ezSelectionHighlightPass in the render pipeline, all objects in this selection will be rendered
/// with an outline.
class EZ_RENDERERCORE_DLL ezExplicitObjectSelection : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExplicitObjectSelection, ezReflectedClass);

public:
  ezDeque<ezGameObjectHandle> m_Objects;
};

/// \brief An extractor that can be instantiated in a render pipeline, to define manually which objects should be rendered with a selection outline.
///
/// \sa ezExplicitObjectSelection
class EZ_RENDERERCORE_DLL ezExplicitlySelectedObjectsExtractor : public ezSelectedObjectsExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExplicitlySelectedObjectsExtractor, ezSelectedObjectsExtractor);

public:
  ezExplicitlySelectedObjectsExtractor(const char* szName = "ExplicitlySelectedObjectsExtractor");
  ~ezExplicitlySelectedObjectsExtractor();

  virtual const ezDeque<ezGameObjectHandle>* GetSelection() override;

  /// \brief The context is typically set through an ezView, through ezView::SetExtractorProperty("<name>", "SelectionContext", pointer);
  void SetSelectionContext(ezExplicitObjectSelection* pSelectionContext) { m_pSelectionContext = pSelectionContext; } // [ property ]
  ezExplicitObjectSelection* GetSelectionContext() const { return m_pSelectionContext; }                              // [ property ]

private:
  ezExplicitObjectSelection* m_pSelectionContext = nullptr;
};
