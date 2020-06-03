#pragma once

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/World.h>
#include <RendererCore/RendererCoreDLL.h>

class EZ_RENDERERCORE_DLL ezRenderComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezRenderComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

protected:
  virtual void Deinitialize() override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  ezRenderComponent();
  ~ezRenderComponent();

  /// \brief Called by ezRenderComponent::OnUpdateLocalBounds().
  /// If EZ_SUCCESS is returned, \a bounds and \a bAlwaysVisible will be integrated into the ezMsgUpdateLocalBounds result,
  /// otherwise the out values are simply ignored.
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) = 0;

  void TriggerLocalBoundsUpdate();

  static ezUInt32 GetUniqueIdForRendering(const ezComponent* pComponent, ezUInt32 uiInnerIndex = 0, ezUInt32 uiInnerIndexShift = 24);

  EZ_ALWAYS_INLINE ezUInt32 GetUniqueIdForRendering(ezUInt32 uiInnerIndex = 0, ezUInt32 uiInnerIndexShift = 24) const
  {
    return GetUniqueIdForRendering(this, uiInnerIndex, uiInnerIndexShift);
  }

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void InvalidateCachedRenderData();
};
