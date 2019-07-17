#include <RendererCorePCH.h>

#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <Core/World/World.h>
#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezCVarBool CVarVisBounds("r_VisBounds", false, ezCVarFlags::Default, "Enables debug visualization of object bounds");
  ezCVarBool CVarVisLocalBBox("r_VisLocalBBox", false, ezCVarFlags::Default, "Enables debug visualization of object local bounding box");
  ezCVarBool CVarVisSpatialData("r_VisSpatialData", false, ezCVarFlags::Default, "Enables debug visualization of the spatial data structure");
  ezCVarString CVarVisSpatialCategory("r_VisSpatialCategory", "", ezCVarFlags::Default, "When set the debug visualization is only shown for the given spatial data category");
  ezCVarBool CVarVisObjectSelection("r_VisObjectSelection", false, ezCVarFlags::Default, "When set the debug visualization is only shown for selected objects");
  ezCVarString CVarVisObjectName("r_VisObjectName", "", ezCVarFlags::Default, "When set the debug visualization is only shown for objects with the given name");

  ezCVarBool CVarExtractionStats("r_ExtractionStats", false, ezCVarFlags::Default, "Display some stats of the render data extraction");
#endif

namespace
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  void VisualizeSpatialData(const ezView& view)
  {
    if (CVarVisSpatialData && CVarVisObjectName.GetValue().IsEmpty() && !CVarVisObjectSelection)
    {
      const ezSpatialSystem& spatialSystem = view.GetWorld()->GetSpatialSystem();
      if (auto pSpatialSystemGrid = ezDynamicCast<const ezSpatialSystem_RegularGrid*>(&spatialSystem))
      {
        ezSpatialData::Category filterCategory = ezSpatialData::FindCategory(CVarVisSpatialCategory.GetValue());

        ezHybridArray<ezBoundingBox, 16> boxes;
        pSpatialSystemGrid->GetAllCellBoxes(boxes, filterCategory);

        for (auto& box : boxes)
        {
          ezDebugRenderer::DrawLineBox(view.GetHandle(), box, ezColor::Cyan);
        }
      }
    }
  }

  void VisualizeObject(const ezView& view, const ezGameObject* pObject)
  {
    if (!CVarVisBounds && !CVarVisLocalBBox && !CVarVisSpatialData)
      return;

    if (CVarVisLocalBBox)
    {
      const ezBoundingBoxSphere& localBounds = pObject->GetLocalBounds();
      if (localBounds.IsValid())
      {
        ezDebugRenderer::DrawLineBox(view.GetHandle(), localBounds.GetBox(), ezColor::Yellow, pObject->GetGlobalTransform());
      }
    }

    if (CVarVisBounds)
    {
      const ezBoundingBoxSphere& globalBounds = pObject->GetGlobalBounds();
      if (globalBounds.IsValid())
      {
        ezDebugRenderer::DrawLineBox(view.GetHandle(), globalBounds.GetBox(), ezColor::Lime);
        ezDebugRenderer::DrawLineSphere(view.GetHandle(), globalBounds.GetSphere(), ezColor::Magenta);
      }
    }

    if (CVarVisSpatialData)
    {
      const ezSpatialSystem& spatialSystem = view.GetWorld()->GetSpatialSystem();
      if (auto pSpatialSystemGrid = ezDynamicCast<const ezSpatialSystem_RegularGrid*>(&spatialSystem))
      {
        ezBoundingBox box;
        if (pSpatialSystemGrid->GetCellBoxForSpatialData(pObject->GetSpatialData(), box).Succeeded())
        {
          ezDebugRenderer::DrawLineBox(view.GetHandle(), box, ezColor::Cyan);
        }
      }
    }
  }
#endif
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExtractor, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezColorAttribute(ezColorGammaUB(128, 0, 0))
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezExtractor::ezExtractor(const char* szName)
{
  m_bActive = true;
  m_sName.Assign(szName);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  m_uiNumCachedRenderData = 0;
  m_uiNumUncachedRenderData = 0;
#endif
}

ezExtractor::~ezExtractor()
{

}

void ezExtractor::SetName(const char* szName)
{
  if (!ezStringUtils::IsNullOrEmpty(szName))
  {
    m_sName.Assign(szName);
  }
}

const char* ezExtractor::GetName() const
{
  return m_sName.GetData();
}

bool ezExtractor::FilterByViewTags(const ezView& view, const ezGameObject* pObject) const
{
  if (!view.m_ExcludeTags.IsEmpty() && view.m_ExcludeTags.IsAnySet(pObject->GetTags()))
    return true;

  if (!view.m_IncludeTags.IsEmpty() && !view.m_IncludeTags.IsAnySet(pObject->GetTags()))
    return true;

  return false;
}

void ezExtractor::ExtractRenderData(const ezView& view, const ezGameObject* pObject, ezMsgExtractRenderData& msg, ezExtractedRenderData& extractedRenderData) const
{
  if (FilterByViewTags(view, pObject))
  {
    return;
  }

  msg.m_ExtractedRenderData.Clear();
  ezUInt32 uiNumUncachedRenderData = 0;

  if (pObject->IsStatic())
  {
    auto cachedRenderData = ezRenderWorld::GetCachedRenderData(view, pObject->GetHandle());
    ezUInt32 uiCacheIndex = 0;

    auto components = pObject->GetComponents();
    const ezUInt32 uiNumComponents = components.GetCount();
    for (ezUInt32 uiComponentIndex = 0; uiComponentIndex < uiNumComponents; ++uiComponentIndex)
    {
      bool bCacheFound = false;
      while (uiCacheIndex < cachedRenderData.GetCount() && cachedRenderData[uiCacheIndex].m_uiComponentIndex == uiComponentIndex)
      {
        if (cachedRenderData[uiCacheIndex].m_pRenderData != nullptr)
        {
          msg.m_ExtractedRenderData.PushBack(cachedRenderData[uiCacheIndex]);
        }
        ++uiCacheIndex;

        bCacheFound = true;
      }

      if (bCacheFound)
      {
        continue;
      }

      ezUInt32 uiOldRenderDataCount = msg.m_ExtractedRenderData.GetCount();
      const ezComponent* pComponent = components[uiComponentIndex];
      if (pComponent->SendMessage(msg))
      {
        if (msg.m_ExtractedRenderData.GetCount() > uiOldRenderDataCount)
        {
          auto newCacheEntries = msg.m_ExtractedRenderData.GetArrayPtr().GetSubArray(uiOldRenderDataCount);
          if (newCacheEntries[0].m_uiCacheIfStatic)
          {
            for (auto& newCacheEntry : newCacheEntries)
            {
              newCacheEntry.m_uiComponentIndex = uiComponentIndex;
            }

            ezRenderWorld::CacheRenderData(view, pObject->GetHandle(), pComponent->GetHandle(), newCacheEntries);
          }

          uiNumUncachedRenderData += newCacheEntries.GetCount();
        }
      }
      else // component does not handle extract message at all
      {
        // Create a dummy cache entry so we don't call send message next time
        ezInternal::RenderDataCacheEntry dummyEntry;
        dummyEntry.m_pRenderData = nullptr;
        dummyEntry.m_uiSortingKey = 0;
        dummyEntry.m_uiCategory = ezInvalidRenderDataCategory.m_uiValue;
        dummyEntry.m_uiComponentIndex = uiComponentIndex;
        dummyEntry.m_uiCacheIfStatic = true;

        ezRenderWorld::CacheRenderData(view, pObject->GetHandle(), pComponent->GetHandle(), ezMakeArrayPtr(&dummyEntry, 1));
      }
    }
  }
  else
  {
    pObject->SendMessage(msg);

    uiNumUncachedRenderData += msg.m_ExtractedRenderData.GetCount();
  }

  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory)
  {
    for (auto& cached : msg.m_ExtractedRenderData)
    {
      extractedRenderData.AddRenderData(cached.m_pRenderData, msg.m_OverrideCategory);
    }
  }
  else
  {
    for (auto& cached : msg.m_ExtractedRenderData)
    {
      extractedRenderData.AddRenderData(cached.m_pRenderData, ezRenderData::Category(cached.m_uiCategory));
    }
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  m_uiNumCachedRenderData += msg.m_ExtractedRenderData.GetCount() - uiNumUncachedRenderData;
  m_uiNumUncachedRenderData += uiNumUncachedRenderData;
#endif
}

void ezExtractor::Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& extractedRenderData)
{

}

void ezExtractor::PostSortAndBatch(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& extractedRenderData)
{

}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisibleObjectsExtractor, 1, ezRTTIDefaultAllocator<ezVisibleObjectsExtractor>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisibleObjectsExtractor::ezVisibleObjectsExtractor(const char* szName)
  : ezExtractor(szName)
{

}

void ezVisibleObjectsExtractor::Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
  ezExtractedRenderData& extractedRenderData)
{
  ezMsgExtractRenderData msg;
  msg.m_pView = &view;

  EZ_LOCK(view.GetWorld()->GetReadMarker());

  #if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    VisualizeSpatialData(view);

    m_uiNumCachedRenderData = 0;
    m_uiNumUncachedRenderData = 0;
  #endif

  for (auto pObject : visibleObjects)
  {
    ExtractRenderData(view, pObject, msg, extractedRenderData);

    #if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      if (CVarVisBounds || CVarVisLocalBBox || CVarVisSpatialData)
      {
        if ((CVarVisObjectName.GetValue().IsEmpty() || ezStringUtils::FindSubString_NoCase(pObject->GetName(), CVarVisObjectName.GetValue()) != nullptr) &&
          !CVarVisObjectSelection)
        {
          VisualizeObject(view, pObject);
        }
      }
    #endif
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  const bool bIsMainView = (view.GetCameraUsageHint() == ezCameraUsageHint::MainView || view.GetCameraUsageHint() == ezCameraUsageHint::EditorView);

  if (CVarExtractionStats && bIsMainView)
  {
    ezViewHandle hView = view.GetHandle();

    ezStringBuilder sb;

    ezDebugRenderer::Draw2DText(hView, "Extraction Stats", ezVec2I32(10, 200), ezColor::LimeGreen);

    sb.Format("Num Cached Render Data: {0}", m_uiNumCachedRenderData);
    ezDebugRenderer::Draw2DText(hView, sb, ezVec2I32(10, 220), ezColor::LimeGreen);

    sb.Format("Num Uncached Render Data: {0}", m_uiNumUncachedRenderData);
    ezDebugRenderer::Draw2DText(hView, sb, ezVec2I32(10, 240), ezColor::LimeGreen);
  }
#endif
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSelectedObjectsExtractor, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSelectedObjectsExtractor::ezSelectedObjectsExtractor(const char* szName)
  : ezExtractor(szName)
  , m_OverrideCategory(ezDefaultRenderDataCategories::Selection)
{
}

void ezSelectedObjectsExtractor::Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
  ezExtractedRenderData& extractedRenderData)
{
  const ezDeque<ezGameObjectHandle>* pSelection = GetSelection();
  if (pSelection == nullptr)
    return;

  ezMsgExtractRenderData msg;
  msg.m_pView = &view;
  msg.m_OverrideCategory = m_OverrideCategory;

  EZ_LOCK(view.GetWorld()->GetReadMarker());

  for (const auto& hObj : *pSelection)
  {
    const ezGameObject* pObject = nullptr;
    if (!view.GetWorld()->TryGetObject(hObj, pObject))
      continue;

    ExtractRenderData(view, pObject, msg, extractedRenderData);

    #if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      if (CVarVisBounds || CVarVisLocalBBox || CVarVisSpatialData)
      {
        if (CVarVisObjectSelection)
        {
          VisualizeObject(view, pObject);
        }
      }
    #endif
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Extractor);

