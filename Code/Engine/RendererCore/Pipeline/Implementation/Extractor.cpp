#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Debug/DebugRenderer.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExtractor, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisibleObjectsExtractor, 1, ezRTTIDefaultAllocator<ezVisibleObjectsExtractor>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSelectedObjectsExtractor, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezExtractor::SetName(const char* szName)
{
  m_sName.Assign(szName);
}

const char* ezExtractor::GetName() const
{
  return m_sName.GetData();
}

void ezVisibleObjectsExtractor::Extract(const ezView& view, ezExtractedRenderData* pExtractedRenderData)
{
  ezExtractRenderDataMessage msg;
  msg.m_pView = &view;
  msg.m_pExtractedRenderData = pExtractedRenderData;
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

    if (false)
    {
      if (pObject->GetLocalBounds().IsValid())
      {
        ezDebugRenderer::DrawLineBox(view.GetWorld(), pObject->GetLocalBounds().GetBox(), ezColor::LimeGreen, pObject->GetGlobalTransform());
        ezDebugRenderer::DrawLineBox(view.GetWorld(), pObject->GetGlobalBounds().GetBox(), ezColor::Magenta);
      }

      ezVec4 screenPos = view.GetViewProjectionMatrix().Transform(pObject->GetGlobalPosition().GetAsVec4(1.0f));
      if (screenPos.w > 0.0f)
      {
        ezVec2 halfScreenSize(view.GetViewport().width * 0.5f, view.GetViewport().height * 0.5f);

        screenPos /= screenPos.w;
        screenPos.x = screenPos.x * halfScreenSize.x + halfScreenSize.x;
        screenPos.y = screenPos.y * -halfScreenSize.y + halfScreenSize.y;

        ezDebugRenderer::DrawText(view.GetWorld(), pObject->GetName(), ezVec2I32((ezInt32)screenPos.x, (ezInt32)screenPos.y), ezColor::White);
      }
    }
  }
}


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

  auto exclFlags = view.m_ExcludeTags;

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
