#include <RendererCore/RendererCorePCH.h>

#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
ezCVarBool cvar_SpatialVisBounds("Spatial.VisBounds", false, ezCVarFlags::Default, "Enables debug visualization of object bounds");
ezCVarBool cvar_SpatialVisLocalBBox("Spatial.VisLocalBBox", false, ezCVarFlags::Default, "Enables debug visualization of object local bounding box");
ezCVarBool cvar_SpatialVisData("Spatial.VisData", false, ezCVarFlags::Default, "Enables debug visualization of the spatial data structure");
ezCVarString cvar_SpatialVisDataOnlyCategory("Spatial.VisData.OnlyCategory", "", ezCVarFlags::Default, "When set the debug visualization is only shown for the given spatial data category");
ezCVarBool cvar_SpatialVisDataOnlySelected("Spatial.VisData.OnlySelected", false, ezCVarFlags::Default, "When set the debug visualization is only shown for selected objects");
ezCVarString cvar_SpatialVisDataOnlyObject("Spatial.VisData.OnlyObject", "", ezCVarFlags::Default, "When set the debug visualization is only shown for objects with the given name");

ezCVarBool cvar_SpatialExtractionShowStats("Spatial.Extraction.ShowStats", false, ezCVarFlags::Default, "Display some stats of the render data extraction");
#endif

namespace
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  void VisualizeSpatialData(const ezView& view)
  {
    if (cvar_SpatialVisData && cvar_SpatialVisDataOnlyObject.GetValue().IsEmpty() && !cvar_SpatialVisDataOnlySelected)
    {
      const ezSpatialSystem& spatialSystem = *view.GetWorld()->GetSpatialSystem();
      if (auto pSpatialSystemGrid = ezDynamicCast<const ezSpatialSystem_RegularGrid*>(&spatialSystem))
      {
        ezSpatialData::Category filterCategory = ezSpatialData::FindCategory(cvar_SpatialVisDataOnlyCategory.GetValue());

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
    if (!cvar_SpatialVisBounds && !cvar_SpatialVisLocalBBox && !cvar_SpatialVisData)
      return;

    if (cvar_SpatialVisLocalBBox)
    {
      const ezBoundingBoxSphere& localBounds = pObject->GetLocalBounds();
      if (localBounds.IsValid())
      {
        ezDebugRenderer::DrawLineBox(view.GetHandle(), localBounds.GetBox(), ezColor::Yellow, pObject->GetGlobalTransform());
      }
    }

    if (cvar_SpatialVisBounds)
    {
      const ezBoundingBoxSphere& globalBounds = pObject->GetGlobalBounds();
      if (globalBounds.IsValid())
      {
        ezDebugRenderer::DrawLineBox(view.GetHandle(), globalBounds.GetBox(), ezColor::Lime);
        ezDebugRenderer::DrawLineSphere(view.GetHandle(), globalBounds.GetSphere(), ezColor::Magenta);
      }
    }

    if (cvar_SpatialVisData && cvar_SpatialVisDataOnlyCategory.GetValue().IsEmpty())
    {
      const ezSpatialSystem& spatialSystem = *view.GetWorld()->GetSpatialSystem();
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
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
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
      new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Red)),
      new ezCategoryAttribute("Extractors")
    }
    EZ_END_ATTRIBUTES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format om

ezExtractor::ezExtractor(const char* szName)
{
  m_bActive = true;
  m_sName.Assign(szName);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  m_uiNumCachedRenderData = 0;
  m_uiNumUncachedRenderData = 0;
#endif
}

ezExtractor::~ezExtractor() = default;

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
  auto AddRenderDataFromMessage = [&](const ezMsgExtractRenderData& msg) {
    if (msg.m_OverrideCategory != ezInvalidRenderDataCategory)
    {
      for (auto& data : msg.m_ExtractedRenderData)
      {
        extractedRenderData.AddRenderData(data.m_pRenderData, msg.m_OverrideCategory);
      }
    }
    else
    {
      for (auto& data : msg.m_ExtractedRenderData)
      {
        extractedRenderData.AddRenderData(data.m_pRenderData, data.m_Category);
      }
    }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    m_uiNumUncachedRenderData += msg.m_ExtractedRenderData.GetCount();
#endif
  };

  if (pObject->IsStatic())
  {
    ezUInt16 uiComponentVersion = pObject->GetComponentVersion();

    auto cachedRenderData = ezRenderWorld::GetCachedRenderData(view, pObject->GetHandle(), uiComponentVersion);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    for (ezUInt32 i = 1; i < cachedRenderData.GetCount(); ++i)
    {
      EZ_ASSERT_DEBUG(cachedRenderData[i - 1].m_uiComponentIndex <= cachedRenderData[i].m_uiComponentIndex, "Cached render data needs to be sorted");
      if (cachedRenderData[i - 1].m_uiComponentIndex == cachedRenderData[i].m_uiComponentIndex)
      {
        EZ_ASSERT_DEBUG(cachedRenderData[i - 1].m_uiPartIndex < cachedRenderData[i].m_uiPartIndex, "Cached render data needs to be sorted");
      }
    }
#endif

    ezUInt32 uiCacheIndex = 0;

    auto components = pObject->GetComponents();
    const ezUInt32 uiNumComponents = components.GetCount();
    for (ezUInt32 uiComponentIndex = 0; uiComponentIndex < uiNumComponents; ++uiComponentIndex)
    {
      bool bCacheFound = false;
      while (uiCacheIndex < cachedRenderData.GetCount() && cachedRenderData[uiCacheIndex].m_uiComponentIndex == uiComponentIndex)
      {
        const ezInternal::RenderDataCacheEntry& cacheEntry = cachedRenderData[uiCacheIndex];
        if (cacheEntry.m_pRenderData != nullptr)
        {
          extractedRenderData.AddRenderData(cacheEntry.m_pRenderData, msg.m_OverrideCategory != ezInvalidRenderDataCategory ? msg.m_OverrideCategory : ezRenderData::Category(cacheEntry.m_uiCategory));

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
          ++m_uiNumCachedRenderData;
#endif
        }
        ++uiCacheIndex;

        bCacheFound = true;
      }

      if (bCacheFound)
      {
        continue;
      }

      const ezComponent* pComponent = components[uiComponentIndex];

      msg.m_ExtractedRenderData.Clear();
      msg.m_uiNumCacheIfStatic = 0;

      if (pComponent->SendMessage(msg))
      {
        // Only cache render data if all parts should be cached otherwise the cache is incomplete and we won't call SendMessage again
        if (msg.m_uiNumCacheIfStatic > 0 && msg.m_ExtractedRenderData.GetCount() == msg.m_uiNumCacheIfStatic)
        {
          ezHybridArray<ezInternal::RenderDataCacheEntry, 16> newCacheEntries(ezFrameAllocator::GetCurrentAllocator());

          for (ezUInt32 uiPartIndex = 0; uiPartIndex < msg.m_ExtractedRenderData.GetCount(); ++uiPartIndex)
          {
            auto& newCacheEntry = newCacheEntries.ExpandAndGetRef();
            newCacheEntry.m_pRenderData = msg.m_ExtractedRenderData[uiPartIndex].m_pRenderData;
            newCacheEntry.m_uiCategory = msg.m_ExtractedRenderData[uiPartIndex].m_Category.m_uiValue;
            newCacheEntry.m_uiComponentIndex = static_cast<ezUInt16>(uiComponentIndex);
            newCacheEntry.m_uiPartIndex = static_cast<ezUInt16>(uiPartIndex);
          }

          ezRenderWorld::CacheRenderData(view, pObject->GetHandle(), pComponent->GetHandle(), uiComponentVersion, newCacheEntries);
        }

        AddRenderDataFromMessage(msg);
      }
      else if (pComponent->IsActiveAndInitialized()) // component does not handle extract message at all
      {
        EZ_ASSERT_DEV(pComponent->GetDynamicRTTI()->CanHandleMessage<ezMsgExtractRenderData>() == false, "");

        // Create a dummy cache entry so we don't call send message next time
        ezInternal::RenderDataCacheEntry dummyEntry;
        dummyEntry.m_pRenderData = nullptr;
        dummyEntry.m_uiCategory = ezInvalidRenderDataCategory.m_uiValue;
        dummyEntry.m_uiComponentIndex = static_cast<ezUInt16>(uiComponentIndex);

        ezRenderWorld::CacheRenderData(view, pObject->GetHandle(), pComponent->GetHandle(), uiComponentVersion, ezMakeArrayPtr(&dummyEntry, 1));
      }
    }
  }
  else
  {
    msg.m_ExtractedRenderData.Clear();
    pObject->SendMessage(msg);

    AddRenderDataFromMessage(msg);
  }
}

ezResult ezExtractor::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_bActive;
  inout_stream << m_sName;
  return EZ_SUCCESS;
}


ezResult ezExtractor::Deserialize(ezStreamReader& inout_stream)
{
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_ASSERT_DEBUG(uiVersion == 1, "Unknown version encountered");

  inout_stream >> m_bActive;
  inout_stream >> m_sName;
  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisibleObjectsExtractor, 1, ezRTTIDefaultAllocator<ezVisibleObjectsExtractor>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisibleObjectsExtractor::ezVisibleObjectsExtractor(const char* szName)
  : ezExtractor(szName)
{
}

ezVisibleObjectsExtractor::~ezVisibleObjectsExtractor() = default;

void ezVisibleObjectsExtractor::Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData)
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
    ExtractRenderData(view, pObject, msg, ref_extractedRenderData);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    if (cvar_SpatialVisBounds || cvar_SpatialVisLocalBBox || cvar_SpatialVisData)
    {
      if ((cvar_SpatialVisDataOnlyObject.GetValue().IsEmpty() ||
            pObject->GetName().FindSubString_NoCase(cvar_SpatialVisDataOnlyObject.GetValue()) != nullptr) &&
          !cvar_SpatialVisDataOnlySelected)
      {
        VisualizeObject(view, pObject);
      }
    }
#endif
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  const bool bIsMainView = (view.GetCameraUsageHint() == ezCameraUsageHint::MainView || view.GetCameraUsageHint() == ezCameraUsageHint::EditorView);

  if (cvar_SpatialExtractionShowStats && bIsMainView)
  {
    ezViewHandle hView = view.GetHandle();

    ezStringBuilder sb;

    ezDebugRenderer::DrawInfoText(hView, ezDebugTextPlacement::TopLeft, "ExtractionStats", "Extraction Stats:");

    sb.SetFormat("Num Cached Render Data: {0}", m_uiNumCachedRenderData);
    ezDebugRenderer::DrawInfoText(hView, ezDebugTextPlacement::TopLeft, "ExtractionStats", sb);

    sb.SetFormat("Num Uncached Render Data: {0}", m_uiNumUncachedRenderData);
    ezDebugRenderer::DrawInfoText(hView, ezDebugTextPlacement::TopLeft, "ExtractionStats", sb);
  }
#endif
}

ezResult ezVisibleObjectsExtractor::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return EZ_SUCCESS;
}

ezResult ezVisibleObjectsExtractor::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSelectedObjectsExtractorBase, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSelectedObjectsExtractorBase::ezSelectedObjectsExtractorBase(const char* szName)
  : ezExtractor(szName)
  , m_OverrideCategory(ezDefaultRenderDataCategories::Selection)
{
}

ezSelectedObjectsExtractorBase::~ezSelectedObjectsExtractorBase() = default;

void ezSelectedObjectsExtractorBase::Extract(
  const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData)
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

    ExtractRenderData(view, pObject, msg, ref_extractedRenderData);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    if (cvar_SpatialVisBounds || cvar_SpatialVisLocalBBox || cvar_SpatialVisData)
    {
      if (cvar_SpatialVisDataOnlySelected)
      {
        VisualizeObject(view, pObject);
      }
    }
#endif
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSelectedObjectsContext, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSelectedObjectsExtractor, 1, ezRTTIDefaultAllocator<ezSelectedObjectsExtractor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("SelectionContext", GetSelectionContext, SetSelectionContext),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSelectedObjectsContext::ezSelectedObjectsContext() = default;
ezSelectedObjectsContext::~ezSelectedObjectsContext() = default;

void ezSelectedObjectsContext::RemoveDeadObjects(const ezWorld& world)
{
  for (ezUInt32 i = 0; i < m_Objects.GetCount();)
  {
    const ezGameObject* pObj;
    if (world.TryGetObject(m_Objects[i], pObj) == false)
    {
      m_Objects.RemoveAtAndSwap(i);
    }
    else
      ++i;
  }
}

void ezSelectedObjectsContext::AddObjectAndChildren(const ezWorld& world, const ezGameObjectHandle& hObject)
{
  const ezGameObject* pObj;
  if (world.TryGetObject(hObject, pObj))
  {
    m_Objects.PushBack(hObject);

    for (auto it = pObj->GetChildren(); it.IsValid(); ++it)
    {
      AddObjectAndChildren(world, it);
    }
  }
}

void ezSelectedObjectsContext::AddObjectAndChildren(const ezWorld& world, const ezGameObject* pObject)
{
  m_Objects.PushBack(pObject->GetHandle());

  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    AddObjectAndChildren(world, it);
  }
}

ezSelectedObjectsExtractor::ezSelectedObjectsExtractor(const char* szName /*= "ExplicitlySelectedObjectsExtractor"*/)
  : ezSelectedObjectsExtractorBase(szName)
{
}

ezSelectedObjectsExtractor::~ezSelectedObjectsExtractor() = default;

const ezDeque<ezGameObjectHandle>* ezSelectedObjectsExtractor::GetSelection()
{
  if (m_pSelectionContext)
    return &m_pSelectionContext->m_Objects;

  return nullptr;
}

ezResult ezSelectedObjectsExtractor::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return EZ_SUCCESS;
}

ezResult ezSelectedObjectsExtractor::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Extractor);
