#include <PCH.h>
#include <RendererCore/Components/RenderComponent.h>

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezRenderComponent, 1)
{
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezUpdateLocalBoundsMessage, OnUpdateLocalBounds)
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_ABSTRACT_COMPONENT_TYPE


ezRenderComponent::ezRenderComponent()
{

}

ezRenderComponent::~ezRenderComponent()
{

}

void ezRenderComponent::OnActivated()
{
  TriggerLocalBoundsUpdate();
}

void ezRenderComponent::OnDeactivated()
{
  // Can't call TriggerLocalBoundsUpdate because it checks whether we are active, which is not the case anymore.
  GetOwner()->UpdateLocalBounds();
}

void ezRenderComponent::OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg)
{
  ezBoundingBoxSphere bounds;
  bounds.SetInvalid();

  bool bAlwaysVisible = false;

  if (GetLocalBounds(bounds, bAlwaysVisible).Succeeded())
  {
    if (bounds.IsValid())
    {
      msg.AddBounds(bounds);
    }

    if (bAlwaysVisible)
    {
      msg.SetAlwaysVisible();
    }
  }
}

void ezRenderComponent::TriggerLocalBoundsUpdate()
{
  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

//static
ezUInt32 ezRenderComponent::GetUniqueIdForRendering(const ezComponent* pComponent, ezUInt32 uiInnerIndex /*= 0*/, ezUInt32 uiInnerIndexShift /*= 24*/)
{
  ezUInt32 uniqueId = pComponent->GetUniqueID();
  if (uniqueId == ezInvalidIndex)
  {
    uniqueId = pComponent->GetOwner()->GetHandle().GetInternalID().m_Data;
  }
  else
  {
    uniqueId |= (uiInnerIndex << uiInnerIndexShift);
  }

  const ezUInt32 dynamicBit = (1 << 31);
  const ezUInt32 dynamicBitMask = ~dynamicBit;
  return (uniqueId & dynamicBitMask) | (pComponent->GetOwner()->IsDynamic() ? dynamicBit : 0);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RenderComponent);

