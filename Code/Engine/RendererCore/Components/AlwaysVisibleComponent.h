#pragma once

#include <RendererCore/Components/RenderComponent.h>

typedef ezComponentManager<class ezAlwaysVisibleComponent, ezBlockStorageType::Compact> ezAlwaysVisibleComponentManager;

/// \brief Attaching this component to a game object makes the renderer consider it always visible, ie. disables culling
class EZ_RENDERERCORE_DLL ezAlwaysVisibleComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAlwaysVisibleComponent, ezRenderComponent, ezAlwaysVisibleComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezAlwaysVisibleComponent

public:
  ezAlwaysVisibleComponent();
  ~ezAlwaysVisibleComponent();
};
