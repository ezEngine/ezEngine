#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/View.h>

#include <Core/World/World.h>


void ezVisibleObjectsExtractor::Extract(const ezView& view)
{
  ezExtractRenderDataMessage msg;
  msg.m_pView = &view;
  msg.m_OverrideRenderPass = ezInvalidIndex;

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
  m_OverridePassType = ezDefaultPassTypes::Selection;
  m_pSelection = nullptr;
}

void ezSelectedObjectsExtractor::Extract(const ezView& view)
{
  if (m_pSelection == nullptr)
    return;

  ezExtractRenderDataMessage msg;
  msg.m_pView = &view;
  msg.m_OverrideRenderPass = m_OverridePassType;

  EZ_LOCK(view.GetWorld()->GetReadMarker());

  for (const auto& hObj : *m_pSelection)
  {
    ezGameObject* pObject;

    if (!view.GetWorld()->TryGetObject(hObj, pObject))
      continue;

    if (!view.m_ExcludeTags.IsEmpty() && view.m_ExcludeTags.IsAnySet(pObject->GetTags()))
      continue;

    if (!view.m_IncludeTags.IsEmpty() && !view.m_IncludeTags.IsAnySet(pObject->GetTags()))
      continue;

    pObject->SendMessage(msg);
  }
}

ezCallDelegateExtractor::ezCallDelegateExtractor()
{

}

void ezCallDelegateExtractor::Extract(const ezView& view)
{
  m_Delegate();
}
