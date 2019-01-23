#pragma once

#include <RtsGamePlugin/RtsGamePlugin.h>

struct RtsMsgSetTarget;

typedef ezComponentManagerSimple<class RtsTorpedoComponent, ezComponentUpdateType::WhenSimulating> RtsTorpedoComponentManager;

class EZ_RTSGAMEPLUGIN_DLL RtsTorpedoComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(RtsTorpedoComponent, ezComponent, RtsTorpedoComponentManager);

public:
  RtsTorpedoComponent();
  ~RtsTorpedoComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent interface

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // Properties
public:

  float m_fSpeed = 10.0f;
  ezInt16 m_iDamage = 10;

  //////////////////////////////////////////////////////////////////////////
  //
public:

  void OnMsgSetTarget(RtsMsgSetTarget& msg);

protected:
  void Update();

  ezGameObjectHandle m_hTargetObject;
  ezVec2 m_vTargetPosition;
};

