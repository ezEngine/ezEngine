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

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg);

  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) = 0;

  void TriggerLocalBoundsUpdate();

  static ezUInt32 GetUniqueIdForRendering(const ezComponent* pComponent, ezUInt32 uiInnerIndex = 0, ezUInt32 uiInnerIndexShift = 24);

  EZ_ALWAYS_INLINE ezUInt32 GetUniqueIdForRendering(ezUInt32 uiInnerIndex = 0, ezUInt32 uiInnerIndexShift = 24) const
  {
    return GetUniqueIdForRendering(this, uiInnerIndex, uiInnerIndexShift);
  }
};
