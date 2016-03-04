#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/View.h>

#include <Core/World/World.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExtractor, 1, ezRTTINoAllocator);
EZ_BEGIN_PROPERTIES
EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisibleObjectsExtractor, 1, ezRTTIDefaultAllocator<ezVisibleObjectsExtractor>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSelectedObjectsExtractor, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCallDelegateExtractor, 1, ezRTTIDefaultAllocator<ezCallDelegateExtractor>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

void ezExtractor::SetName(const char* szName)
{
  m_sName.Assign(szName);
}

const char* ezExtractor::GetName() const
{
  return m_sName.GetData();
}

void ezVisibleObjectsExtractor::Extract(const ezView& view, ezBatchedRenderData* pBatchedRenderData)
{
  ezExtractRenderDataMessage msg;
  msg.m_pView = &view;
  msg.m_pBatchedRenderData = pBatchedRenderData;
  msg.m_OverrideCategory = ezInvalidIndex;

  EZ_LOCK(view.GetWorld()->GetReadMarker());

  /// \todo use spatial data to do visibility culling etc.
  for (auto it = view.GetWorld()->GetObjects(); it.IsValid(); ++it)
  {
    const ezGameObject* pObject = it;

    if (!view.m_ExcludeTags.IsEmpty() && view.m_ExcludeTags.IsAnySet(pObject->GetTags()))
      continue;

    if (!view.m_IncludeTags.IsEmpty() && !view.m_IncludeTags.IsAnySet(pObject->GetTags()))
      continue;

    pObject->SendMessage(msg);
  }
}


ezSelectedObjectsExtractor::ezSelectedObjectsExtractor()
{
  m_OverrideCategory = ezDefaultRenderDataCategories::Selection;
}

void ezSelectedObjectsExtractor::Extract(const ezView& view, ezBatchedRenderData* pBatchedRenderData)
{
  const ezDeque<ezGameObjectHandle>* pSelection = GetSelection();
  if (pSelection == nullptr)
    return;

  ezExtractRenderDataMessage msg;
  msg.m_pView = &view;
  msg.m_pBatchedRenderData = pBatchedRenderData;
  msg.m_OverrideCategory = m_OverrideCategory;

  /// \todo Move this into an editor specific extractor
  auto exclFlags = view.m_ExcludeTags;
  exclFlags.RemoveByName("EditorSelected");

  EZ_LOCK(view.GetWorld()->GetReadMarker());

  for (const auto& hObj : *pSelection)
  {
    ezGameObject* pObject;

    if (!view.GetWorld()->TryGetObject(hObj, pObject))
      continue;

    if (!exclFlags.IsEmpty() && exclFlags.IsAnySet(pObject->GetTags()))
      continue;

    if (!view.m_IncludeTags.IsEmpty() && !view.m_IncludeTags.IsAnySet(pObject->GetTags()))
      continue;

    pObject->SendMessage(msg);
  }
}

ezCallDelegateExtractor::ezCallDelegateExtractor()
{

}

void ezCallDelegateExtractor::Extract(const ezView& view, ezBatchedRenderData* pBatchedRenderData)
{
  m_Delegate();
}
