#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

typedef ezComponentManagerSimple<class RecursiveGrowthComponent, ezComponentUpdateType::WhenSimulating> RecursiveGrowthComponentManager;

class RecursiveGrowthComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(RecursiveGrowthComponent, ezComponent, RecursiveGrowthComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // RecursiveGrowthComponent

public:
  RecursiveGrowthComponent();
  ~RecursiveGrowthComponent();

  ezUInt8 m_uiNumChildren = 2;
  ezUInt8 m_uiRecursionDepth = 2;

protected:
  void Update();

  ezUInt32 m_uiChild = 0;
};
