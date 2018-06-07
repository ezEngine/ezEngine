#pragma once

#include <RtsGamePlugin/RtsGamePlugin.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>

typedef ezComponentManager<class RtsSelectableComponent, ezBlockStorageType::Compact> RtsSelectableComponentManager;

class EZ_RTSGAMEPLUGIN_DLL RtsSelectableComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(RtsSelectableComponent, ezComponent, RtsSelectableComponentManager);

public:
  RtsSelectableComponent();
  ~RtsSelectableComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // Properties
public:

  float m_fSelectionRadius = 1.0f;

};

