#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezRenderComponent, 1)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

ezRenderComponent::ezRenderComponent() = default;
ezRenderComponent::~ezRenderComponent() = default;

void ezRenderComponent::OnActivated()
{
  // Ensure that the render data manager exists.
  GetWorld()->GetOrCreateModule<ezRenderDataManager>();

  TriggerLocalBoundsUpdate();
}

void ezRenderComponent::OnDeactivated()
{
  // Can't call InvalidateCachedRenderData because it checks whether we are active, which is not the case anymore.
  ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

  // Only update the local bounds if the owner is still active, if not no other components are active anymore and the bounds update would be pointless.
  // The bounds will be updated when the owner is re-activated anyway.
  if (GetOwner()->IsActive())
  {
    // Can't call TriggerLocalBoundsUpdate because it checks whether we are active, which is not the case anymore.
    GetOwner()->UpdateLocalBounds();
  }
}

void ezRenderComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  ezBoundingBoxSphere bounds = ezBoundingBoxSphere::MakeInvalid();

  bool bAlwaysVisible = false;

  if (GetLocalBounds(bounds, bAlwaysVisible, msg).Succeeded())
  {
    ezSpatialData::Category category = GetOwner()->IsDynamic() ? ezDefaultSpatialDataCategories::RenderDynamic : ezDefaultSpatialDataCategories::RenderStatic;

    if (bounds.IsValid())
    {
      msg.AddBounds(bounds, category);
    }

    if (bAlwaysVisible)
    {
      msg.SetAlwaysVisible(category);
    }
  }
}

void ezRenderComponent::InvalidateCachedRenderData()
{
  if (IsActiveAndInitialized())
  {
    ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

void ezRenderComponent::TriggerLocalBoundsUpdate()
{
  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezRenderComponent::QueueLocalBoundsUpdate()
{
  if (IsActiveAndInitialized())
  {
    GetOwner()->QueueLocalBoundsUpdate();
  }
}

// static
ezUInt32 ezRenderComponent::GetUniqueIdForRendering(const ezComponent& component)
{
  ezUInt32 uniqueId = component.GetUniqueID();
  if (uniqueId == ezInvalidIndex)
  {
    uniqueId = component.GetOwner()->GetHandle().GetInternalID().m_InstanceIndex;
  }

  const ezUInt32 dynamicBit = (1 << 31);
  const ezUInt32 dynamicBitMask = ~dynamicBit;
  return (uniqueId & dynamicBitMask) | (component.GetOwner()->IsDynamic() ? dynamicBit : 0);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RenderComponent);
