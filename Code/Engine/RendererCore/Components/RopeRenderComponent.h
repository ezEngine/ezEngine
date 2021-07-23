#pragma once

#include <Foundation/Math/Declarations.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

struct ezMsgRopePoseUpdated;

// TODO: don't use an updating manager, at all
//using ezRopeRenderComponentManager = ezComponentManager<class ezRopeRenderComponent, ezBlockStorageType::Compact>;
using ezRopeRenderComponentManager = ezComponentManagerSimple<class ezRopeRenderComponent, ezComponentUpdateType::Always, ezBlockStorageType::Compact>;

class EZ_RENDERERCORE_DLL ezRopeRenderComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRopeRenderComponent, ezRenderComponent, ezRopeRenderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

  //////////////////////////////////////////////////////////////////////////
  // ezRopeRenderComponent

public:
  ezRopeRenderComponent();
  ~ezRopeRenderComponent();

  ezColor m_Color = ezColor::Black;

private:
  void OnRopePoseUpdated(ezMsgRopePoseUpdated& msg); // [ msg handler ]
  void Update();

  ezBoundingBoxSphere m_LocalBounds;

  // TODO: don't use debug rendering
  ezDynamicArray<ezDebugRenderer::Line> m_Lines;
};
