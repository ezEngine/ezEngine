#pragma once

#include <RtsGamePlugin/RtsGamePluginDLL.h>

typedef ezComponentManager<class RtsSelectableComponent, ezBlockStorageType::Compact> RtsSelectableComponentManager;

struct ezMsgUpdateLocalBounds;

class EZ_RTSGAMEPLUGIN_DLL RtsSelectableComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(RtsSelectableComponent, ezComponent, RtsSelectableComponentManager);

public:
  RtsSelectableComponent();
  ~RtsSelectableComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent interface

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void OnActivated() override;
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);

  //////////////////////////////////////////////////////////////////////////
  // Properties
public:

  float m_fSelectionRadius = 1.0f;

protected:

};

