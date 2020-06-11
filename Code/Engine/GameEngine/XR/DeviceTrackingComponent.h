#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/XRInterface.h>
#include <GameEngine/XR/XRInputDevice.h>

struct ezXRPoseLocation
{
  using StorageType = ezUInt8;
  enum Enum : ezUInt8
  {
    Grip,
    Aim,
    Default = Grip,
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezXRPoseLocation);

//////////////////////////////////////////////////////////////////////////

typedef ezComponentManagerSimple<class ezDeviceTrackingComponent, ezComponentUpdateType::WhenSimulating> ezDeviceTrackingComponentManager;

/// \brief Tracks the position of a XR device and applies it to the owner.
class EZ_GAMEENGINE_DLL ezDeviceTrackingComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezDeviceTrackingComponent, ezComponent, ezDeviceTrackingComponentManager);

public:
  ezDeviceTrackingComponent();
  ~ezDeviceTrackingComponent();

  /// \brief Sets the type of device this component is going to track.
  void SetDeviceType(ezEnum<ezXRDeviceType> type);
  ezEnum<ezXRDeviceType> GetDeviceType() const;

  void SetPoseLocation(ezEnum<ezXRPoseLocation> poseLocation);
  ezEnum<ezXRPoseLocation> GetPoseLocation() const;

  /// \brief Whether to set the owner's local or global transform, see ezXRTransformSpace.
  void SetTransformSpace(ezEnum<ezXRTransformSpace> space);
  ezEnum<ezXRTransformSpace> GetTransformSpace() const;

  //
  // ezComponent Interface
  //

protected:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  void Update();

  ezEnum<ezXRDeviceType> m_deviceType;
  ezEnum<ezXRPoseLocation> m_poseLocation;
  ezEnum<ezXRTransformSpace> m_space;
};

