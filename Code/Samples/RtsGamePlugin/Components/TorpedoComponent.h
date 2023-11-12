#pragma once

#include <RtsGamePlugin/RtsGamePluginDLL.h>

struct RtsMsgSetTarget;

using RtsTorpedoComponentManager = ezComponentManagerSimple<class RtsTorpedoComponent, ezComponentUpdateType::WhenSimulating>;

class EZ_RTSGAMEPLUGIN_DLL RtsTorpedoComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(RtsTorpedoComponent, ezComponent, RtsTorpedoComponentManager);

public:
  RtsTorpedoComponent();
  ~RtsTorpedoComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent interface

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // Properties
public:
  float m_fSpeed = 10.0f;
  ezInt16 m_iDamage = 10;

  //////////////////////////////////////////////////////////////////////////
  //
public:
  void OnMsgSetTarget(RtsMsgSetTarget& ref_msg);

protected:
  void Update();

  ezGameObjectHandle m_hTargetObject;
  ezVec2 m_vTargetPosition;
};
