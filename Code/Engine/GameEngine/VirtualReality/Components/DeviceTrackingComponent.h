#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/Basics.h>
#include <GameEngine/Interfaces/VRInterface.h>

struct ezVRTransformSpace
{
  typedef ezUInt8 StorageType;
  enum Enum : ezUInt8
  {
    Local,  ///< Sets the local transform to the pose in stage space. Use if owner is direct child of ezStageSpaceComponent.
    Global, ///< Uses the global transform of the device in world space.
    Default = Local,
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezVRTransformSpace);

//////////////////////////////////////////////////////////////////////////

typedef ezComponentManagerSimple<class ezDeviceTrackingComponent, ezComponentUpdateType::WhenSimulating> ezDeviceTrackingComponentManager;

/// \brief Tracks the position of a VR device and applies it to the owner.
class EZ_GAMEENGINE_DLL ezDeviceTrackingComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezDeviceTrackingComponent, ezComponent, ezDeviceTrackingComponentManager);

public:
  ezDeviceTrackingComponent();
  ~ezDeviceTrackingComponent();

  /// \brief Sets the type of device this component is going to track.
  void SetDeviceType(ezEnum<ezVRDeviceType> type);
  ezEnum<ezVRDeviceType> GetDeviceType() const;

  /// \brief Whether to set the owner's local or global transform, see ezVRTransformSpace.
  void SetTransformSpace(ezEnum<ezVRTransformSpace> space);
  ezEnum<ezVRTransformSpace> GetTransformSpace() const;

  //
  // ezComponent Interface
  //

protected:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  void Update();

  ezEnum<ezVRDeviceType> m_deviceType;
  ezEnum<ezVRTransformSpace> m_space;
};
