#pragma once

#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Pipeline/RenderData.h>

class ezStreamWriter;

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

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData) = 0;

  virtual void PostSortAndBatch(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData) = 0;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const;
  virtual ezResult Deserialize(ezStreamReader& inout_stream);

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

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData) override;
  virtual void PostSortAndBatch(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData) override {}

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;
};

class EZ_RENDERERCORE_DLL ezSelectedObjectsExtractorBase : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSelectedObjectsExtractorBase, ezExtractor);

public:
  ezSelectedObjectsExtractorBase(const char* szName = "SelectedObjectsExtractor");
  ~ezSelectedObjectsExtractorBase();

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData) override;
  virtual void PostSortAndBatch(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData) override {}

  virtual const ezDeque<ezGameObjectHandle>* GetSelection() = 0;

  ezRenderData::Category m_OverrideCategory;
};

/// \brief Stores a list of game objects that should get highlighted by the renderer.
///
/// Store an instance somewhere in your game code:
/// ezSelectedObjectsContext m_SelectedObjects;
/// Add handles to game object that should be get the highlighting outline (as the editor uses for selected objects).
/// On an ezView call:
/// ezView::SetExtractorProperty("HighlightObjects", "SelectionContext", &m_SelectedObjects);
/// The first name must be the name of an ezSelectedObjectsExtractor that is instantiated by the render pipeline.
///
/// As long as there is also an ezSelectionHighlightPass in the render pipeline, all objects in this selection will be rendered
/// with an outline.
class EZ_RENDERERCORE_DLL ezSelectedObjectsContext : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSelectedObjectsContext, ezReflectedClass);

public:
  ezSelectedObjectsContext();
  ~ezSelectedObjectsContext();

  void RemoveDeadObjects(const ezWorld& world);
  void AddObjectAndChildren(const ezWorld& world, const ezGameObjectHandle& hObject);
  void AddObjectAndChildren(const ezWorld& world, const ezGameObject* pObject);

  ezDeque<ezGameObjectHandle> m_Objects;
};

/// \brief An extractor that can be instantiated in a render pipeline, to define manually which objects should be rendered with a selection outline.
///
/// \sa ezSelectedObjectsContext
class EZ_RENDERERCORE_DLL ezSelectedObjectsExtractor : public ezSelectedObjectsExtractorBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSelectedObjectsExtractor, ezSelectedObjectsExtractorBase);

public:
  ezSelectedObjectsExtractor(const char* szName = "ExplicitlySelectedObjectsExtractor");
  ~ezSelectedObjectsExtractor();

  virtual const ezDeque<ezGameObjectHandle>* GetSelection() override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  /// \brief The context is typically set through an ezView, through ezView::SetExtractorProperty("<name>", "SelectionContext", pointer);
  void SetSelectionContext(ezSelectedObjectsContext* pSelectionContext) { m_pSelectionContext = pSelectionContext; } // [ property ]
  ezSelectedObjectsContext* GetSelectionContext() const { return m_pSelectionContext; }                              // [ property ]

private:
  ezSelectedObjectsContext* m_pSelectionContext = nullptr;
};
