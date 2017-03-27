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

void ezRenderComponent::Initialize()
{
  TriggerLocalBoundsUpdate(true);
}

void ezRenderComponent::OnBeforeDetachedFromObject()
{
  TriggerLocalBoundsUpdate(false);
}

void ezRenderComponent::OnActivated()
{
  if (ezGameObject* pOwner = GetOwner())
  {
    pOwner->UpdateLocalBounds();
  }
}

void ezRenderComponent::OnDeactivated()
{
  if (ezGameObject* pOwner = GetOwner())
  {
    pOwner->UpdateLocalBounds();
  }
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

void ezRenderComponent::TriggerLocalBoundsUpdate(bool bIncludeOwnBounds)
{
  if (!IsActive())
    return;

  if (bIncludeOwnBounds)
  {
    GetOwner()->UpdateLocalBounds();
  }
  else
  {
    // temporary set to inactive so we don't receive the msg
    SetActive(false);
    GetOwner()->UpdateLocalBounds();
    SetActive(true);
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RenderComponent);

