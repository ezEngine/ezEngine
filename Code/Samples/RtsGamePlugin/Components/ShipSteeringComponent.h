#pragma once

#include <RtsGamePlugin/Components/ComponentMessages.h>
#include <RtsGamePlugin/RtsGamePluginDLL.h>

class RtsShipSteeringComponentManager : public ezComponentManager<class RtsShipSteeringComponent, ezBlockStorageType::Compact>
{
public:
  RtsShipSteeringComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;

  void SteeringUpdate(const ezWorldModule::UpdateContext& context);
};


class EZ_RTSGAMEPLUGIN_DLL RtsShipSteeringComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(RtsShipSteeringComponent, ezComponent, RtsShipSteeringComponentManager);

public:
  RtsShipSteeringComponent();
  ~RtsShipSteeringComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent interface

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // Properties
public:
  float m_fMaxSpeed = 5.0f;
  float m_fMaxAcceleration = 5.0f;
  float m_fMaxDeceleration = 10.0f;
  ezAngle m_MaxTurnSpeed;

  //////////////////////////////////////////////////////////////////////////
  // Message Handlers
public:
  void OnMsgNavigateTo(RtsMsgNavigateTo& ref_msg);
  void OnMsgStopNavigation(RtsMsgStopNavigation& ref_msg);

  //////////////////////////////////////////////////////////////////////////
  //

protected:
  void UpdateSteering();

  enum class Mode
  {
    None,
    Steering,
    Stop
  };

  Mode m_Mode = Mode::None;
  ezVec2 m_vTargetPosition;
  float m_fCurrentSpeed = 0;
};
