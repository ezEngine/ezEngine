#pragma once

#include <RtsGamePlugin/RtsGamePluginDLL.h>

using RtsSelectableComponentManager = ezComponentManager<class RtsSelectableComponent, ezBlockStorageType::Compact>;

struct ezMsgUpdateLocalBounds;

class EZ_RTSGAMEPLUGIN_DLL RtsSelectableComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(RtsSelectableComponent, ezComponent, RtsSelectableComponentManager);

public:
  RtsSelectableComponent();
  ~RtsSelectableComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent interface

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  virtual void OnActivated() override;
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg);

  //////////////////////////////////////////////////////////////////////////
  // Properties
public:
  float m_fSelectionRadius = 1.0f;

  static ezSpatialData::Category s_SelectableCategory;

protected:
};
