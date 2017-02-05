#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <Core/World/World.h>
#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Foundation/Configuration/CVar.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezCVarBool CVarVisBounds("r_VisBounds", false, ezCVarFlags::Default, "Enables debug visualization of object bounds");
  ezCVarBool CVarVisLocalBBox("r_VisLocalBBox", false, ezCVarFlags::Default, "Enables debug visualization of object local bounding box");
  ezCVarBool CVarVisSpatialData("r_VisSpatialData", false, ezCVarFlags::Default, "Enables debug visualization of the spatial data structure");
  ezCVarString CVarVisObjectName("r_VisObjectName", "", ezCVarFlags::Default, "When set the debug visualization is only shown for objects with the given name");
#endif

namespace
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  void VisualizeSpatialData(const ezView& view)
  {
    if (CVarVisSpatialData && CVarVisObjectName.GetValue().IsEmpty() &&
      view.GetCameraUsageHint() == ezCameraUsageHint::MainView)
    {
      const ezSpatialSystem& spatialSystem = view.GetWorld()->GetSpatialSystem();
      if (auto pSpatialSystemGrid = ezDynamicCast<const ezSpatialSystem_RegularGrid*>(&spatialSystem))
      {
        ezHybridArray<ezBoundingBox, 16> boxes;
        pSpatialSystemGrid->GetAllCellBoxes(boxes);

        for (auto& box : boxes)
        {
          ezDebugRenderer::DrawLineBox(&view, box, ezColor::Cyan);
        }
      }
    }
  }

  void VisualizeObject(const ezView& view, const ezGameObject* pObject)
  {
    if (!CVarVisBounds && !CVarVisLocalBBox && !CVarVisSpatialData)
      return;

    if (view.GetCameraUsageHint() != ezCameraUsageHint::MainView)
      return;

    if (CVarVisObjectName.GetValue().IsEmpty() || CVarVisObjectName.GetValue() == pObject->GetName())
    {
      if (CVarVisLocalBBox)
      {
        const ezBoundingBoxSphere& localBounds = pObject->GetLocalBounds();
        if (localBounds.IsValid())
        {
          ezDebugRenderer::DrawLineBox(&view, localBounds.GetBox(), ezColor::Yellow, pObject->GetGlobalTransform());
        }
      }

      if (CVarVisBounds)
      {
        const ezBoundingBoxSphere& globalBounds = pObject->GetGlobalBounds();
        if (globalBounds.IsValid())
        {
          ezDebugRenderer::DrawLineBox(&view, globalBounds.GetBox(), ezColor::Lime);
          ezDebugRenderer::DrawLineSphere(&view, globalBounds.GetSphere(), ezColor::Magenta);
        }
      }

      if (CVarVisSpatialData)
      {
        const ezSpatialSystem& spatialSystem = view.GetWorld()->GetSpatialSystem();
        if (auto pSpatialSystemGrid = ezDynamicCast<const ezSpatialSystem_RegularGrid*>(&spatialSystem))
        {
          ezHybridArray<ezBoundingBox, 16> boxes;
          pSpatialSystemGrid->GetCellBoxesForSpatialData(pObject->GetSpatialData(), boxes);

          for (auto& box : boxes)
          {
            ezDebugRenderer::DrawLineBox(&view, box, ezColor::Cyan);
          }
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
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
  {
    new ezColorAttribute(ezColorGammaUB(128, 0, 0))
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezExtractor::SetName(const char* szName)
{
  m_sName.Assign(szName);
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

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisibleObjectsExtractor, 1, ezRTTIDefaultAllocator<ezVisibleObjectsExtractor>)
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezVisibleObjectsExtractor::Extract(const ezView& view, ezExtractedRenderData* pExtractedRenderData)
{
  ezExtractRenderDataMessage msg;
  msg.m_pView = &view;
  msg.m_pExtractedRenderData = pExtractedRenderData;
  msg.m_OverrideCategory = ezInvalidIndex;

  EZ_LOCK(view.GetWorld()->GetReadMarker());

  #if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    VisualizeSpatialData(view);
  #endif

  /// \todo use spatial data to do visibility culling etc.
  for (auto it = view.GetWorld()->GetObjects(); it.IsValid(); ++it)
  {
    const ezGameObject* pObject = it;
    if (FilterByViewTags(view, pObject))
      continue;

    pObject->SendMessage(msg);

    #if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      VisualizeObject(view, pObject);
    #endif
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSelectedObjectsExtractor, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSelectedObjectsExtractor::ezSelectedObjectsExtractor()
{
  m_OverrideCategory = ezDefaultRenderDataCategories::Selection;
}

void ezSelectedObjectsExtractor::Extract(const ezView& view, ezExtractedRenderData* pExtractedRenderData)
{
  const ezDeque<ezGameObjectHandle>* pSelection = GetSelection();
  if (pSelection == nullptr)
    return;

  ezExtractRenderDataMessage msg;
  msg.m_pView = &view;
  msg.m_pExtractedRenderData = pExtractedRenderData;
  msg.m_OverrideCategory = m_OverrideCategory;

  EZ_LOCK(view.GetWorld()->GetReadMarker());

  for (const auto& hObj : *pSelection)
  {
    const ezGameObject* pObject;

    if (!view.GetWorld()->TryGetObject(hObj, pObject))
      continue;

    if (FilterByViewTags(view, pObject))
      continue;

    pObject->SendMessage(msg);
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Extractor);

