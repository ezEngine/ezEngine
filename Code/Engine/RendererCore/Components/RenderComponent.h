#pragma once

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/World.h>
#include <RendererCore/RendererCoreDLL.h>

/// \brief Base class for objects that should be rendered.
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
  ///
  /// If EZ_SUCCESS is returned, out_bounds and out_bAlwaysVisible will be integrated into the ezMsgUpdateLocalBounds ref_msg,
  /// otherwise the out values are simply ignored.
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) = 0;

  /// \brief Call this when some value was modified that affects the size of the local bounding box and it should be recomputed.
  void TriggerLocalBoundsUpdate();

  /// \brief Computes a unique ID for the given component, that is usually given to the renderer to distinguish objects.
  static ezUInt32 GetUniqueIdForRendering(const ezComponent& component, ezUInt32 uiInnerIndex = 0, ezUInt32 uiInnerIndexShift = 24);

  /// \brief Computes a unique ID for the given component, that is usually given to the renderer to distinguish objects.
  EZ_ALWAYS_INLINE ezUInt32 GetUniqueIdForRendering(ezUInt32 uiInnerIndex = 0, ezUInt32 uiInnerIndexShift = 24) const
  {
    return GetUniqueIdForRendering(*this, uiInnerIndex, uiInnerIndexShift);
  }

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void InvalidateCachedRenderData();
};
