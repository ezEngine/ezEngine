#pragma once

#include <RendererCore/Components/RenderComponent.h>

typedef ezComponentManager<class ezAlwaysVisibleComponent, ezBlockStorageType::Compact> ezAlwaysVisibleComponentManager;

class EZ_RENDERERCORE_DLL ezAlwaysVisibleComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAlwaysVisibleComponent, ezRenderComponent, ezAlwaysVisibleComponentManager);

public:
  ezAlwaysVisibleComponent();
  ~ezAlwaysVisibleComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent Interface
public:

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent Interface

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;
};
