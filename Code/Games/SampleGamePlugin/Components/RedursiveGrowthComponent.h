#pragma once

#include <SampleGamePlugin/SampleGamePluginDLL.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>

typedef ezComponentManagerSimple<class RecursiveGrowthComponent, ezComponentUpdateType::WhenSimulating> RecursiveGrowthComponentManager;

class RecursiveGrowthComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(RecursiveGrowthComponent, ezComponent, RecursiveGrowthComponentManager);

public:
  RecursiveGrowthComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void OnSimulationStarted() override;

  void Update();

private:
  //////////////////////////////////////////////////////////////////////////
  /// Properties

  ezUInt8 m_uiNumChildren;
  ezUInt8 m_uiRecursionDepth;

protected:
  ezUInt32 m_uiChild;



  virtual void Initialize() override;

};
