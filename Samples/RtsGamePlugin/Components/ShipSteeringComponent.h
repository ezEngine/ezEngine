#pragma once

#include <RtsGamePlugin/RtsGamePlugin.h>
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

  //////////////////////////////////////////////////////////////////////////
  // Message Handlers
public:
  void OnMsgNavigateTo(RtsMsgNavigateTo& msg);

  //////////////////////////////////////////////////////////////////////////
  //

protected:
  void UpdateSteering();

  enum class Mode { None, Steering };

  Mode m_Mode = Mode::None;
  ezVec3 m_vTargetPosition;
};

