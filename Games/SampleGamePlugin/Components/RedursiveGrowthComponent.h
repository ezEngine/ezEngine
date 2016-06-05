#pragma once

#include <SampleGamePlugin/SampleGamePlugin.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>

typedef ezComponentManagerSimple<class RecursiveGrowthComponent, true> RecursiveGrowthComponentManager;

class RecursiveGrowthComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(RecursiveGrowthComponent, ezComponent, RecursiveGrowthComponentManager);

public:
  RecursiveGrowthComponent();

  void SerializeComponent(ezWorldWriter& stream) const;
  void DeserializeComponent(ezWorldReader& stream);

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