#pragma once

#include <RtsGamePlugin/RtsGamePluginDLL.h>
#include <RtsGamePlugin/Components/ComponentMessages.h>

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

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

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
  void OnMsgNavigateTo(RtsMsgNavigateTo& msg);
  void OnMsgStopNavigation(RtsMsgStopNavigation& msg);

  //////////////////////////////////////////////////////////////////////////
  //

protected:
  void UpdateSteering();

  enum class Mode { None, Steering, Stop };

  Mode m_Mode = Mode::None;
  ezVec2 m_vTargetPosition;
  float m_fCurrentSpeed = 0;
};

