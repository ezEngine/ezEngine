#pragma once

#include <RendererCore/Basics.h>
#include <Core/World/World.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>

class EZ_RENDERERCORE_DLL ezRenderComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezRenderComponent, ezComponent);

public:
  ezRenderComponent();
  ~ezRenderComponent();

  virtual void Initialize() override;
  virtual void OnBeforeDetachedFromObject() override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg);

  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds) = 0;

  void TriggerLocalBoundsUpdate(bool bIncludeOwnBounds);
};
